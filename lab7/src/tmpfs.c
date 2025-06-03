#include "tmpfs.h"
#include "types.h"
#include "vfs.h"
#include "malloc.h"
#include "mm.h"
#include "string.h"

#define LOG_TMPFS 0
#if LOG_TMPFS
#include "muart.h"
#endif

/* Static Function Declaration */
static int tmpfs_open(struct vnode* file_node, struct file** target, int flags);
static int tmpfs_close(struct file* file);
static int tmpfs_read(struct file* file, void* buf, size_t len);
static int tmpfs_write(struct file* file, const void* buf, size_t len);
static int tmpfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name);
static int tmpfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name);
static int tmpfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name);
static long tmpfs_lseek64(struct file* file, long offset, int whence);

static struct filesystem tmpfs_filesystem = {
    .name = "tmpfs",
    .setup_mount = tmpfs_setup_mount
};

static struct file_operations tmpfs_file_ops = {
    .open = tmpfs_open,
    .close = tmpfs_close,
    .read = tmpfs_read,
    .write = tmpfs_write,
    .lseek64 = tmpfs_lseek64
};

static struct vnode_operations tmpfs_vnode_ops = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create,  
    .mkdir = tmpfs_mkdir
};

// Create tmpfs's inode ( internal representation of tmpfs )
static struct tmpfs_inode* tmpfs_create_inode(int type) {
    struct tmpfs_inode* inode = dmalloc(sizeof(struct tmpfs_inode));
    if (!inode) {
        return NULL;
    }
    
    // Initialize the inode
    memzero((unsigned long)inode, sizeof(struct tmpfs_inode));
    inode->type = type;
    
    return inode;
}

/* Free the memory space of inode */
static void tmpfs_destroy_inode(struct tmpfs_inode* inode) {
    if (inode) {
        dfree(inode);
    }
}

// Create tmpfs's vnode in VFS
static struct vnode* tmpfs_create_vnode(int type) {
    #if LOG_TMPFS
        muart_puts("[tmpfs_create_vnode] Creating vnode of type: ");
        if (type == TMPFS_TYPE_FILE) {
            muart_puts("File\r\n");
        } else if (type == TMPFS_TYPE_DIR) {
            muart_puts("Directory\r\n");
        } else {
            muart_puts("Unknown\r\n");
        }
    #endif

    // Create the internal representation (inode) of tmpfs 
    struct tmpfs_inode* inode = tmpfs_create_inode(type);
    if (!inode) {
        return NULL;
    }
    
    struct vnode* vnode = dmalloc(sizeof(struct vnode));
    if (!vnode) {
        tmpfs_destroy_inode(inode);
        return NULL;
    }
    
    vnode->mount = NULL;
    vnode->v_ops = &tmpfs_vnode_ops;
    vnode->f_ops = &tmpfs_file_ops;
    vnode->internal = inode;    
    
    inode->vnode = vnode;
    
    return vnode;
}

// Register the tmpfs_filesystem into the VFS (registered_fs array)
int init_tmpfs(void) {
    return register_filesystem(&tmpfs_filesystem);
}

/* 
 * Each mounted file system has its own root vnode. You should create the root vnode during the mount setup. 
 * Create a root vnode for the tmpfs and set it as the root of the mounted file system instance
 */
int tmpfs_setup_mount(struct filesystem* fs, struct mount* mount) {
    // Create the root vnode for the mounted tmpfs filesystem
    struct vnode* tmpfs_root_vnode = tmpfs_create_vnode(TMPFS_TYPE_DIR);
    if (!tmpfs_root_vnode) {
        return VFS_ENOMEM;
    }
    
    mount->root = tmpfs_root_vnode;  // set the root vnode of the mounted file system instance
    mount->fs = fs;                  // set the filesystem of the mounted file system instance
    
    return VFS_OK;
}

