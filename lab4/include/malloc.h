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
#define LOG_MALLOC 1

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
#endif