#include "mm.h"

/**
 * Set a block of memory to zero.
 *
 * @param src Pointer to the memory block to be zeroed
 * @param n Number of bytes to set to zero
 */
void memzero(unsigned long src, unsigned long n) {
    // Use byte-by-byte zeroing for small sizes
    if (n < 8) {
        unsigned char* s = (unsigned char*)src;
        while (n--) {
            *s++ = 0;
        }
        return;
    }
    
    // For 8-byte aligned addresses, use 8-byte zeroing for efficiency
    if ((src & 0x7) == 0) {
        unsigned long* s = (unsigned long*)src;
        unsigned long count = n >> 3;  // Divide by 8
        
        // Zero 8 bytes at a time
        while (count--) {
            *s++ = 0;
        }
        
        // Handle remaining bytes (less than 8)
        unsigned char* s_char = (unsigned char*)s;
        n &= 0x7;  // n % 8
        while (n--) {
            *s_char++ = 0;
        }
    } else {
        // For non-aligned addresses, use byte-by-byte zeroing
        unsigned char* s = (unsigned char*)src;
        while (n--) {
            *s++ = 0;
        }
    }
}


void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}