static int tmpfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    #if LOG_TMPFS
        muart_puts("[tmpfs_lookup] Looking up component: ");
        muart_puts(component_name);
        muart_puts("\r\n");
    #endif
    if (!dir_node || !target || !component_name) {
        return VFS_EINVAL;
    }
    
    struct tmpfs_inode* dir_inode = (struct tmpfs_inode*)dir_node->internal;
    if (!dir_inode || dir_inode->type != TMPFS_TYPE_DIR) {
        return VFS_ENOENT;
    }
    
    // Search for the component in directory entries
    for (int i = 0; i < TMPFS_MAX_ENTRIES; i++) {
        struct tmpfs_dirent* entry = &dir_inode->tmpfs_dir.entries[i];
        if (entry->used && strcmp(entry->name, component_name) == 0) {
            *target = entry->inode->vnode;
            return VFS_OK;
        }
    }
    
    return VFS_ENOENT;  // No such file or directory
}

/* Create a new file in parent directory */
static int tmpfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    #if LOG_TMPFS
        muart_puts("[tmpfs_create] Creating file: ");
        muart_puts(component_name);
        muart_puts("\r\n");
    #endif 
    if (!dir_node || !target || !component_name) {
        return VFS_EINVAL;
    }
    
    struct tmpfs_inode* dir_inode = (struct tmpfs_inode*)dir_node->internal;
    
    // Find free directory entry
    int free_slot = -1;
    for (int i = 0; i < TMPFS_MAX_ENTRIES; i++) {
        if (!dir_inode->tmpfs_dir.entries[i].used) {
            free_slot = i;
            break;
        }
    }
    
    // Create new file vnode
    struct vnode* new_vnode = tmpfs_create_vnode(TMPFS_TYPE_FILE);
    
    // Add to parent directory's entry
    struct tmpfs_dirent* entry = &dir_inode->tmpfs_dir.entries[free_slot];
    strncpy(entry->name, component_name, TMPFS_MAX_NAME);
    entry->inode = (struct tmpfs_inode*)new_vnode->internal;
    entry->used = 1;
    dir_inode->tmpfs_dir.entry_count++;
    
    *target = new_vnode;
    return VFS_OK;
}

// tmpfs file operations
static int tmpfs_open(struct vnode* file_node, struct file** target, int flags) {
    #if LOG_TMPFS
        muart_puts("[tmpfs_open] Opening file vnode with flags ");
        muart_send_dec(flags);
        muart_puts("\r\n");
        int O_TRUNC_set = flags & O_TRUNC;
        muart_puts("[O_TRUNC] is ");
        muart_puts(O_TRUNC_set ? "set" : "not set");
        muart_puts("\r\n");
    #endif

    if (!file_node || !target) {
        return VFS_EINVAL;
    }
    
    // Create file handle
    struct file* file = (struct file*)dmalloc(sizeof(struct file));
    
    // Initialize file handle
    file->vnode = file_node;
    file->f_pos = 0;                    // R/W position starts at 0
    file->f_ops = file_node->f_ops;     
    file->flags = flags;                
    

    // For basic 2 demo, when mounting tmpfs, it will reopen the existing file.
    if (flags == O_CREAT) {
        struct tmpfs_inode* inode = (struct tmpfs_inode*)file_node->internal;
        if (inode && inode->tmpfs_file.size > 0) {
            inode->tmpfs_file.size = 0;  // Reset file size
        }
    }

    *target = file;                    
    return VFS_OK;
}

static int tmpfs_close(struct file* file) {
    if (!file) {
        return VFS_EINVAL;
    }
    
    dfree(file);
    return VFS_OK;
}

static int tmpfs_read(struct file* file, void* buf, size_t len) {
    if (!file || !buf) {
        return VFS_EINVAL;
    }
    
    struct tmpfs_inode* inode = (struct tmpfs_inode*)file->vnode->internal;
    
    // Calculate how much we can actually read
    size_t available = inode->tmpfs_file.size - file->f_pos;
    if (available == 0) {
        return 0;  // EOF
    }

    // read min(len, readable size) byte to buf from the opened file.
    size_t to_read = (len < available) ? len : available;
    
    // Copy data from file to buffer
    memcpy(buf, &inode->tmpfs_file.data[file->f_pos], to_read);
    file->f_pos += to_read;
    
    #if LOG_TMPFS
        muart_puts("[tmpfs_read] Read ");
        muart_send_dec(to_read);
        muart_puts(" bytes from file\r\n");
    #endif
    return to_read; // return the number of bytes read
}

