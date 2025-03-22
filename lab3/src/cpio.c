#include "cpio.h"
#include "muart.h"
#include "string.h"

static unsigned int initramfs_address = 0x20000000;

void set_initramfs_address(unsigned int addr) {
    initramfs_address = addr;
}

const void* get_cpio_addr(void) {
    return (const void*)initramfs_address;
}

inline unsigned int cpio_padded_size(unsigned int size) {
    // If the value is the multiple of 4, then its last 2 bits must be 0
    // Add 3 to ensure round "up" tto the next multiple of 4 
    return (size + 3) & ~3;  
}

unsigned int cpio_hex_to_int(const char* hex, unsigned int len){
    unsigned int result = 0;
    for(int i = 0; i < len; i++){
        char hex_digit = hex[i];
        unsigned int value;

        if( hex_digit >= '0' && hex_digit <= '9'){
            value = hex_digit - '0';
        }
        else if( hex_digit >= 'A' && hex_digit <= 'F'){
            value = hex_digit - 'A' + 10;
        }
        else if( hex_digit >= 'a' && hex_digit <= 'f'){
            value = hex_digit - 'a' + 10;
        }

        result = (result << 4) | value; // left shift 4 bits (1 hex digit equals 4 binary digits)
    }

    return result;
}

void cpio_ls(const void* cpio_file_addr){
    const char* current_addr = (const char*)cpio_file_addr;
    cpio_newc_header* header;

    while(1){
        header = (cpio_newc_header*)current_addr; 

        // Get the pathname size from the header and convert it to unsigned int
        unsigned int pathname_size = cpio_hex_to_int(header->c_namesize, 8);

        // Get the filedata size from the header and convert it to unsigned int
        unsigned int filedata_size = cpio_hex_to_int(header->c_filesize, 8);

        // Get the pathname which is followed by the header (the total size of header is sizeof(cpio_newc_header) bytes)
        const char *pathname = current_addr + sizeof(cpio_newc_header);

        // Check if reached the trailer
        if( strcmp(pathname, CPIO_TRAILER) == 0 ){
            break;
        }
        
        // Print the filename
        muart_puts(pathname);
        muart_puts("\r\n");

        // Move to the next entry
        current_addr = current_addr + sizeof(cpio_newc_header) + pathname_size + cpio_padded_size(filedata_size);
    }
    return;
}

void cpio_cat(const void* cpio_file_addr, const char* file_name){
    const char* current_addr = (const char*)cpio_file_addr;
    cpio_newc_header* header;
    char* file_data;

    while(1){
        header = (cpio_newc_header*)current_addr; 

        // Get the pathname size from the header and convert it to unsigned int
        unsigned int pathname_size = cpio_hex_to_int(header->c_namesize, 8);

        // Get the filedata size from the header and convert it to unsigned int
        unsigned int filedata_size = cpio_hex_to_int(header->c_filesize, 8);

        // Get the pathname which is followed by the header (the total size of header is sizeof(cpio_newc_header) bytes)
        const char *pathname = current_addr + sizeof(cpio_newc_header);

        // Check if reached the trailer
        if( strcmp(pathname, CPIO_TRAILER) == 0 ) {
            break;
        }
        
        // Print the file data of the specific file
        if( strcmp(file_name, pathname) == 0 ){
            file_data = pathname + pathname_size; // file data is followed by the pathname in CPIO format
            for(unsigned int i = 0; i < filedata_size; i++){
                muart_send(*(file_data+i));
            }
            muart_puts("\r\n");
            return;
        }

        // Move to the next entry
        current_addr = current_addr + sizeof(cpio_newc_header) + pathname_size + cpio_padded_size(filedata_size);
    }
    muart_puts("File not found: ");
    muart_puts(file_name);
    muart_puts("\r\n");
    return;
}