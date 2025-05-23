#ifndef _MALLOC_H
#define _MALLOC_H

#include "types.h"

/* --- Simple Allocator Interface --- */ 
/* Simple linear allocator (bump allocator) */
void* simple_alloc(size_t size);


/* --- Buddy Allocator Data Structures and API --- */
#include "list.h"  

#define MAX_ORDER 11  /* Maximum allocation order, 2^11 pages = 2048 pages = 8MB */
#define PAGE_SIZE 4096 /* Page size in bytes (4KB) */
#define PAGE_SHIFT 12  /* log2(PAGE_SIZE) */

/* Page flags */
#define PAGE_FLAG_UNUSED 0  // Page frame is allocatable
#define PAGE_FLAG_USED 1    // Page frame is currently allocated

/* Printlog message */
#define LOG_MALLOC 0

/* Page descriptor: decribe properties of the page frame */
typedef struct page_t{
    struct list_head list; // will be linked in the buddy system's free list 
    unsigned int flag;  
    int order;
}page_t;

/* Holds the free pages of a certain order */
typedef struct free_area_t{
    struct list_head free_list;
    unsigned int nr_free;   // number of free pages
}free_area_t;

typedef struct buddy_system_t{
    unsigned long base_pfn; // starting page frame number
    unsigned int total_pages; // total numbers of pages
    struct page_t *pages; // page descriptor
    struct free_area_t free_area[MAX_ORDER]; // free areas of each order
}buddy_system_t;

/* Buddy allocator API */
/* Initialize a buddy system */
void buddy_init(buddy_system_t* buddy, void* mem_start, size_t mem_size, page_t* page_array);

/* Allocate 2^order pages, return the starting address of the allocated page in the certain order free list */
void* buddy_alloc_pages(buddy_system_t* buddy, unsigned int order);

/* Free the allocated pages */
void buddy_free_pages(buddy_system_t* buddy, void* addr);


/* --- Dynamic Allocator Data Structures and API --- */
typedef struct chunk_t {
    struct chunk_t* next;  // List of the next chunk
} chunk_t;

typedef struct pool_t {
    unsigned int chunk_size;        // Size of each chunk in this memory pool
    chunk_t* free_chunks;           // Pointer to the list of free chunks
    void* page_address;             // Starting address of the page
    unsigned int nr_free_chunks;    // Current number of free chunks
    unsigned int total_chunks;      // Total number of chunks in the page
} pool_t;

/* Initialize the memory pools */
void memory_pools_init();

/**
 * @brief Allocates memory of specified size
 * 
 * This function allocates memory of the requested size. 
 * For large allocations, it call the buddy allocator to allocate full pages.
 * For small allocations (less than or equal to the largest chunk size)
 * it round up the requested allocation size to the nearest size and 
 * check if there is any unallocated slot. If not, allocate a new page frame 
 * from the page allocator. Then, return one chunk to the caller., it uses a pool allocator
 * that divides pages into chunks of fixed sizes. 
 * 
 * 
 * @param size The requested size in bytes
 * @return void* Pointer to allocated memory or NULL if allocation fails
 */
void* dmalloc(size_t size);

/**
 * @brief Frees memory allocated with dmalloc
 * 
 * This function returns memory previously allocated with dmalloc back to the
 * appropriate memory pool or to the buddy allocator. If the memory block
 * is page-aligned, it checks if it belongs to a memory pool (chunk 0) before
 * returning it to the buddy allocator. When all chunks in a memory pool page 
 * are freed, the entire page is returned to the buddy allocator.
 * 
 * @param ptr Pointer to memory block to free, or NULL (no-op)
 */
void dfree(void* ptr);

/**
 * Set a block of memory to zero.
 *
 * @param src Pointer to the memory block to be zeroed
 * @param n Number of bytes to set to zero
 */
void memzero(unsigned long src, unsigned long n);

/* Demo */
/* Memory area for buddy system */
#define BUDDY_MEM_START 0x10000000
#define BUDDY_MEM_SIZE  (16 * 1024 * 1024)  /* 16MB */
void dynamic_allocator_demo();

#endif