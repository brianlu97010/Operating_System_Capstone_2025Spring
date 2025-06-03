#ifndef _INITRAMFS_H
#define _INITRAMFS_H

#include "vfs.h"

// File types
#define INITRAMFS_TYPE_FILE 1
#define INITRAMFS_TYPE_DIR 2

/* Internal representation of initramfs */
struct initramfs_entry {
    char* name;          // File name
    char* data;          // File content (NULL for directories)
    size_t size;         // File size
    int type;            // INITRAMFS_TYPE_FILE or INITRAMFS_TYPE_DIR
    struct vnode* vnode; // Back reference to vnode
};

/* Initramfs filesystem structure, manage the current initramfs */
struct initramfs_fs {
    struct initramfs_entry* entries;
    int entry_count;
    int max_entries;
};

int init_initramfs(void);

#endif