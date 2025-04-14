#include "malloc.h"
#include "types.h"
#include "muart.h"

// External symbols defined in the linker script
extern char heap_begin;
extern char heap_end;

// Set the next_free pointer to point to the address where heap_begin symbol is located
// next_free pointer will always point to the first available byte
static char* next_free = &heap_begin;

void* simple_alloc(size_t size) {
    // Align the size to 8 bytes (for ARMv8 64bits)
    size = (size + 7) & ~7;
    
    // Check whether the pre-allocated memory pool have enough space
    if (next_free + size > &heap_end) {
        return NULL; // Out of memory
    }
    
    // Save the current address
    void* result = next_free;
    
    // Advance the pointer
    next_free += size;
    
    return result;
}

/*
 * **************************
 *      Buddy Allocator     *
 * **************************
 */

// Maximum buddy order - determines the maximum allocation size
// Demo constraint : 1 <= size <= 2^31-1 = 2GB - 1 bytes
// So I need to make sure that the maximum block size is 2GB
// Since page size is 4KB, then the maximun block needs 2GB/4KB = 2^31/2^12 = 2^19 pages
// But https://www.raspberrypi.com/products/raspberry-pi-3-model-b/ says that Raspberry Pi 3 Model B+ only has 1GB RAM
// #define MAX_ORDER 19

#include "list.h"

/* Internal Function */
static void split_page(buddy_system_t* buddy, page_t* page, unsigned int high_order, unsigned int low_order);
static inline page_t* find_buddy(buddy_system_t* buddy, page_t* page, unsigned int order);
static void print_size(size_t size);

/* Todo : Buddy Allocator API */
/* Initialize a buddy system */
void buddy_init(buddy_system_t* buddy, void* mem_start, size_t mem_size, page_t* page_array){
    unsigned int nr_pages = mem_size >> PAGE_SHIFT;     // calculate the number of pages by mem_size / page_size
    unsigned long base_pfn = (unsigned long)mem_start >> PAGE_SHIFT;    // calculate the base physical frame number 
    
    buddy->base_pfn = base_pfn;
    buddy->total_pages = nr_pages;
    buddy->pages = page_array;

    // Initialize all free area lists
    for(int i = 0; i < MAX_ORDER; i++){
        INIT_LIST_HEAD(&buddy->free_area[i].free_list); // All order's free lists initialize to an empty list (Only contains the node named "free_list", no any page list) 
        buddy->free_area[i].nr_free = 0;
    }

    // Initialize all pages
    for( int i = 0; i < nr_pages; i++){
        page_t* page = &page_array[i];
        INIT_LIST_HEAD(&page->list);
        page->flag = PAGE_FLAG_UNUSED;
        page->order = 0;
    }

    // Add all pages to the highest possible order free lists
    int idx = 0;
    while( idx < nr_pages ){
        unsigned int order = 0;
        unsigned int size = 0;

        // Find the highest order that can be used for this page
        for(order = MAX_ORDER-1; order > 0; order--){
            size = 1U << order; // 2^order
            unsigned int mask = size - 1;

            // Check if the page is alighned to this order's size
            if(!(idx & mask) && idx + size <= nr_pages){
                break;
            }
        }

        // Update the all pages in this block with the correct order
        for(int j = 0; j < size; j++){
            page_array[idx+j].order = order;
        }

        // Add the first page in free block to the free list
        // list_add(&page_array[idx].list, &buddy->free_area[order].free_list);
        // 如果要讓 0 在第一個，可能要改成 list_add_tail，把新加的放尾巴
        list_add_tail(&page_array[idx].list, &buddy->free_area[order].free_list);
        buddy->free_area[order].nr_free++;

        // Move to the next block
        idx += size;
    }
}

