#include "bootloader.h"
#include "muart.h"
#include "utils.h"

void bootloader_main(){
// Initialize mini UART
    muart_init();
    
    // Receive header first
    header_t header;
    
    // Since muart_receive() receives data one byte at a time, casting to unsigned char pointer to enable byte-by-byte access 
    unsigned char* header_ptr = (unsigned char*)&header;
    for (int i = 0; i < 12; i++) {  // 12 : 3 members defined in header_t, each member is 4 bytes
        header_ptr[i] = muart_receive();
    }

    // Validate the signature
    if (header.signature != BOOT_SIGNATURE) {
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
    
    // Validate the checksum
    unsigned long rec_checksum = 0;
    for (int i = 0; i < header.size; i++) {
        rec_checksum += kernel_ptr[i];
    }
    // rec_checksum &= 0xFF;
    

    if (rec_checksum != header.checksum) {
        // return;
    }
    
    // Jump to kernel using function pointer
    void (*kernel_entry)(void) = (void (*)(void))KERNEL_LOAD_ADDR;
    kernel_entry();
}