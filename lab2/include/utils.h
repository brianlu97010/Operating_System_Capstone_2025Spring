/*
 * Some useful functions and useful definition
*/

#ifndef _UTILS_H
#define _UTILS_H

/* NULL */
#ifndef NULL
#define NULL ((void*)0)
#endif

/* Register operations */
void regWrite(long addr, int val);
int regRead(long addr);
void waitCycle(unsigned int cycles);

#endif