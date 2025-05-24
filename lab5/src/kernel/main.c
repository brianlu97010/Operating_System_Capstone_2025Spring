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

// extern void kernel_fork_process();
extern void kernel_fork_process_cpio();

int video_player_test(void* fdt) {
    muart_puts("Starting system call test with initramfs.cpio program ...\r\n");
    
    // Create a kernel thread that will move to user mode
    // pid_t pid = kernel_thread(kernel_fork_process, NULL);

    // Create initialization data
    struct task_init_data* init_data = (struct task_init_data*)dmalloc(sizeof(struct task_init_data));
    if (!init_data) {
        muart_puts("Error: Failed to allocate init data\r\n");
        return -1;
    }
    
    init_data->filename = "syscall.img";
    init_data->initramfs_addr = get_initramfs_address(fdt);
    
    // Create a kernel thread that will load and execute syscall.img
    pid_t pid = kernel_thread(kernel_fork_process_cpio, init_data);
    if (pid < 0) {
        dfree(init_data);
        muart_puts("Error: Failed to create kernel thread\r\n");
        return -1;
    }
    
    muart_puts("Created kernel thread with PID: ");
    muart_send_dec(pid);
    muart_puts("\r\n");
    
    schedule();
    
    return 0;
}

void main(void* fdt){    
    // Initialize mini UART
    muart_init();

    // Print the dtb loading address
    muart_puts("Kernel : flatten device tree address is at ");
    muart_send_hex(((unsigned long)fdt));
    muart_puts("\r\n");

    // Get the initramfs address from the device tree
    unsigned long initramfs_addr = get_initramfs_address(fdt);
    if (initramfs_addr == 1) {
        muart_puts("Error: Failed to get initramfs address from device tree\r\n");
        return;
    }
    muart_puts("Initramfs_addr is at ");
    muart_send_hex(initramfs_addr);
    muart_puts("\r\n");

    // Debug: Print the first few bytes at initramfs_addr
    muart_puts("First 6 bytes at initramfs_addr: ");
    const char* initramfs_ptr = (const char*)initramfs_addr;
    for(int i = 0; i < 6; i++) {
        muart_send(initramfs_ptr[i]);
    }
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

    // Initialize the scheduler
    sched_init();

    // Demo of the dynamic allocator
    // dynamic_allocator_demo();

    // Test thread mechanism
    // muart_puts("\r\n=== Starting Thread Test ===\r\n");
    // thread_test();
    // muart_puts("=== Thread Test Completed ===\r\n");

    // syscall test
    video_player_test(fdt);

    // Start Simple Shell
    shell();
}