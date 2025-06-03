#include "initramfs.h"
#include "vfs.h"
#include "cpio.h"
#include "string.h"
#include "malloc.h"
#include "muart.h"
#include "mm.h"

#define LOG_INITRAMFS 0

/* Forward declarations */
static int initramfs_setup_mount(struct filesystem* fs, struct mount* mount);
static int initramfs_open(struct vnode* file_node, struct file** target, int flags);
static int initramfs_close(struct file* file);
static int initramfs_read(struct file* file, void* buf, size_t len);
static int initramfs_write(struct file* file, const void* buf, size_t len);
static int initramfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name);
static int initramfs_create(struct vnode* dir_node, struct vnode** target,  const char* component_name);
static int initramfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name);
static long initramfs_lseek64(struct file* file, long offset, int whence);

/* Initramfs filesystem registration */
static struct filesystem initramfs_filesystem = {
    .name = "initramfs",
    .setup_mount = initramfs_setup_mount,
};

/* Initramfs file operations */
static struct file_operations initramfs_file_ops = {
    .open = initramfs_open,
    .close = initramfs_close,
    .read = initramfs_read,
    .write = initramfs_write,    // Will always fail (read-only)
    .lseek64 = initramfs_lseek64
};

/* Initramfs vnode operations */
static struct vnode_operations initramfs_vnode_ops = {
    .lookup = initramfs_lookup,
    .create = initramfs_create,  // Will always fail (read-only)
    .mkdir = initramfs_mkdir     // Will always fail (read-only)
};

static struct initramfs_fs* g_initramfs = NULL;

/* Create the vnode for initramfs entry */
static struct vnode* initramfs_create_vnode(struct initramfs_entry* entry) {
    struct vnode* vnode = (struct vnode*)dmalloc(sizeof(struct vnode));
    if (!vnode) {
        muart_puts("[initramfs] ERROR: Failed to allocate vnode\r\n");
        return NULL;
    }
    
    vnode->mount = NULL;
    vnode->v_ops = &initramfs_vnode_ops;
    vnode->f_ops = &initramfs_file_ops;
    vnode->internal = entry;
    
    return vnode;
}

