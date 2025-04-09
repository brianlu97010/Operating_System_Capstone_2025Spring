#ifndef _UTILS_H
#define _UTILS_H

/* Register operations */
void regWrite(long addr, int val);
int regRead(long addr);
void waitCycle(unsigned int cycles);

#endif