#ifndef _PID_H
#define _PID_H

#include "types.h"
#include "bitmap.h"


#define MIN_PID 2
#define MAX_PID 32768

/* PID bitmap structure */
typedef struct {
    DECLARE_BITMAP(pid_bitmap, MAX_PID + 1);
    int last_pid;
} pid_bitmap_t;

/* PID management functions */
void pid_bitmap_init(void);
pid_t pid_alloc(void);
void pid_free(pid_t pid);
unsigned int find_next_zero_bit(const unsigned long *bitmap, unsigned int size, unsigned int offset);
unsigned int find_first_zero_bit(const unsigned long *bitmap, unsigned int size);

#endif 