/* Parse CPIO archive and build file list in initramfs file system */
static int parse_cpio_to_entries(void) {
    const void* cpio_addr = get_cpio_addr();
    if (!cpio_addr) {
        #if LOG_INITRAMFS
            muart_puts("[initramfs] ERROR: No CPIO address set\r\n");
        #endif
        return VFS_ERROR;
    }
    #if LOG_INITRAMFS
        muart_puts("[initramfs] Parsing CPIO archive at: ");
        muart_send_hex((unsigned long)cpio_addr);
        muart_puts("\r\n");
    #endif 

    // First pass: count entries
    const char* current_addr = (const char*)cpio_addr;
    int entry_count = 0;
    
    cpio_newc_header* header;

    // Calculate the total entry count in the CPIO archive
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
        
        // Skip "." entry
        if (strcmp(pathname, ".") != 0) {
            entry_count++;
        }       

        #if LOG_INITRAMFS
            // Print the filename
            muart_puts(pathname);
            muart_puts("\r\n");
        #endif 

        // Move to the next entry
        // 1. 計算 data 起始位置（相對於 header 起始位置對齊）
        unsigned long data_start = (unsigned long)current_addr + sizeof(cpio_newc_header) + pathname_size;
        data_start = cpio_padded_size(data_start);  // 4-byte align
        
        // 2. 計算下一個 header 位置（相對於 data 起始位置對齊）
        unsigned long next_header = data_start + filedata_size;
        next_header = cpio_padded_size(next_header); // 4-byte align

        current_addr = (const char*)next_header;   
    }
    
    #if LOG_INITRAMFS
        muart_puts("[initramfs] Found ");
        muart_send_dec(entry_count);
        muart_puts(" entries\r\n");
    #endif
    
    // Allocate entries array according the above entry_count and set up initramfs
    g_initramfs->entries = (struct initramfs_entry*)dmalloc(sizeof(struct initramfs_entry)* entry_count);    
    g_initramfs->max_entries = entry_count;
    g_initramfs->entry_count = 0;
    
    // Fill the entries array with the data in CPIO archive
    current_addr = (const char*)cpio_addr;
    
    while (1) {
        header = (cpio_newc_header*)current_addr; 

        // Get the pathname size from the header and convert it to unsigned int
        unsigned int pathname_size = cpio_hex_to_int(header->c_namesize, 8);

        // Get the filedata size from the header and convert it to unsigned int
        unsigned int filedata_size = cpio_hex_to_int(header->c_filesize, 8);

        // Get the pathname which is followed by the header
        const char *pathname = current_addr + sizeof(cpio_newc_header);
        
        if (strcmp(pathname, CPIO_TRAILER) == 0) {
            break;
        }
        
        // Skip "." entry
        if (strcmp(pathname, ".") == 0) {
            goto next_entry;
        }
        
        // Allocate and copy filename
        struct initramfs_entry* entry = &g_initramfs->entries[g_initramfs->entry_count];
        entry->name = (char*)dmalloc(strlen(pathname) + 1);
        if (!entry->name) {
            return VFS_ENOMEM;
        }
        strcpy(entry->name, pathname);
        
        entry->size = filedata_size;
        
        // Calculate data start position
        unsigned long data_start = (unsigned long)current_addr + sizeof(cpio_newc_header) + pathname_size;
        data_start = cpio_padded_size(data_start);
        
        // Point directly to CPIO data (read-only)
        entry->data = (char*)data_start;
        
        // Create associated vnode for this entry
        entry->vnode = initramfs_create_vnode(entry);

        #if LOG_INITRAMFS
            muart_puts("[initramfs] Added: ");
            muart_puts(entry->name);
            muart_puts(", size: ");
            muart_send_dec(entry->size);
            muart_puts(")\r\n");
        #endif 

        g_initramfs->entry_count++;
        
    next_entry:
        // Move to next entry
        data_start = (unsigned long)current_addr + sizeof(cpio_newc_header) + pathname_size;
        data_start = cpio_padded_size(data_start);
        unsigned long next_header = data_start + filedata_size;
        next_header = cpio_padded_size(next_header);
        current_addr = (const char*)next_header;
    }
    
    return VFS_OK;
}


/* Initramfs vnode lookup */
static int initramfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    #if LOG_INITRAMFS
        muart_puts("[initramfs_lookup] Looking for: ");
        muart_puts(component_name);
        muart_puts("\r\n");
    #endif 
    
    if (!dir_node || !target || !component_name) {
        return VFS_EINVAL;
    }
    
    // For root directory lookup, search in all entries
    for (int i = 0; i < g_initramfs->entry_count; i++) {
        if (strcmp(g_initramfs->entries[i].name, component_name) == 0) {
            struct initramfs_entry* entry = &g_initramfs->entries[i];
            *target = entry->vnode;
            #if LOG_INITRAMFS
                muart_puts("[initramfs_lookup] Found: ");
                muart_puts(component_name);
                muart_puts(" (reusing vnode: ");
                muart_send_hex((unsigned long)entry->vnode);
                muart_puts(")\r\n");
            #endif 
            return VFS_OK;
        }
    }
    // If not found, return error code
    #if LOG_INITRAMFS
        muart_puts("[initramfs_lookup] Not found: ");
        muart_puts(component_name);
        muart_puts("\r\n");
    #endif
    return VFS_ENOENT;
}

/* Initramfs create (always fails - read-only) */
static int initramfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    return VFS_ERROR;  // Read-only filesystem
}

/* Initramfs mkdir (always fails - read-only) */
static int initramfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    return VFS_ERROR;  // Read-only filesystem
}

/* Initramfs file open */
static int initramfs_open(struct vnode* file_node, struct file** target, int flags) {
    if (!file_node || !target) {
        return VFS_EINVAL;
    }
    
    // Create file handle
    struct file* file = (struct file*)dmalloc(sizeof(struct file));
    if (!file) {
        return VFS_ENOMEM;
    }
    
    file->vnode = file_node;
    file->f_pos = 0;
    file->f_ops = &initramfs_file_ops;
    file->flags = flags;
    
    *target = file;
    
    return VFS_OK;
}

