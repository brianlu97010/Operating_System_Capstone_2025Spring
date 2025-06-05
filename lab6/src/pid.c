#include "pid.h"
#include "bitmap.h"
#include "muart.h"

/* Global PID bitmap */
static pid_bitmap_t pid_bitmap;

/**
 * Find the first zero bit in the bitmap, starting from offset.
 * 
 * @param bitmap  Base address of the bitmap
 * @param size    Total number of bits in the bitmap
 * @param offset  Start searching from this position
 * @return        Index of the first zero bit found, or size if not found
 */
unsigned int find_next_zero_bit(const unsigned long *bitmap, unsigned int size, unsigned int offset) {
    // If offset is out of range, return size directly
    if (offset >= size) {
        return size;
    }
    
    // Calculate starting element and bit offset
    unsigned int word_offset = BIT_WORD(offset);
    unsigned int bit_offset = offset % BITS_PER_LONG;
    unsigned long word;
    
    // Check bits after offset in the current element
    word = bitmap[word_offset] >> bit_offset;
    
    // If there is a zero bit after the offset in the current word (i.e., ~word has a 1 bit)
    if (~word != 0) {
        bit_offset += __builtin_ffsl(~word) - 1;
        if (bit_offset < BITS_PER_LONG && offset + bit_offset - (offset % BITS_PER_LONG) < size) {
            return offset + bit_offset - (offset % BITS_PER_LONG);
        }
    }
    
    // Check subsequent elements
    size -= (offset + (BITS_PER_LONG - bit_offset));
    word_offset++;
    
    // Handle full unsigned long words
    while (size >= BITS_PER_LONG) {
        if (bitmap[word_offset] != ~0UL) {
            // Found the first zero bit
            bit_offset = __builtin_ffsl(~bitmap[word_offset]) - 1;
            return word_offset * BITS_PER_LONG + bit_offset;
        }
        
        word_offset++;
        size -= BITS_PER_LONG;
    }
    
    // Handle the remaining bits less than one unsigned long
    if (size > 0) {
        word = bitmap[word_offset];
        if (word != ~0UL) {
            bit_offset = __builtin_ffsl(~word) - 1;
            if (bit_offset < size) {
                return word_offset * BITS_PER_LONG + bit_offset;
            }
        }
    }
    
    // If not found, search from the beginning (circular search)
    word_offset = BIT_WORD(MIN_PID);
    unsigned int search_limit = BIT_WORD(offset);
    
    for (; word_offset < search_limit; word_offset++) {
        if (bitmap[word_offset] != ~0UL) {
            bit_offset = __builtin_ffsl(~bitmap[word_offset]) - 1;
            return word_offset * BITS_PER_LONG + bit_offset;
        }
    }
    
    // No zero bit found
    return size;
}

/**
 * Find the first zero bit in the bitmap
 * 
 * @param bitmap  Base address of the bitmap
 * @param size    Total number of bits in the bitmap
 * @return        Index of the first zero bit found, or size if not found
 */
unsigned int find_first_zero_bit(const unsigned long *bitmap, unsigned int size) {
    return find_next_zero_bit(bitmap, size, MIN_PID);
}

/**
 * Initialize the PID bitmap
 */
void pid_bitmap_init(void) {
    // Clear the entire bitmap
    for (int i = 0; i < BITS_TO_LONGS(MAX_PID + 1); i++) {
        pid_bitmap.pid_bitmap[i] = 0;
    }
    
    // Reserve PID 0 and 1
    set_bit(0, pid_bitmap.pid_bitmap); // Reserved for idle task
    set_bit(1, pid_bitmap.pid_bitmap); // Reserved for init task
    
    // Set last_pid
    pid_bitmap.last_pid = 1;
    
    muart_puts("PID bitmap initialized, PIDs range from ");
    muart_send_dec(MIN_PID);
    muart_puts(" to ");
    muart_send_dec(MAX_PID);
    muart_puts("\r\n");
}

/**
 * Allocate a new PID
 * 
 * @return        Allocated PID, or -1 on failure
 */
pid_t pid_alloc(void) {
    // Start searching from the PID after the last allocated one
    int offset = pid_bitmap.last_pid + 1;
    if (offset > MAX_PID) {
        offset = MIN_PID;  // Wrap around to the minimum available PID
    }
    
    // Find the next available PID
    int pid = find_next_zero_bit(pid_bitmap.pid_bitmap, MAX_PID + 1, offset);
    
    // Check if the found PID is valid
    if (pid < MIN_PID || pid > MAX_PID) {
        // If not found or invalid, search again from the minimum PID
        pid = find_next_zero_bit(pid_bitmap.pid_bitmap, MAX_PID + 1, MIN_PID);
        
        // Check again
        if (pid < MIN_PID || pid > MAX_PID) {
            muart_puts("Error: No free PID available\r\n");
            return -1;
        }
    }
    
    // Mark the PID as allocated
    set_bit(pid, pid_bitmap.pid_bitmap);
    pid_bitmap.last_pid = pid;
    
    return pid;
}

/**
 * Free an allocated PID
 * 
 * @param pid     PID to free
 */
void pid_free(pid_t pid) {
    // Check if the PID is valid and allocated
    if (pid < MIN_PID || pid > MAX_PID) {
        muart_puts("Error: Attempted to free invalid PID ");
        muart_send_dec(pid);
        muart_puts(" (out of range)\r\n");
        return;
    }
    
    if (!test_bit(pid, pid_bitmap.pid_bitmap)) {
        muart_puts("Warning: Attempting to free unallocated PID ");
        muart_send_dec(pid);
        muart_puts("\r\n");
        return;
    }
    
    // Free the PID
    clear_bit(pid, pid_bitmap.pid_bitmap);
} 