#include "muart.h"
#include "shell.h"
#include "test.h"

void main(){
    // Initialize mini UART
    muart_init();
    
    // Check the bss segment are initialized to 0
    test_bss();

    // Check the stack pointer is set to a proper address
    test_stack();

    // Start Simple Shell
    shell();
}