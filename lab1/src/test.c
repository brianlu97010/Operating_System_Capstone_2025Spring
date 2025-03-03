#include "muart.h"

void test_bss() {
    extern int bss_begin;
    extern int bss_end;
    int *bss_ptr;
    
    for (bss_ptr = &bss_begin; bss_ptr < &bss_end; bss_ptr++) {
        if (*bss_ptr != 0) {
            muart_puts("BSS not properly initialized !\n");
            return;
        }
    }
    muart_puts("BSS initialization successful !\n");
}

void test_stack() {
    unsigned long sp;
    __asm__ volatile("mov %0, sp" : "=r"(sp));
    
    muart_puts("Current stack pointer: ");
    muart_send_hex(sp);
    muart_puts("\n");
}