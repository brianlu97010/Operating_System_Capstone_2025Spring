#include "cpio.h"
#include "muart.h"
#include "string.h"
#include "exception.h"
#include "sched.h"
#include "malloc.h"
#include "mm.h"

static unsigned long initramfs_address = 0x20000000;

/* The API for other modules set the initramfs_address */
void set_initramfs_address(unsigned int addr) {
    initramfs_address = addr;
}

/* The API for other modules get the initramfs_address */
const void* get_cpio_addr(void) {
    return (const void*)initramfs_address;
}

/* 
 * Round up to a multiple of 4
 * Will be used to align filedata size to 4 bytes 
 */
inline unsigned int cpio_padded_size(unsigned int size) {
    // If the value is the multiple of 4, then its last 2 bits must be 0
    // Add 3 to ensure round "up" tto the next multiple of 4 
    return (size + 3) & ~3;  
}

/* Convert ASCII string (stored in hex format) to unsigned int (The New ASCII Format uses 8-byte hexadecimal fields) */
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

/* List all files in the CPIO archive */
void cpio_ls(const void* cpio_file_addr){
    const char* current_addr = (const char*)cpio_file_addr;
    cpio_newc_header* header;

    while(1){
        header = (cpio_newc_header*)current_addr; 

        // Get the pathname size from the header and convert it to unsigned int
        unsigned int pathname_size = cpio_hex_to_int(header->c_namesize, 8);

        // Get the filedata size from the header and convert it to unsigned int
        unsigned int filedata_size = cpio_hex_to_int(header->c_filesize, 8);

        // Get the pathname which is followed by the header
        const char *pathname = current_addr + sizeof(cpio_newc_header);

        // Check if reached the trailer
        if( strcmp(pathname, CPIO_TRAILER) == 0 ){
            break;
        }
        
        // Print the filename
        muart_puts(pathname);
        muart_puts("\r\n");

        // Move to the next entry
        // 1. 計算 data 起始位置（相對於 header 起始位置對齊）
        unsigned long data_start = (unsigned long)current_addr + sizeof(cpio_newc_header) + pathname_size;
        data_start = cpio_padded_size(data_start);  // 4-byte align
        
        // 2. 計算下一個 header 位置（相對於 data 起始位置對齊）
        unsigned long next_header = data_start + filedata_size;
        next_header = cpio_padded_size(next_header); // 4-byte align

        current_addr = (const char*)next_header;
    }
    return;
}

/* Print the file data in the CPIO archive */
void cpio_cat(const void* cpio_file_addr, const char* file_name){
    char* current_addr = (char*)cpio_file_addr;
    cpio_newc_header* header;
    char* file_data;

    while(1){
        header = (cpio_newc_header*)current_addr; 

        // Get the pathname size from the header and convert it to unsigned int
        unsigned int pathname_size = cpio_hex_to_int(header->c_namesize, 8);

        // Get the filedata size from the header and convert it to unsigned int
        unsigned int filedata_size = cpio_hex_to_int(header->c_filesize, 8);

        // Get the pathname which is followed by the header (the total size of header is sizeof(cpio_newc_header) bytes)
        char *pathname = current_addr + sizeof(cpio_newc_header);

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
        // 1. 計算 data 起始位置（相對於 header 起始位置對齊）
        unsigned long data_start = (unsigned long)current_addr + sizeof(cpio_newc_header) + pathname_size;
        data_start = cpio_padded_size(data_start);  // 4-byte align
        
        // 2. 計算下一個 header 位置（相對於 data 起始位置對齊）
        unsigned long next_header = data_start + filedata_size;
        next_header = cpio_padded_size(next_header); // 4-byte align

        current_addr = (char*)next_header;
    }
    muart_puts("File not found: ");
    muart_puts(file_name);
    muart_puts("\r\n");
    return;
}