/* Allocate 2^order pages, return the starting address of the allocated page in the certain order free list */
void *buddy_alloc_pages(buddy_system_t* buddy, unsigned int request_order ){
    // todo
    #if LOG_MALLOC
    muart_puts("\r\n[Page] Allocate ");
    print_size((1 << request_order) * PAGE_SIZE);
    muart_puts(" at order ");
    muart_send_dec(request_order);
    muart_puts("\r\n");
    #endif

    unsigned int current_order = 0;
    page_t* page = NULL;

    // Examine the request order cannot exceed max order
    if( request_order >= MAX_ORDER ){
        return NULL;
    }

    // Start from the request order, find smallest available order that can satisfy the request
    for(current_order=request_order; current_order < MAX_ORDER; current_order++){
        // Check the certain order free list is empty or not
        if(!list_empty(&buddy->free_area[current_order].free_list)){
            page = list_first_entry(&buddy->free_area[current_order].free_list, page_t, list);  // Get the first page in the certain order free list
            list_del(&page->list);      // Delete the chosen page in the free list
            buddy->free_area[current_order].nr_free--; 

            // Demo msg
            #if LOG_MALLOC
            muart_puts("[-] Get Page ");
            muart_send_dec((unsigned int)(page - buddy->pages));
            muart_puts(" in order ");
            muart_send_dec(current_order);
            muart_puts("\t Range of free blocks: [");
            muart_send_dec((unsigned int)(page - buddy->pages));
            muart_puts(", ");
            muart_send_dec((unsigned int)(page+(1 << current_order) - buddy->pages - 1));
            muart_puts("]");
            muart_puts("\r\n");
            #endif 

            break;  // Found a page, exit the loop
        }
    }

    if(!page){
        return NULL;
    }

    // If we got a higher order page, split it to the request order
    if( current_order > request_order ){
        #if LOG_MALLOC
        muart_puts("Start Spliting: \r\n");
        #endif 
        split_page(buddy, page, current_order, request_order);
    }
    #if LOG_MALLOC
    else{
        muart_puts("Exist at least one free block in order ");
        muart_send_dec(current_order);
        muart_puts("\r\n");
    }
    #endif

    // Update the order of the allocate page 
    page->order = request_order;

    // Mark the page as used
    page->flag = PAGE_FLAG_USED;

    // Calculate the allocate physical page frame number
    unsigned int page_idx = page - buddy->pages;
    unsigned long allocated_pfn = buddy->base_pfn + page_idx;

    // Return the physical address of the allocated page
    void* allocated_addr = (void*)(allocated_pfn << PAGE_SHIFT);

    #if LOG_MALLOC
    if (allocated_addr){
        muart_puts("Allocated at address ");
        muart_send_hex((unsigned int)allocated_addr);
        muart_puts(" at order ");
        muart_send_dec(page->order);
        muart_puts(", page");
        muart_send_dec(page_idx);
        muart_puts("\r\n");
    } 
    else{
        muart_puts(" FAILED - Out of memory\r\n");
    }
    #endif

    return allocated_addr;
}

