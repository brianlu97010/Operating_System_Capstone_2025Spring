void regWrite(volatile unsigned int* addr, int val){
    *addr = val;
    return;
}

int regRead(volatile unsigned int* addr){
    return *addr;
}

void waitCycle(unsigned int cycles) {
    for (volatile unsigned int i = 0; i < cycles; i++) {
        __asm__ volatile ("nop \n");
    }
    return;
}