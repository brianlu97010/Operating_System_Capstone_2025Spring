#include "muart.h"
#include "shell.h"
#include "fdt.h"
#include "cpio.h"
#include "exception.h"
#include "timer.h"
#include "malloc.h"
#include "sched.h"

/* Global buddy system instance */
buddy_system_t buddy;

/* Page array for buddy system */
page_t page_array[BUDDY_MEM_SIZE / PAGE_SIZE];

void main(void* fdt){    
    // Initialize mini UART
    muart_init();

    // Print the dtb loading address
    muart_puts("Kernel : flatten device tree address is at ");
    muart_send_hex(((unsigned int)fdt));
    muart_puts("\r\n");

    // Get the initramfs address from the device tree
    unsigned int initramfs_addr = get_initramfs_address(fdt);
    muart_puts("Initramfs_addr is at ");
    muart_send_hex(initramfs_addr);
    muart_puts("\r\n");

    // Update the initramfs address in CPIO module
    set_initramfs_address(initramfs_addr);

    // Initialize the vector table
    exception_table_init();
    
    core_timer_init();
    muart_puts("Core timer initialized successful !\r\n");

    // Initialize the buddy allocator and memory pools
    buddy_init(&buddy, (void *)BUDDY_MEM_START, BUDDY_MEM_SIZE, page_array);    
    memory_pools_init();
    muart_puts("Dynamic allocator initialized successful !\r\n");

    // Demo of the dynamic allocator
    // dynamic_allocator_demo();

    // Test thread mechanism
    muart_puts("\r\n=== Starting Thread Test ===\r\n");
    thread_test();
    muart_puts("=== Thread Test Completed ===\r\n");

    // Start Simple Shell
    shell();
}