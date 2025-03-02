#include "muart.h"
#include "shell.h"

void main(){
    // Initialize mini UART
    muart_init();

    // Start Simple Shell
    shell();
}