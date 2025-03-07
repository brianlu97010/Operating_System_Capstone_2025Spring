#include "utils.h"

void regWrite(long addr, int val){
    volatile unsigned int* addr_ptr = (unsigned int*)addr;
    *addr_ptr = val;
    return;
}

int regRead(long addr){
    volatile unsigned int* addr_ptr = (unsigned int*)addr;
    return *addr_ptr;
}

void waitCycle(unsigned int cycles) {
    for (volatile unsigned int i = 0; i < cycles; i++) {
        __asm__ volatile ("nop \n");
    }
    return;
}