/* Free the allocated pages */
void buddy_free_pages(buddy_system_t* buddy, void* allocated_addr){
    // todo
    if (!allocated_addr){
        #if LOG_MALLOC
        muart_puts("[Page] Free: NULL pointer\r\n");
        #endif
        return;
    }

    // Calculate the page index
    unsigned long allocated_pfn = (unsigned long)allocated_addr >> PAGE_SHIFT;
    unsigned int page_idx = allocated_pfn - buddy->base_pfn;

    // Check if the page index is valid
    if( page_idx >= buddy->total_pages ){
        #if LOG_MALLOC
        muart_puts("[Page] Free: Invalid address ");
        muart_send_hex((unsigned int)allocated_addr);
        muart_puts("\r\n");
        #endif
        return;
    }
    
    
    page_t* page = &buddy->pages[page_idx];
    page_t* buddy_page;
    
    // Check if page is allocated
    if(page->flag == PAGE_FLAG_UNUSED){
        return;
    }
    
    // Set the flag to unused
    page->flag = PAGE_FLAG_UNUSED;
    
    unsigned int order = page->order;
    
    #if LOG_MALLOC
        muart_puts("\r\n[Page] Free the memory at address ");
        muart_send_hex((unsigned int)allocated_addr);
        muart_puts(" with order ");
        muart_send_dec(order);
        muart_puts(", page ");
        muart_send_dec(page_idx);
        muart_puts(". Total ");
        print_size((1 << order) * PAGE_SIZE);
        muart_puts("\r\n");
    #endif
    
    // Try to merge with buddies as much as possible until MAX_ORDER - 1 (free_area maintains order 0 ~ MAX_ORDER-1)
    while( order < MAX_ORDER - 1 ){
        page_t* buddy_page = find_buddy(buddy, page, order);

        // Check the buddy page is valid
        if(!buddy_page){
            break;
        }

        // Check if buddy page is free and has same order as allocated page, if satisfy, then we can coalesce it to a bigger memory block
        if ((buddy_page->flag == PAGE_FLAG_USED) || buddy_page->order != order){
            break;
        }

        #if LOG_MALLOC
        muart_puts("[*] Find an unused buddy page ");
        muart_send_dec((unsigned int)(buddy_page - buddy->pages));
        muart_puts(" in order ");
        muart_send_dec(order);
        muart_puts("\r\n");
        #endif
        
        // Remove buddy page from free list
        list_del(&buddy_page->list);
        buddy->free_area[order].nr_free--;

        
        #if LOG_MALLOC
        // Remove buddy page from free list
        muart_puts("[-] Remove Page ");
        muart_send_dec((unsigned int)(buddy_page - buddy->pages));
        muart_puts(" in order ");
        muart_send_dec(order);
        muart_puts("\t Range of free blocks: [");
        muart_send_dec((unsigned int)(buddy_page - buddy->pages));
        muart_puts(", ");
        muart_send_dec((unsigned int)(buddy_page+(1 << order) - buddy->pages - 1));
        muart_puts("]");
        muart_puts("\r\n");
        
        // Coalesce two pages to a bigger block
        muart_puts("[*] Coalesce page ");
        muart_send_dec((unsigned int)(page - buddy->pages));
            muart_puts(" and page ");
            muart_send_dec((unsigned int)(buddy_page - buddy->pages));
            muart_puts(" to order ");
            muart_send_dec(order+1);
            muart_puts("\r\n");
            
            // Page 0
            muart_puts("    [");
            muart_send_dec((unsigned int)(page - buddy->pages));
            muart_puts(", ");
            muart_send_dec((unsigned int)(page+(1 << order) - buddy->pages - 1));
            muart_puts("] + ");
            
            // Page 1
            muart_puts(" [");
            muart_send_dec((unsigned int)(buddy_page - buddy->pages));
            muart_puts(", ");
            muart_send_dec((unsigned int)(buddy_page+(1 << order) - buddy->pages - 1));
            muart_puts("]");
            
            // Final Page
            muart_puts(" => [");
            muart_send_dec((unsigned int)(page - buddy->pages));
            muart_puts(", ");
            muart_send_dec((unsigned int)(page+(1 << (order+1)) - buddy->pages - 1));
            muart_puts("]");
            muart_puts("\r\n");
            #endif 

            // Determine the combined page, only the one with smaller index will be added to free list 
            if( buddy_page < page ){
                page = buddy_page;
            }
            
            // Increase order
            order++;
            page->order = order;
        }
        
        // Add the page to the free list
        list_add(&page->list, &buddy->free_area[order].free_list);
        buddy->free_area[order].nr_free++;
        
        #if LOG_MALLOC
        muart_puts("[+] Add page ");
    muart_send_dec((unsigned int)(page - buddy->pages));
    muart_puts(" to order ");
    muart_send_dec(order);
    muart_puts("\t Range of free blocks: [");
    muart_send_dec((unsigned int)(page - buddy->pages));
    muart_puts(", ");
    muart_send_dec((unsigned int)(page+(1<<order) - buddy->pages - 1));
    muart_puts("]");
    muart_puts("\r\n");
    #endif

}

/* Todo : Usage Function in Buddy System*/
/* Split a higher-order page to get a page of the requested order */
static void split_page(buddy_system_t* buddy, page_t* page, unsigned int high_order, unsigned int low_order){
    // todo
    page_t* buddy_page;

    unsigned int size = 1U << high_order;    // size = 2^order
    
    while(high_order > low_order){
        high_order--;
        size >>= 1; // size = 2^order / 2 = 2^(order-1)

        // Calculate the buddy page
        buddy_page = &page[size];

        // Add the buddy page to the certain order free list
        buddy_page->order = high_order;
        list_add(&buddy_page->list, &buddy->free_area[high_order].free_list);
        buddy->free_area[high_order].nr_free++;

        #if LOG_MALLOC
        muart_puts("[+] Add page ");
        muart_send_dec((unsigned int)(buddy_page - buddy->pages));
        muart_puts(" to order ");
        muart_send_dec(high_order);
        muart_puts("\t Range of free blocks: [");
        muart_send_dec((unsigned int)(buddy_page - buddy->pages));
        muart_puts(", ");
        muart_send_dec((unsigned int)(buddy_page+size - buddy->pages - 1));
        muart_puts("]");
        muart_puts("\r\n");
        #endif
    }
}

/* Find buddy page for the given page and order */
static inline page_t* find_buddy(buddy_system_t* buddy, page_t* page, unsigned int order){
    // todo
    // Calculate the page index
    unsigned int page_idx = page - buddy->pages;
    
    // Find the buddy index by XOR operation
    unsigned int buddy_idx = page_idx ^ (1U << order);  // page index XOR 2^order

    // Check if the buddy index we find is valid
    if(buddy_idx >= buddy->total_pages){
        return NULL;
    }

    return &buddy->pages[buddy_idx];
}


