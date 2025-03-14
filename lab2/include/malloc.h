#ifndef _MALLOC_H
#define _MALLOC_H

/* Size types */
typedef unsigned long size_t;    // For sizes and indices, on a 64-bit system size_t will take 64 bits

/* Allocate memory and return the pointer */
void* simple_alloc(size_t size);

#endif