#ifndef _CPIO_H
#define _CPIO_H

#define CPIO_MAGIC              "070701"
#define CPIO_TRAILER            "TRAILER!!!"

#define RASPI 1
#if RASPI
#define INITRANFS_ADDR          0x20000000 // the memory address where the cpio file load into, defined in config.txt
#else 
#define INITRANFS_ADDR          0x8000000  // QEMU loads the cpio archive file to 0x8000000 by default.
#endif

typedef struct{
    char    c_magic[6];     // string "070701"
    char    c_ino[8];       // The inode numbers from the disk.
    char    c_mode[8];      // The mode specifies both the regular permissions and the file type.
    char    c_uid[8];       // User ID
    char    c_gid[8];       // Group ID
    char    c_nlink[8];     // The number of links to this file.
    char    c_mtime[8];     // Modification time of the file
    char    c_filesize[8];  // size of file data (not including NUL bytes)
    char    c_devmajor[8];
    char    c_devminor[8];
    char    c_rdevmajor[8];
    char    c_rdevminor[8];
    char    c_namesize[8];  // size of pathname (including NUL bytes)
    char    c_check[8]; 
} cpio_newc_header;

// Round up to a multiple of 4
// Will be used to align filedata size to 4 bytes
unsigned int cpio_padded_size(unsigned int);

// Convert ASCII string (stored in hex format) to unsigned int (The New ASCII Format uses 8-byte hexadecimal fields)
unsigned int cpio_hex_to_int(const char*, unsigned int);

// List all files in the CPIO archive
void cpio_ls(const void*);

// Print the file data in the CPIO archive
void cpio_cat(const void*, const char*);

#endif