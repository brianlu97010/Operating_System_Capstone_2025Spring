#ifndef _MM_H
#define _MM_H

#include "types.h"

/**
 * Set a block of memory to zero.
 *
 * @param src Pointer to the memory block to be zeroed
 * @param n Number of bytes to set to zero
 */
void memzero(unsigned long src, unsigned long n);

/**
 * Copy a block of memory from source to destination.
 * 
 * @param dest Pointer to the destination memory area
 * @param src Pointer to the source memory area
 * @param n Number of bytes to copy
 */
void* memcpy(void* dest, const void* src, size_t n);


#endif