/* Demo */
/* Memory area for buddy system demo */
#define DEMO_MEM_START 0x10000000
#define DEMO_MEM_SIZE  (16 * 1024 * 1024)  /* 16MB */

/* Global buddy system instance */
static buddy_system_t buddy;

/* Page array for buddy system */
static page_t page_array[DEMO_MEM_SIZE / PAGE_SIZE];

/* Helper function to print memory size in human-readable format */
static void print_size(size_t size) {
    if (size < 1024) {
        muart_send_dec(size);
        muart_puts("B");
    } else if (size < 1024 * 1024) {
        muart_send_dec(size / 1024);
        muart_puts("KB");
    } else {
        muart_send_dec(size / (1024 * 1024));
        muart_puts("MB");
    }
}

/* Print buddy system statistics */
void buddy_print_stats() {
    muart_puts("\r\n=== Buddy System Statistics ===\r\n");
    muart_puts("Base address: ");
    muart_send_hex((unsigned int)DEMO_MEM_START);
    muart_puts("\r\n");
    
    muart_puts("Total pages: ");
    muart_send_dec(buddy.total_pages);
    muart_puts(" (");
    print_size(buddy.total_pages * PAGE_SIZE);
    muart_puts(")\r\n");
    
    muart_puts("Free blocks by order:\r\n");
    
    unsigned int total_free = 0;
    for (int current_order = 0; current_order < MAX_ORDER; current_order++) {
        unsigned int pages_in_order = buddy.free_area[current_order].nr_free * (1 << current_order);
        total_free += pages_in_order;
        
        muart_puts("  Order ");
        muart_send_dec(current_order);
        muart_puts(":\t");
        muart_send_dec(buddy.free_area[current_order].nr_free);
        muart_puts(" blocks ");

        if(!list_empty(&buddy.free_area[current_order].free_list)){
            muart_puts("( ");
            struct list_head* pos;
            list_for_each(pos, &buddy.free_area[current_order].free_list){
                page_t *page_entry = list_entry(pos, page_t, list);
                unsigned int page_idx = page_entry - page_array;
                muart_puts("[");
                muart_send_dec(page_idx);
                muart_puts(", ");
                muart_send_dec((unsigned int)(page_entry+(1 << current_order) - page_array - 1));
                muart_puts("]");
                muart_puts(" ");
            }
            muart_puts(")");
        }
        
        muart_puts("\r\n");
    }
    
    muart_puts("Total free: ");
    muart_send_dec(total_free);
    muart_puts(" pages\r\n");
    
    muart_puts("Used: ");
    muart_send_dec(buddy.total_pages - total_free);
    muart_puts(" pages (");
    print_size((buddy.total_pages - total_free) * PAGE_SIZE);
    muart_puts(")\r\n");
    
    muart_puts("================================\r\n");
}

/* Main demo entry point */
void buddy_demo() {
    /* Initialize buddy system with 16MB memory */
    muart_puts("Initializing buddy system demo...\r\n");
    buddy_init(&buddy, (void *)DEMO_MEM_START, DEMO_MEM_SIZE, page_array);
    
    muart_puts("Buddy system initialized with ");
    muart_send_dec(buddy.total_pages);
    muart_puts(" pages (");
    print_size(buddy.total_pages * PAGE_SIZE);
    muart_puts(")\r\n");
    
    buddy_print_stats();
    
    muart_puts("\r\n=== Basic Allocation Test ===\r\n");
    // Allocate test
    void* p1 = buddy_alloc_pages(&buddy, 0);
    buddy_print_stats();

    void* p2 = buddy_alloc_pages(&buddy, 1);
    buddy_print_stats();

    void* p3 = buddy_alloc_pages(&buddy, 2);
    buddy_print_stats();

    void* p4 = buddy_alloc_pages(&buddy, 3);
    buddy_print_stats();

    // Free test
    buddy_free_pages(&buddy, p2);
    buddy_print_stats();

    buddy_free_pages(&buddy, p1);
    buddy_print_stats();

    buddy_free_pages(&buddy, p3);
    buddy_print_stats();

    buddy_free_pages(&buddy, p4);
    buddy_print_stats();
    
    muart_puts("\r\nBuddy allocator demo completed.\r\n");
}