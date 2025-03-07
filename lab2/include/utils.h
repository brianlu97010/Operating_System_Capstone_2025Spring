/*
 * Some useful functions and useful definition
*/

#ifndef _UTILS_H
#define _UTILS_H

/* Size types */
typedef unsigned long    size_t;    // For sizes and indices
typedef   signed long   ssize_t;    // Signed size

/* NULL */
#ifndef NULL
#define NULL ((void*)0)
#endif


/* Register operations */
void regWrite(long addr, int val);
int regRead(long addr);
void waitCycle(unsigned int cycles);

#endif