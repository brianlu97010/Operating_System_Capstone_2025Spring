#include "bootloader.h"
#include "muart.h"
#include "utils.h"

void bootloader_main(void* fdt_addr){
    // Initialize mini UART
    muart_init();
    
    muart_puts("Hello from the bootloader ! \r\n");
    muart_puts("bootloader main function is relocated at ");
    muart_send_hex((unsigned int)bootloader_main);
    muart_puts("\r\n");

    // Receive header first
    header_t header;
    
    // Since muart_receive() receives data one byte at a time, casting to unsigned char pointer to enable byte-by-byte access 
    unsigned char* header_ptr = (unsigned char*)&header;
    for (int i = 0; i < sizeof(header_t); i++){
        header_ptr[i] = muart_receive();
    }

    // Validate the signature
    if (header.signature != BOOT_SIGNATURE){
        muart_puts("ERROR: Invalid signature!\r\n");
        return;
    }
    
    /* Receive kernel8.img
     * Set a pointer to the memory location where the kernel will be loaded
     * Cast the memory address to an unsigned char pointer to enable byte-by-byte access
     * Write the kernel data (in bytes) to Raspi's starting address (0x80000) */
    unsigned char* kernel_ptr = (unsigned char*)KERNEL_LOAD_ADDR;
    for (int i = 0; i < header.size; i++) {
        kernel_ptr[i] = muart_receive();
    }
    
    muart_puts("Kernel transmit complete. Validating checksum...\r\n");

    // Validate the checksum
    unsigned long rec_checksum = 0;
    for (int i = 0; i < header.size; i++) {
        rec_checksum += kernel_ptr[i];
    }
    rec_checksum &= 0xFF;
    

    if (rec_checksum != header.checksum) {
        muart_puts("ERROR: Checksum verification failed!\r\n");
        return;
    }
    
    muart_puts("Checksum valid. Jumping to kernel at ");
    muart_send_hex(KERNEL_LOAD_ADDR);
    muart_puts("...\r\n");

    // Jump to kernel using function pointer, pass the fdt_addr as the parameter (will be stored in x0 reg in kernel/boot.S)
    void (*kernel_entry)(void*) = (void (*)(void*))KERNEL_LOAD_ADDR;
    kernel_entry(fdt_addr);
}