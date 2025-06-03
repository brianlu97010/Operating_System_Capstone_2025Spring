#ifndef _TMPFS_H
#define _TMPFS_H

#include "vfs.h"

// For tmpfs, you can assume that component name won’t excced 15 characters, and at most 16 entries for a directory. and at most 4096 bytes for a file.
#define TMPFS_MAX_NAME 15
#define TMPFS_MAX_ENTRIES 16
#define TMPFS_MAX_FILE_SIZE 4096

// File types
#define TMPFS_TYPE_FILE 1
#define TMPFS_TYPE_DIR 2

// Directory entry
struct tmpfs_dirent {
    char name[TMPFS_MAX_NAME + 1];  // commonent name
    struct tmpfs_inode* inode;      
    int used;  // 1 if entry is used, 0 if free
};

// tmpfs inode structure - represents internal data for each file/directory
struct tmpfs_inode {
    int type;  // TMPFS_TYPE_FILE or TMPFS_TYPE_DIR
    
    union {
        // For files: data storage
        struct tmpfs_file {
            char data[TMPFS_MAX_FILE_SIZE];
            size_t size;  // Current file size
        }tmpfs_file;
        
        // For directories: directory entries
        struct tmpfs_dir {
            struct tmpfs_dirent entries[TMPFS_MAX_ENTRIES];
            int entry_count;
        }tmpfs_dir;
    };
    
    struct vnode* vnode;  // Back reference to vnode
};

int tmpfs_setup_mount(struct filesystem* fs, struct mount* mount);

// Initialize tmpfs and register with VFS
int init_tmpfs(void);

#endif