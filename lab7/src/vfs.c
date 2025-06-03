#include "vfs.h"
#include "string.h"
#include "malloc.h"
#include "tmpfs.h"
#include "sched.h"
#include "mm.h"

#define LOG_VFS 0
#if LOG_VFS
#include "muart.h"
#endif

// Root file system
struct mount* rootfs;

#define MAX_FILESYSTEMS 16
static struct filesystem* registered_fs[MAX_FILESYSTEMS];   // 儲存已註冊的 file systems (存 file system 的 address)
static int fs_count = 0;

/* Parsing the path components */
static int parse_path_component(const char* path, int start, char* component, int max_len) {
    if (!path || !component || max_len <= 0) {
        return VFS_EINVAL;  // Invalid argument
    }
    
    // Skip the first '/' characters for absolute path
    while (path[start] == '/') {
        start++;
    }
    
    // End of path
    if (path[start] == '\0') return VFS_ENDOFPATH;
    
    // Find the next '/' or end of string '\0
    int i = 0;
    while (path[start + i] != '\0' && path[start + i] != '/' && i < max_len - 1) {
        component[i] = path[start + i];
        i++;
    }
    component[i] = '\0';
    
    return start + i;
}

/* 
 * Since each file system has its own initialization method, the VFS should provide interface for each file system to register.
 * Register the file system to the kernel (you can also initialize memory pool of the file system here.) 
 */
// 把 file system 註冊到 VFS 的 array 中
int register_filesystem(struct filesystem* fs) {
    // Error check
    if( !fs || !fs->name || !fs->setup_mount || fs_count >= MAX_FILESYSTEMS ) {
        return VFS_EINVAL; 
    }

    // Check if filesystem already registered
    for (int i = 0; i < fs_count; i++) {
        if (strcmp(registered_fs[i]->name, fs->name) == 0) {
            return VFS_EEXIST;
        }
    }
    
    // Register the filesystem into the kernel
    registered_fs[fs_count++] = fs;
    return VFS_OK;
}

/* Find the filesystem in the registered file system array through the given file system's name */
static struct filesystem* find_filesystem(const char* name) {    
    for (int i = 0; i < fs_count; i++) {
        #if LOG_VFS
            muart_puts("[find_filesystem] Iterate the registered file system array : ");
            muart_puts(registered_fs[i]->name);
            muart_puts("\r\n");
        #endif
        if (strcmp(registered_fs[i]->name, name) == 0) {
            return registered_fs[i];
        }
    }
    return NULL;
}

/* Find the last slash '/' position in the path */
static int find_last_slash(const char* path) {
    int last_pos = -1;
    int i = 0;
    
    while (path[i] != '\0') {
        if (path[i] == '/') {
            last_pos = i;
        }
        i++;
    }
    
    return last_pos;
}

/* 
 * Open the file vnode, and create a file handle for the file 
 * Will create a new file handle for this vnode if found in vnode->f_ops->open()，在各自的 open method 中自己去 create file handle
 */
int vfs_open(const char* pathname, int flags, struct file** target) {
    if (!pathname || !target) {
        return VFS_EINVAL;  // Invalid argument
    }
    #if LOG_VFS
        muart_puts("[vfs_open] Opening file: ");
        muart_puts(pathname);
        muart_puts("\r\n");
    #endif

    struct vnode* vnode = NULL;
    // Lookup pathname
    int ret = vfs_lookup(pathname, &vnode); // Find the file vnode corresponding to the pathname
    
    // Create a new file if O_CREAT is specified in flags and vnode not found
    if ( (ret == VFS_ENOENT)  && (flags & O_CREAT) ) {
        int last_slash_pos = find_last_slash(pathname);
        char* file_name = pathname + last_slash_pos + 1;
    
        // Create the parent directory buffer
        char dir_pathname[256];
        if (last_slash_pos == 0) {  // root directory
            dir_pathname[0] = '/';
            dir_pathname[1] = '\0';
        }
        else {
            strncpy(dir_pathname, pathname, last_slash_pos);
            dir_pathname[last_slash_pos] = '\0'; 
            #if LOG_VFS
                muart_puts("[vfs_open] Parent directory pathname: ");
                muart_puts(dir_pathname);
                muart_puts("\r\n");
            #endif
        }

        // Lookup the parent directory vnode
        struct vnode* parent_vnode = NULL;
        ret = vfs_lookup(dir_pathname, &parent_vnode);
        if (ret != VFS_OK) {
            return ret;  // Return error code if parent directory not found
        }

        // Create the new file vnode in the parent directory
        ret = parent_vnode->v_ops->create(parent_vnode, &vnode, file_name);
        if (ret != VFS_OK) {
            return ret;  // Return error code if file creation fails
        }
    }
    
    // Open the file's vnode
    return vnode->f_ops->open(vnode, target, flags);
}

