#ifndef _BOOTLOADER_H
#define _BOOTLOADER_H

#define KERNEL_LOAD_ADDR    0x80000
#define BOOT_SIGNATURE      0x544F4F42  // "BOOT" in ASCII (little endian)

typedef struct header_t{
    unsigned int signature;    // 0x544F4F42
    unsigned int size;         // Kernel size in bytes
    unsigned int checksum;     // For verification
} header_t;


void bootloader_main();

#endif