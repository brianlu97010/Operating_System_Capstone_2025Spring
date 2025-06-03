#ifndef _VFS_H
#define _VFS_H

#include "types.h"
#include "vfs.h"

struct vnode {
    struct mount* mount;
    struct vnode_operations* v_ops;
    struct file_operations* f_ops;
    void* internal;   // Each file system's vnode may differ, use the internal pointer to point to each file system's internal representation (e.g. for tmnpfs, it points to `struct tmpfs_inode`)
};

// file handle
struct file {
    struct vnode* vnode;
    size_t f_pos;  // RW position of this file handle
    struct file_operations* f_ops;
    int flags;
};

// Represent a mounted file system
struct mount {
    struct vnode* root;       // Each mounted file system has its own root vnode
    struct filesystem* fs;    // 這個 mounted filesystem instance 是哪一個 file system
};

struct filesystem {
    const char* name;
    int (*setup_mount)(struct filesystem* fs, struct mount* mount);   // Each file system has its own initialization method
};

struct file_operations {
    int (*open)(struct vnode* file_node, struct file** target, int flags);
    int (*close)(struct file* file);
    int (*read)(struct file* file, void* buf, size_t len);
    int (*write)(struct file* file, const void* buf, size_t len);
    long (*lseek64)(struct file* file, long offset, int whence);
};

struct vnode_operations {
    int (*lookup)(struct vnode* dir_node, struct vnode** target, const char* component_name);
    int (*create)(struct vnode* dir_node, struct vnode** target, const char* component_name);
    int (*mkdir)(struct vnode* dir_node, struct vnode** target, const char* component_name);
};

struct task_struct;

// Error codes
#define VFS_OK      0
#define VFS_ERROR   -1
#define VFS_ENOENT  -2  // No such file or directory
#define VFS_EEXIST  -3  // File exists
#define VFS_EINVAL  -4  // Invalid argument
#define VFS_ENOMEM  -5  // Out of memory
#define VFS_ENDOFPATH -6  // End of path reached

// File open flags
#define O_RDONLY    00000000    // Read only
#define O_WRONLY    00000001    // Write only  
#define O_RDWR      00000002    // Read and write
#define O_CREAT     00000100    // Create file if it doesn't exist
#define O_TRUNC     00000200    // Truncate file to zero length if it exists
#define O_APPEND    00000400    // Append to the end of the file

// Seek constants
#define SEEK_SET 0      // Seek from the beginning of the file
#define SEEK_CUR 1      // Seek from the current position
#define SEEK_END 2      // Seek from the end of the file

int register_filesystem(struct filesystem* fs);

int setup_root_filesystem(void);

int vfs_lookup(const char* pathname, struct vnode** target);
int vfs_open(const char* pathname, int flags, struct file** target);
int vfs_close(struct file* file);
int vfs_read(struct file* file, void* buf, size_t len);
int vfs_write(struct file* file, const void* buf, size_t len);
long vfs_lseek64(struct file* file, long offset, int whence);
int vfs_mkdir(const char* pathname);
int vfs_mount(const char* target, const char* filesystem);

/* Multi-Task VFS */
int vfs_task_init(struct task_struct* task);
void vfs_cleanup_task(struct task_struct* task);
int allocate_fd(struct task_struct* task);

void get_abs_path(char *path, char *cwd);

#endif