/* Close and release the file handle */
int vfs_close(struct file* file) {
    if (!file) {
        return VFS_EINVAL;
    }
    #if LOG_VFS
        muart_puts("[vfs_close] Closing file\r\n");
    #endif

    return file->f_ops->close(file);
}
/* Call the corresponding read method to read the file starting from f_pos, then updates f_pos after read. (or not if it’s a special file) */
int vfs_read(struct file* file, void* buf, size_t len) {
    // 1. read min(len, readable size) byte to buf from the opened file.
    // 2. block if nothing to read for FIFO type
    // 2. return read size or error code if an error occurs.
    if (!file || !buf) {
        return VFS_EINVAL;
    }
    #if LOG_VFS
        muart_puts("[vfs_read] Reading file\r\n");
    #endif

    return file->f_ops->read(file, buf, len);
}

/*  Call the corresponding write method to write the file starting from f_pos, then updates f_pos and size after write. (or not if it’s a special file) Returns size written or error code on error. */
int vfs_write(struct file* file, const void* buf, size_t len) {
    // 1. write len byte from buf to the opened file.
    // 2. return written size or error code if an error occurs.
    if (!file || !buf) {
        return VFS_EINVAL;
    }
    #if LOG_VFS
        muart_puts("[vfs_write] Writing file\r\n");
    #endif

    return file->f_ops->write(file, buf, len);
}

int vfs_mkdir(const char* pathname) {
    if (!pathname) {
        return VFS_EINVAL;
    }

    #if LOG_VFS
        muart_puts("[vfs_mkdir] Creating directory: ");
        muart_puts(pathname);
        muart_puts("\r\n"); 
    #endif 

    struct vnode* existing_vnode = NULL;
    if (vfs_lookup(pathname, &existing_vnode) == VFS_OK) {
        return VFS_EEXIST;  // Directory already exists
    }

    int last_slash_pos = find_last_slash(pathname);
    char* dir_name = pathname + last_slash_pos + 1;

    // Create the parent directory buffer
    char dir_pathname[256];
    if (last_slash_pos == 0) {  // root directory
        dir_pathname[0] = '/';
        dir_pathname[1] = '\0';
    }
    else {
        strncpy(dir_pathname, pathname, last_slash_pos);
        dir_pathname[last_slash_pos] = '\0'; 
    }

    // Lookup the parent directory vnode
    struct vnode* parent_vnode = NULL;
    int ret = vfs_lookup(dir_pathname, &parent_vnode);

    // Create directory vnode
    struct vnode* new_dir_vnode = NULL;
    ret = parent_vnode->v_ops->mkdir(parent_vnode, &new_dir_vnode, dir_name);
    if (ret != VFS_OK) {
        return ret;  // Return error code if mkdir fails
    }

    return VFS_OK;
}

/* Mounting a file system to a mount point */
int vfs_mount(const char* target, const char* filesystem) {
    if (!target || !filesystem) {
        return VFS_EINVAL;
    }

    #if LOG_VFS
        muart_puts("[vfs_mount] Mounting ");
        muart_puts(filesystem);
        muart_puts(" at ");
        muart_puts(target);
        muart_puts("\r\n");
    #endif

    // Find the filesystem in the registered file system array
    struct filesystem* fs = find_filesystem(filesystem);

    // Lookup the mounting point vnode
    struct vnode* mount_point = NULL;
    int ret = vfs_lookup(target, &mount_point);
    if ( ret != VFS_OK ) {
        return ret;  // Return error code if mount point not found
    }

    // Create a new mount structure to represent the mounted file system
    struct mount* mount_fs = dmalloc(sizeof(struct mount));

    // Set up the mount structure
    ret = fs->setup_mount(fs, mount_fs);
    if (ret != VFS_OK) {
        dfree(mount_fs);  
        return ret;
    }

    // Update the mount point vnode to point to the mounted file system
    mount_point->mount = mount_fs;

    return VFS_OK;
}

/*
 * File system iterates through directory entries and compares the component name to find the target file. 
 * Then, it passes the file’s vnode to the VFS if it finds the file. 
 * Parse the pathname and find the corresponding vnode, then return the vnode into target 
 * 找 VFS 對應 pathname 的 vnode
 */
