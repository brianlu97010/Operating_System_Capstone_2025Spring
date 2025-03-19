#ifndef _CPIO_H
#define _CPIO_H

#define CPIO_MAGIC              "070701"
#define CPIO_TRAILER            "TRAILER!!!"

typedef struct cpio_newc_header{
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

/* Round up to a multiple of 4
 * Will be used to align filedata size to 4 bytes */
unsigned int cpio_padded_size(unsigned int);

/* Convert ASCII string (stored in hex format) to unsigned int (The New ASCII Format uses 8-byte hexadecimal fields) */
unsigned int cpio_hex_to_int(const char*, unsigned int);

/* List all files in the CPIO archive */
void cpio_ls(const void*);

/* Print the file data in the CPIO archive */
void cpio_cat(const void*, const char*);

/* The API for other modules set the initramfs_address */
void set_initramfs_address(unsigned int addr);

/* The API for other modules get the initramfs_address */
const void* get_cpio_addr(void);

#endif