static int tmpfs_write(struct file* file, const void* buf, size_t len) {
    if (!file || !buf) {
        return VFS_EINVAL;
    }
    
    struct tmpfs_inode* inode = (struct tmpfs_inode*)file->vnode->internal;
    
    // Check if write would exceed file size limit, if exceed, only write up to TMPFS_MAX_FILE_SIZE
    if (file->f_pos + len > TMPFS_MAX_FILE_SIZE) {
        len = TMPFS_MAX_FILE_SIZE - file->f_pos;
    }
    
    // Copy data from buffer to file
    memcpy(&inode->tmpfs_file.data[file->f_pos], buf, len);
    file->f_pos += len;
    
    // Update file size
    if (file->f_pos > inode->tmpfs_file.size) {
        inode->tmpfs_file.size = file->f_pos;
    }

    #if LOG_TMPFS
        muart_puts("[tmpfs_write] Wrote ");
        muart_send_dec(len);
        muart_puts(" bytes to file\r\n");
    #endif
    
    return len;     // return the number of bytes written
}

static long tmpfs_lseek64(struct file* file, long offset, int whence) {
    #if LOG_TMPFS
        muart_puts("[tmpfs_lseek64] Seeking file position\r\n");
    #endif
    if (!file || !file->vnode) {
        return VFS_EINVAL;
    }
    
    struct tmpfs_inode* inode = (struct tmpfs_inode*)file->vnode->internal;
    if (!inode || inode->type != TMPFS_TYPE_FILE) {
        return VFS_ENOENT;
    }
    
    long new_pos;
    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = file->f_pos + offset;
            break;
        case SEEK_END:
            new_pos = inode->tmpfs_file.size + offset;
            break;
        default:
            return VFS_EINVAL;
    }
    
    // Ensure position is not negative
    if (new_pos < 0) {
        new_pos = 0;
    } else if (new_pos > inode->tmpfs_file.size) {
        new_pos = inode->tmpfs_file.size;
    }
    
    file->f_pos = new_pos;
    return new_pos;
}

/* Create directory in tmpfs */
static int tmpfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    if (!dir_node || !target || !component_name) {
        return VFS_EINVAL;
    }
    
    struct tmpfs_inode* dir_inode = (struct tmpfs_inode*)dir_node->internal;

    // Check if directory already exists
    struct vnode* existing = NULL;
    if (tmpfs_lookup(dir_node, &existing, component_name) == VFS_OK) {
        return VFS_EEXIST;  // Directory already exists
    }

    // Find free directory entry 
    int free_slot = -1;
    for (int i = 0; i < TMPFS_MAX_ENTRIES; i++) {
        if (!dir_inode->tmpfs_dir.entries[i].used) {
            free_slot = i;
            break;
        }
    }

    // Create new directory vnode
    struct vnode* new_dir_vnode = tmpfs_create_vnode(TMPFS_TYPE_DIR);

    // Add to parent directory
    struct tmpfs_dirent* entry = &dir_inode->tmpfs_dir.entries[free_slot];
    strncpy(entry->name, component_name, TMPFS_MAX_NAME);
    entry->name[TMPFS_MAX_NAME] = '\0';  // Ensure null termination
    entry->inode = (struct tmpfs_inode*)new_dir_vnode->internal;
    entry->used = 1;
    dir_inode->tmpfs_dir.entry_count++;
    
    #if LOG_TMPFS
        muart_puts("[tmpfs_mkdir] Directory created at slot: ");
        muart_send_dec(free_slot);
        muart_puts("\r\n");
    #endif
    
    *target = new_dir_vnode;
    return VFS_OK;
}