#include "muart.h"
#include "shell.h"
#include "fdt.h"
#include "cpio.h"

void main(void* fdt){
    // Initialize mini UART
    muart_init();

    muart_puts("Kernel : flatten device tree address is at ");
    muart_send_hex(((unsigned int)fdt));
    muart_puts("\r\n");

    // Get the initramfs address from the device tree
    unsigned int initramfs_addr = get_initramfs_address(fdt);
    muart_puts("kernel : initramfs_addr is at ");
    muart_send_hex(initramfs_addr);
    muart_puts("\r\n");

    // Update the initramfs address in CPIO module
    set_initramfs_address(initramfs_addr);

    // Start Simple Shell
    shell();
}