/* Initramfs file close */
static int initramfs_close(struct file* file) {
    if (!file) {
        return VFS_EINVAL;
    }
    
    dfree(file);
    return VFS_OK;
}

/* Initramfs file read */
static int initramfs_read(struct file* file, void* buf, size_t len) {
    if (!file || !buf) {
        return VFS_EINVAL;
    }
    
    struct initramfs_entry* entry = (struct initramfs_entry*)file->vnode->internal;

    size_t available = entry->size - file->f_pos;
    if (available == 0) {
        return 0;  // EOF
    }

    size_t to_read = (len < available) ? len : available;

    memcpy(buf, entry->data + file->f_pos, to_read);
    file->f_pos += to_read;

    return to_read;
}

/* Initramfs file write (always fails - read-only) */
static int initramfs_write(struct file* file, const void* buf, size_t len) {
    return VFS_ERROR;  // Read-only filesystem
}

/* Initramfs file seek */
static long initramfs_lseek64(struct file* file, long offset, int whence) {
    if (!file) {
        return VFS_EINVAL;
    }
    
    struct initramfs_entry* entry = (struct initramfs_entry*)file->vnode->internal;
    if (!entry) {
        return VFS_EINVAL;
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
            new_pos = entry->size + offset;
            break;
        default:
            return VFS_EINVAL;
    }
    
    if (new_pos < 0) {
        new_pos = 0;
    } else if (new_pos > entry->size) {
        new_pos = entry->size;
    }
    
    file->f_pos = new_pos;
    return new_pos;
}

/* Setup initramfs mount */
static int initramfs_setup_mount(struct filesystem* fs, struct mount* mount) {
    #if LOG_INITRAMFS
        muart_puts("[initramfs] Setting up mount\r\n");
    #endif
    // Initialize filesystem structure if not already done
    if (!g_initramfs) {
        g_initramfs = (struct initramfs_fs*)dmalloc(sizeof(struct initramfs_fs));
        if (!g_initramfs) {
            return VFS_ENOMEM;
        }
        
        memzero(g_initramfs, sizeof(struct initramfs_fs));
        
        // Parse CPIO archive
        int ret = parse_cpio_to_entries();
        if (ret != VFS_OK) {
            dfree(g_initramfs);
            g_initramfs = NULL;
            return ret;
        }
    }
    
    // Create root vnode for this mount
    struct vnode* root_vnode = (struct vnode*)dmalloc(sizeof(struct vnode));
    if (!root_vnode) {
        return VFS_ENOMEM;
    }
    
    root_vnode->mount = NULL;
    root_vnode->v_ops = &initramfs_vnode_ops;
    root_vnode->f_ops = &initramfs_file_ops;
    root_vnode->internal = NULL;
    
    mount->root = root_vnode;
    mount->fs = fs;
    #if LOG_INITRAMFS
        muart_puts("[initramfs] Mount setup completed\r\n");
    #endif
    return VFS_OK;
}

/* Initialize initramfs filesystem */
int init_initramfs(void) {    
    #if LOG_INITRAMFS
    muart_puts("[initramfs] Initializing initramfs filesystem\r\n");
    #endif
    // Register initramfs to kernel
    int ret = register_filesystem(&initramfs_filesystem);
    if (ret != VFS_OK) {
        return ret;
    }
    
    #if LOG_INITRAMFS
    // Mount initramfs
    muart_puts("[initramfs] Mounting initramfs at /initramfs\r\n");
    #endif
    // Ccreate the mount point directory "/initramfs"
    ret = vfs_mkdir("/initramfs");
    if (ret != VFS_OK && ret != VFS_EEXIST) {
        return ret;
    }
    
    // Mount initramfs
    ret = vfs_mount("/initramfs", "initramfs");
    if (ret != VFS_OK) {
        return ret;
    }

    #if LOG_INITRAMFS
        muart_puts("[initramfs] Successfully mounted at /initramfs\r\n");
    #endif
    return VFS_OK;
}