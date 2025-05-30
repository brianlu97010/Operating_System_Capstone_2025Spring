#include "types.h"

struct vnode {
  struct mount* mount;          // 指向被掛載的 mounted file system
  struct vnode_operations* v_ops;
  struct file_operations* f_ops;
  void* internal;
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
  struct vnode* root;       // 指向這個被掛載後的 file system 的根目錄
  struct filesystem* fs;    // 這個 mounted filesystem instance 是哪一個 file system
};

struct filesystem {
  const char* name;
  int (*setup_mount)(struct filesystem* fs, struct mount* mount);
};

struct file_operations {
  int (*write)(struct file* file, const void* buf, size_t len);
  int (*read)(struct file* file, void* buf, size_t len);
  int (*open)(struct vnode* file_node, struct file** target);
  int (*close)(struct file* file);
//   long lseek64(struct file* file, long offset, int whence);
};

struct vnode_operations {
  int (*lookup)(struct vnode* dir_node, struct vnode** target, const char* component_name);
  int (*create)(struct vnode* dir_node, struct vnode** target, const char* component_name);
  int (*mkdir)(struct vnode* dir_node, struct vnode** target, const char* component_name);
};

struct mount* rootfs;
    
int register_filesystem(struct filesystem* fs) {
  // register the file system to the kernel.
  // you can also initialize memory pool of the file system here.
}

int vfs_open(const char* pathname, int flags, struct file** target) {
  // 1. Lookup pathname
  // 2. Create a new file handle for this vnode if found.
  // 3. Create a new file if O_CREAT is specified in flags and vnode not found
  // lookup error code shows if file exist or not or other error occurs
  // 4. Return error code if fails
}

int vfs_close(struct file* file) {
  // 1. release the file handle
  // 2. Return error code if fails
}

int vfs_write(struct file* file, const void* buf, size_t len) {
  // 1. write len byte from buf to the opened file.
  // 2. return written size or error code if an error occurs.
}

int vfs_read(struct file* file, void* buf, size_t len) {
  // 1. read min(len, readable size) byte to buf from the opened file.
  // 2. block if nothing to read for FIFO type
  // 2. return read size or error code if an error occurs.
}

int vfs_mkdir(const char* pathname);
int vfs_mount(const char* target, const char* filesystem);
int vfs_lookup(const char* pathname, struct vnode** target);