int vfs_lookup(const char* pathname, struct vnode** target) {
    #if LOG_VFS
        muart_puts("[vfs_lookup] Looking up pathname: ");
        muart_puts(pathname);
        muart_puts("\r\n");
    #endif

    if( !pathname || !target ){
        return VFS_EINVAL;  // Invalid argument
    }

    // pathname is "/"
    if( strcmp(pathname, "/") == 0 ) {
        *target = rootfs->root;  // Return the root vnode of the root file system
        return VFS_OK;
    }

    // Start parsing the path from the root file system's root vnode
    struct vnode* current = rootfs->root;     
    char component[256];  // Buffer to hold the current path component
    int pos = 0;          // Start position in the pathname
    while ( ( pos = parse_path_component(pathname, pos, component, sizeof(component)) ) > 0 ) {
        struct vnode* next = NULL;
        int ret = current->v_ops->lookup(current, &next, component);
        #if LOG_VFS
            muart_puts("[vfs_lookup] Parsing component: ");
            muart_puts(component);
            muart_puts("\r\n");
            muart_puts("[vfs_lookup] Lookup result: ");
            muart_send_dec(ret);
            muart_puts("\r\n");
        #endif
        if (ret != VFS_OK) {
            return ret;  // Return error code if lookup fails
        }
        current = next;  // Move to the next vnode

        // If the current vnode is the mount point, cross to the mounted filesystem's root
        if (current && current->mount != NULL) {
            #if LOG_VFS
                muart_puts("[vfs_lookup] Found mount point, crossing to mounted filesystem\r\n");
                muart_puts("[vfs_lookup] Mount point vnode: ");
                muart_send_hex((unsigned long)current);
                muart_puts(", Mounted fs root: ");
                muart_send_hex((unsigned long)current->mount->root);
                muart_puts("\r\n"); 
            #endif
            
            // Cross to the mounted filesystem's root
            current = current->mount->root;
        }
    }

    // Return the found vnode
    *target = current;
    return VFS_OK;
}

long vfs_lseek64(struct file* file, long offset, int whence) {
    if (!file || !file->f_ops || !file->f_ops->lseek64) {
        return VFS_EINVAL; 
    }
    
    return file->f_ops->lseek64(file, offset, whence);
}

int setup_root_filesystem(void) {
    // 1. Register the tmpfs into kernel
    int ret = init_tmpfs();
    
    struct mount* root_mount = dmalloc(sizeof(struct mount));
    
    // 2. Directly call the tmpfs's setup_mount (因為 rootfs 還沒有 mount ，沒辦法用 vfs_lookup)
    struct filesystem* tmpfs_fs = find_filesystem("tmpfs");
    ret = tmpfs_fs->setup_mount(tmpfs_fs, root_mount);
    if( ret != VFS_OK ){
        #if LOG_VFS
            muart_puts("[setup_root_filesystem]Error setting up root filesystem: ");
            muart_send_dec(ret);
            muart_puts("\r\n");
        #endif
        dfree(root_mount);  // Free the mount structure if setup fails
        return ret;  // Return the error code
    }

    // 3. Set the mounted file system instance as the root file system
    rootfs = root_mount;

    #if LOG_VFS
        muart_puts("[setup_root_filesystem]Root filesystem `struct mount*` at address:");
        muart_send_hex((unsigned long)rootfs);
        muart_puts("\r\n");
    #endif

    return VFS_OK;
}

/* Task VFS initialization */
int vfs_task_init(struct task_struct* task) {
    // Initialize the file decriptor table
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        task->fd_table[i] = NULL;
    }

    strcpy(task->cwd, "/");  // Set the current working directory to root

    return VFS_OK;
}

/* Cleanup task VFS resources */
void vfs_cleanup_task(struct task_struct* task) {
    if (!task) {
        return;
    }
    
    // Close all open files
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (task->fd_table[i]) {
            vfs_close(task->fd_table[i]);
            task->fd_table[i] = NULL;
        }
    }
}

/* Allocate a free file descriptor */
int allocate_fd(struct task_struct* task) {
    if (!task) {
        return VFS_EINVAL;
    }
    
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (task->fd_table[i] == NULL) {
            return i;
        }
    }
    
    return -1; // No free fd available
}

void get_abs_path(char *path, char *cwd) {
    #if LOG_VFS
        muart_puts("[get_abs_path] Input path name: ");
        muart_puts(path);
        muart_puts("\r\n");
        muart_puts("[get_abs_path] Current working directory: ");
        muart_puts(cwd);
        muart_puts("\r\n");
    #endif
    // Concate the cwd and path if the path is not absolute
    if (path[0] != '/') { 
        char tmp[MAX_PATH_LENGTH];
        strcpy(tmp, cwd);
        if (strcmp(cwd, "/") != 0) {
            strcat(tmp, "/");
        }
        strcat(tmp, path);
        strcpy(path, tmp);
    }
    #if LOG_VFS
        muart_puts("[get_abs_path] After concatenation: ");
        muart_puts(path);
        muart_puts("\r\n");
    #endif

    char abs_path[MAX_PATH_LENGTH + 1];
    memzero(abs_path, sizeof(abs_path));
    int idx = 0;
    for (int i = 0; i < strlen(path); i++) {
        if (path[i] == '/' && path[i + 1] == '.' && path[i + 2] == '.') {
            for (int j = idx; j >= 0; j--) {
                if (abs_path[j] == '/') {
                    abs_path[j] = 0;
                    idx = j;
                }
            }
            i += 2;
            continue;
        }

        if (path[i] == '/' && path[i + 1] == '.') {
            i++;
            continue;
        }

        abs_path[idx++] = path[i];
    }
    abs_path[idx] = 0;
    strcpy(path, abs_path);
    #if LOG_VFS
        muart_puts("[get_abs_path] Final absolute path: ");
        muart_puts(path);
        muart_puts("\r\n");
    #endif 
}