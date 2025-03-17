#include "malloc.h"
#include "types.h"

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