/* Execute the user program in initramfs in EL0 */
void cpio_exec(const void* cpio_file_addr, const char* file_name){
    const char* current_addr = (const char*)cpio_file_addr;
    cpio_newc_header* header;
    void* program_start_addr = NULL;

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
        
        // Store the file information
        if( strcmp(file_name, pathname) == 0 ){
            // Calculate the start address where file data starts
            program_start_addr = (void*)(pathname + pathname_size);
            break;
        }

        // Move to the next entry
        // 1. 計算 data 起始位置（相對於 header 起始位置對齊）
        unsigned long data_start = (unsigned long)current_addr + sizeof(cpio_newc_header) + pathname_size;
        data_start = cpio_padded_size(data_start);  // 4-byte align
        
        // 2. 計算下一個 header 位置（相對於 data 起始位置對齊）
        unsigned long next_header = data_start + filedata_size;
        next_header = cpio_padded_size(next_header); // 4-byte align

        current_addr = (char*)next_header;
    }

    // Check if found the file
    if(program_start_addr == NULL){
        muart_puts("Error: File not found: ");
        muart_puts(file_name);
        muart_puts("\r\n");
        return;
    }
    
    // Print the file information if find
    muart_puts("Executing program: ");
    muart_puts(file_name);
    muart_puts("\r\n");
    
    muart_puts("Program address: ");
    muart_send_hex((unsigned long)program_start_addr);
    muart_puts("\r\n");
    
    // Switch to EL0 and executre the user program
    exec_user_program(program_start_addr);
    return;
}

// Load the user program from initramfs and copy it to the task's user space
unsigned long cpio_load_program(const void* cpio_file_addr, const char* file_name) {
    const char* current_addr = (const char*)cpio_file_addr;
    cpio_newc_header* header;
    void* program_start_addr = NULL;
    unsigned int program_size = 0;

    while(1){
        header = (cpio_newc_header*)current_addr; 

        // Get the pathname size from the header and convert it to unsigned int
        unsigned int pathname_size = cpio_hex_to_int(header->c_namesize, 8);

        // Get the filedata size from the header and convert it to unsigned int
        unsigned int filedata_size = cpio_hex_to_int(header->c_filesize, 8);

        // Get the pathname which is followed by the header
        const char *pathname = current_addr + sizeof(cpio_newc_header);

        // Check if reached the trailer
        if( strcmp(pathname, CPIO_TRAILER) == 0 ){
            break;
        }
        
        // Store the file information
        if( strcmp(file_name, pathname) == 0 ){
            // Calculate the start address where file data starts
            unsigned long data_start = (unsigned long)current_addr + sizeof(cpio_newc_header) + pathname_size;
            data_start = cpio_padded_size(data_start);  // 4-byte align
            program_start_addr = (void*)data_start;
            program_size = filedata_size;
            break;
        }

        // Move to the next entry
        // Calculate data start position (aligned relative to header start position)
        unsigned long data_start = (unsigned long)current_addr + sizeof(cpio_newc_header) + pathname_size;
        data_start = cpio_padded_size(data_start);  // 4-byte align
        
        // Calculate next header position (aligned relative to data start position)
        unsigned long next_header = data_start + filedata_size;
        next_header = cpio_padded_size(next_header); // 4-byte align

        current_addr = (const char*)next_header;
    }

    // Check if found the file
    if(program_start_addr == NULL){
        muart_puts("Error: File not found: ");
        muart_puts(file_name);
        muart_puts("\r\n");
        return 0;
    }
    
    // Print the file information if found
    muart_puts("Found program: ");
    muart_puts(file_name);
    muart_puts(", size: ");
    muart_send_dec(program_size);
    muart_puts(" bytes\r\n");
    
    muart_puts("Program address in initramfs: ");
    muart_send_hex((unsigned long)program_start_addr);
    muart_puts("\r\n");
    
    // Get current task to allocate memory for user program
    struct task_struct* current = (struct task_struct*)get_current_thread();
    
    // Allocate memory for user program (copy to separate memory space)
    current->user_program = dmalloc(program_size);
    if (!current->user_program) {
        muart_puts("Error: Failed to allocate memory for user program\r\n");
        return 0;
    }
    
    // Store program size for cleanup later
    current->user_program_size = program_size;
    
    // Copy program from initramfs to allocated memory
    memcpy(current->user_program, program_start_addr, program_size);
    
    muart_puts("Copied program to allocated memory at: ");
    muart_send_hex((unsigned long)current->user_program);
    muart_puts("\r\n");
    
    return (unsigned long)current->user_program;
}