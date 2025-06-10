# OSC 2025 | Lab 7: Virtual File System
> [!IMPORTANT]
> **Todo** :  
> Advanced Exercises 1 - /dev/uart  
> Advanced Exercises 2 - /dev/framebuffer  

## Overview

這個 lab 實作了一個 Virtual File System (VFS) 介面以及記憶體檔案系統 (tmpfs)，讓 kernel 能夠提供統一的檔案系統介面。VFS 定義了在實體檔案系統上更高一層的 interface，讓 user application 可以透過 VFS 定義好的介面存取底層資料，不用考慮底層是如何實作。

### VFS 架構

```
┌─────────────────────────────────────────────────────────┐
│                 User Applications                       │
│  ( 呼叫 open, read, write, close 等 syscall)             │
└─────────────────┬───────────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────────┐
│                  VFS Layer                              │
│  統一的 API: vfs_open(), vfs_read(), vfs_write()...    │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐     │
│  │   vnode     │  │   vnode     │  │   vnode     │     │
│  │ (統一介面)  │  │ (統一介面)  │  │ (統一介面)  │     │
│  └─────────────┘  └─────────────┘  └─────────────┘     │
└─────────────────┬───────────────┬───────────────┬───────┘
                  │               │               │
┌─────────────────▼─┐  ┌─────────▼──┐  ┌─────────▼────────┐
│     tmpfs         │  │ initramfs  │  │  Device Files    │
│  (記憶體檔案系統) │  │ (唯讀)     │  │ (UART/FB)        │
│                   │  │            │  │                  │
│ tmpfs_read()      │  │ init_read()│  │ uart_read()      │
│ tmpfs_write()     │  │ init_write()│  │ uart_write()     │
│ tmpfs_create()    │  │            │  │ fb_write()       │
└─────────────────┬─┘  └────────────┘  └──────────────────┘
                  │
┌─────────────────▼─┐
│       RAM         │
│  實際資料儲存     │
│  (動態記憶體分配) │
└───────────────────┘
```

## Data Structure of VFS

### vnode
每個 file、directory、device 在 VFS 中都用一個 vnode 來代表，提供統一的 interface：

```c
struct vnode {
    struct mount* mount;                    
    struct vnode_operations* v_ops;         // 跟 vnode 操作有關的 functions (lookup, mkdir, create)
    struct file_operations* f_ops;          // 跟 file 操作有關的 functions (open, close, read, write, lseek)
    void* internal;                         // 指向具體 file system 的內部資料結構
};
```

### file handle
同一個 file 可能被多個 user application 同時開啟，需要有個 data structure 叫做 file handle 來記錄目前的狀態：

```c
struct file {
    struct vnode *vnode;                    // 這個 file handle 實際是紀錄哪一個 file
    size_t f_pos;                          // 目前的 read/write position
    struct file_operations *f_ops;         // file operation
    int flags;                             // 開啟模式（ read/write/create 等）
};
```

### mount
代表一個已經 mounted 的 file system：

```c
struct mount {
    struct vnode* root;                     // Each mounted file system has its own root vnode
    struct filesystem* fs;                  // 這個 mounted filesystem instance 是哪一個 file system
};
```

## Basic Exercise 1 - Root File System

### 目標
實作 tmpfs 作為 root file system，提供基本的檔案操作功能。

### 實作重點

#### 1. File System Registration
```c
static struct filesystem tmpfs_filesystem = {
    .name = "tmpfs",
    .setup_mount = tmpfs_setup_mount
};

int init_tmpfs(void) {
    return register_filesystem(&tmpfs_filesystem);
}
```

每個 file system 都要註冊到 VFS 的 `registered_fs` array 中：

```c
static struct filesystem* registered_fs[MAX_FILESYSTEMS];
static int fs_count = 0;

int register_filesystem(struct filesystem* fs) {
    // 檢查是否重複註冊
    for (int i = 0; i < fs_count; i++) {
        if (strcmp(registered_fs[i]->name, fs->name) == 0) {
            return VFS_EEXIST;
        }
    }
    
    // 註冊到 kernel
    registered_fs[fs_count++] = fs;
    return VFS_OK;
}
```

#### 2. Root File System Setup
```c
int setup_root_filesystem(void) {
    // 1. 註冊 tmpfs 到 kernel
    int ret = init_tmpfs();
    
    struct mount* root_mount = dmalloc(sizeof(struct mount));
    
    // 2. 直接呼叫 tmpfs 的 setup_mount
    struct filesystem* tmpfs_fs = find_filesystem("tmpfs");
    ret = tmpfs_fs->setup_mount(tmpfs_fs, root_mount);
    
    // 3. 設為 global 的 root file system
    rootfs = root_mount;
    
    return VFS_OK;
}
```

#### 3. tmpfs 內部資料結構
```c
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
```

#### 4. 核心操作實作

**File Open:**
```c
static int tmpfs_open(struct vnode* file_node, struct file** target, int flags) {
    // Create file handle
    struct file* file = (struct file*)dmalloc(sizeof(struct file));
    
    // Initialize file handle
    file->vnode = file_node;
    file->f_pos = 0;
    file->f_ops = file_node->f_ops;
    file->flags = flags;
    
    *target = file;
    return VFS_OK;
}
```

**File Read/Write:**
```c
static int tmpfs_read(struct file* file, void* buf, size_t len) {
    struct tmpfs_inode* inode = (struct tmpfs_inode*)file->vnode->internal;
    
    size_t available = inode->tmpfs_file.size - file->f_pos;
    size_t to_read = (len < available) ? len : available;
    
    memcpy(buf, &inode->tmpfs_file.data[file->f_pos], to_read);
    file->f_pos += to_read;
    
    return to_read;
}

static int tmpfs_write(struct file* file, const void* buf, size_t len) {
    struct tmpfs_inode* inode = (struct tmpfs_inode*)file->vnode->internal;
    
    if (file->f_pos + len > TMPFS_MAX_FILE_SIZE) {
        len = TMPFS_MAX_FILE_SIZE - file->f_pos;
    }
    
    memcpy(&inode->tmpfs_file.data[file->f_pos], buf, len);
    file->f_pos += len;
    
    if (file->f_pos > inode->tmpfs_file.size) {
        inode->tmpfs_file.size = file->f_pos;
    }
    
    return len;
}
```

## Basic Exercise 2 - Multi-level VFS

### 目標
實作多層級目錄結構和檔案系統掛載功能。

### 實作重點

#### 1. Directory Creation
```c
static int tmpfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    struct tmpfs_inode* dir_inode = (struct tmpfs_inode*)dir_node->internal;

    // 找到空的 directory entry
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
    entry->inode = (struct tmpfs_inode*)new_dir_vnode->internal;
    entry->used = 1;
    dir_inode->tmpfs_dir.entry_count++;
    
    *target = new_dir_vnode;
    return VFS_OK;
}
```

#### 2. Pathname Lookup
核心的路徑解析功能，支援跨檔案系統的查找：

```c
int vfs_lookup(const char* pathname, struct vnode** target) {
    // 從 root file system 的 root vnode 開始
    struct vnode* current = rootfs->root;     
    char component[256];
    int pos = 0;
    
    while ((pos = parse_path_component(pathname, pos, component, sizeof(component))) > 0) {
        struct vnode* next = NULL;
        int ret = current->v_ops->lookup(current, &next, component);
        if (ret != VFS_OK) {
            return ret;
        }
        current = next;

        // 如果遇到 mounting point，跳躍到被掛載的檔案系統
        if (current && current->mount != NULL) {
            current = current->mount->root;
        }
    }

    *target = current;
    return VFS_OK;
}
```

#### 3. File System Mounting
```c
int vfs_mount(const char* target, const char* filesystem) {
    // 找到要掛載的檔案系統
    struct filesystem* fs = find_filesystem(filesystem);

    // 找到掛載點 vnode
    struct vnode* mount_point = NULL;
    int ret = vfs_lookup(target, &mount_point);

    // 建立 mount structure
    struct mount* mount_fs = dmalloc(sizeof(struct mount));
    ret = fs->setup_mount(fs, mount_fs);

    // 更新掛載點 vnode
    mount_point->mount = mount_fs;

    return VFS_OK;
}
```

#### 4. File Creation with Path
```c
int vfs_open(const char* pathname, int flags, struct file** target) {
    struct vnode* vnode = NULL;
    int ret = vfs_lookup(pathname, &vnode);
    
    // 如果檔案不存在且要求創建
    if ((ret == VFS_ENOENT) && (flags & O_CREAT)) {
        int last_slash_pos = find_last_slash(pathname);
        char* file_name = pathname + last_slash_pos + 1;
    
        // 建立父目錄路徑
        char dir_pathname[256];
        if (last_slash_pos == 0) {
            strcpy(dir_pathname, "/");
        } else {
            strncpy(dir_pathname, pathname, last_slash_pos);
            dir_pathname[last_slash_pos] = '\0';
        }

        // 找到父目錄 vnode
        struct vnode* parent_vnode = NULL;
        ret = vfs_lookup(dir_pathname, &parent_vnode);

        // 在父目錄中創建檔案
        ret = parent_vnode->v_ops->create(parent_vnode, &vnode, file_name);
    }
    
    return vnode->f_ops->open(vnode, target, flags);
}
```

## Basic Exercise 3 - Multitask VFS

### 目標
為每個 task 提供獨立的檔案描述符表和當前工作目錄。

### 實作重點

#### 1. Task File Descriptor Table
```c
struct task_struct {
    // ... 其他成員
    char cwd[MAX_PATH_LENGTH];                      // Current working directory
    struct file* fd_table[MAX_OPEN_FILES];          // File descriptor table
};
```

#### 2. VFS Task Initialization
```c
int vfs_task_init(struct task_struct* task) {
    // 初始化檔案描述符表
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        task->fd_table[i] = NULL;
    }

    strcpy(task->cwd, "/");  // 設定當前工作目錄為 root
    return VFS_OK;
}
```

#### 3. System Call Implementation

**open system call:**
```c
int open(const char *pathname, int flags) {
    struct task_struct* current = (struct task_struct*)get_current_thread();
    
    // 計算絕對路徑
    char abs_path[MAX_PATH_LENGTH];
    strcpy(abs_path, pathname);
    get_abs_path(abs_path, current->cwd);

    // 分配檔案描述符
    int fd = allocate_fd(current);

    // 呼叫 vfs_open 開啟檔案
    struct file* file;
    int ret = vfs_open(abs_path, flags, &file);
    if (ret != VFS_OK) {
        return ret;
    }

    // 儲存到檔案描述符表
    current->fd_table[fd] = file;
    return fd;
}
```

**read/write system calls:**
```c
long read(int fd, void *buf, unsigned long count) {
    struct task_struct* current = (struct task_struct*)get_current_thread();
    struct file* file = current->fd_table[fd];
    return vfs_read(file, buf, count);
}

long write(int fd, const void *buf, unsigned long count) {
    struct task_struct* current = (struct task_struct*)get_current_thread();
    struct file* file = current->fd_table[fd];
    return vfs_write(file, buf, count);
}
```

#### 4. Path Resolution
支援相對路徑和絕對路徑：

```c
void get_abs_path(char *path, char *cwd) {
    // 如果不是絕對路徑，與當前工作目錄結合
    if (path[0] != '/') { 
        char tmp[MAX_PATH_LENGTH];
        strcpy(tmp, cwd);
        if (strcmp(cwd, "/") != 0) {
            strcat(tmp, "/");
        }
        strcat(tmp, path);
        strcpy(path, tmp);
    }
    
    // 處理 ".." 和 "." 
    char abs_path[MAX_PATH_LENGTH + 1];
    memzero(abs_path, sizeof(abs_path));
    int idx = 0;
    
    for (int i = 0; i < strlen(path); i++) {
        if (path[i] == '/' && path[i + 1] == '.' && path[i + 2] == '.') {
            // 回到上一層目錄
            for (int j = idx; j >= 0; j--) {
                if (abs_path[j] == '/') {
                    abs_path[j] = 0;
                    idx = j;
                    break;
                }
            }
            i += 2;
            continue;
        }

        if (path[i] == '/' && path[i + 1] == '.') {
            // 跳過當前目錄
            i++;
            continue;
        }

        abs_path[idx++] = path[i];
    }
    
    strcpy(path, abs_path);
}
```

#### 5. Change Directory
```c
int chdir(const char *path) {
    struct task_struct* current = (struct task_struct*)get_current_thread();
    
    char abs_path[MAX_PATH_LENGTH];
    strcpy(abs_path, path);
    get_abs_path(abs_path, current->cwd);
    strcpy(current->cwd, abs_path);
    
    return 0;
}
```

## Basic Exercise 4 - /initramfs

### 目標
實作 initramfs 唯讀檔案系統，並掛載到 `/initramfs`。

### 實作重點

#### 1. initramfs 資料結構
```c
struct initramfs_entry {
    char* name;                             // File name
    char* data;                            // File content (NULL for directories)
    size_t size;                           // File size
    int type;                              // INITRAMFS_TYPE_FILE or INITRAMFS_TYPE_DIR
    struct vnode* vnode;                   // Back reference to vnode
};

struct initramfs_fs {
    struct initramfs_entry* entries;
    int entry_count;
    int max_entries;
};
```

#### 2. CPIO Archive Parsing
```c
static int parse_cpio_to_entries(void) {
    const void* cpio_addr = get_cpio_addr();
    const char* current_addr = (const char*)cpio_addr;
    
    // 第一遍：計算檔案數量
    int entry_count = 0;
    while(1) {
        cpio_newc_header* header = (cpio_newc_header*)current_addr;
        unsigned int pathname_size = cpio_hex_to_int(header->c_namesize, 8);
        unsigned int filedata_size = cpio_hex_to_int(header->c_filesize, 8);
        const char *pathname = current_addr + sizeof(cpio_newc_header);

        if (strcmp(pathname, CPIO_TRAILER) == 0) {
            break;
        }
        
        if (strcmp(pathname, ".") != 0) {
            entry_count++;
        }

        // Move to next entry
        unsigned long data_start = (unsigned long)current_addr + sizeof(cpio_newc_header) + pathname_size;
        data_start = cpio_padded_size(data_start);
        unsigned long next_header = data_start + filedata_size;
        next_header = cpio_padded_size(next_header);
        current_addr = (const char*)next_header;
    }
    
    // 分配 entries array
    g_initramfs->entries = (struct initramfs_entry*)dmalloc(sizeof(struct initramfs_entry) * entry_count);
    g_initramfs->max_entries = entry_count;
    
    // 第二遍：填入資料
    // ... 解析並建立 initramfs_entry
}
```

#### 3. Read-only Operations
```c
// initramfs 的所有寫入操作都會失敗
static int initramfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    return VFS_ERROR;  // Read-only filesystem
}

static int initramfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    return VFS_ERROR;  // Read-only filesystem
}

static int initramfs_write(struct file* file, const void* buf, size_t len) {
    return VFS_ERROR;  // Read-only filesystem
}
```

#### 4. initramfs Mounting
```c
int init_initramfs(void) {    
    // 註冊 initramfs 到 kernel
    int ret = register_filesystem(&initramfs_filesystem);
    
    // 建立掛載點目錄 "/initramfs"
    ret = vfs_mkdir("/initramfs");
    
    // 掛載 initramfs
    ret = vfs_mount("/initramfs", "initramfs");
    
    return VFS_OK;
}
```

#### 5. File Access via VFS
掛載完成後，可以透過 VFS 介面存取 initramfs 中的檔案：

```c
// 透過 exec system call 執行 initramfs 中的程式
int sys_exec(const char* name, char *const argv[]) {
    struct task_struct* current = (struct task_struct*)get_current_thread();
    struct file* program_file = NULL;
    
    // 使用 VFS 開啟程式檔案
    ret = vfs_open(name, O_RDONLY, &program_file);
    
    // 取得檔案大小
    long file_size = vfs_lseek64(program_file, 0, SEEK_END);
    vfs_lseek64(program_file, 0, SEEK_SET);
    
    // 分配記憶體並讀取程式
    current->user_program = dmalloc(file_size);
    long bytes_read = vfs_read(program_file, current->user_program, file_size);
    
    vfs_close(program_file);
    
    // 設定 trap frame 執行新程式
    struct trap_frame* regs = task_tf(current);
    regs->elr_el1 = (unsigned long)current->user_program;
    regs->sp_el0 = (unsigned long)current->user_stack + THREAD_STACK_SIZE;
    
    return 0;
}
```


### 遇到 mounting point ，跳到 mounted file system 的 root
```
路徑: "/initramfs/config.txt"

1. 從 tmpfs 根目錄開始
2. 查找 "initramfs" → 找到 tmpfs 中的目錄
3. 檢查該目錄是否為掛載點 → 是！
4. 跳躍到 initramfs 檔案系統的根目錄
5. 在 initramfs 中查找 "config.txt" → 找到檔案
6. 返回該檔案的 vnode
```

### 完整的檔案系統層次
```
tmpfs (主檔案系統 - rootfs):           
    /                           
    ├── tmp/                    (tmpfs directory)
    ├── usr/                    (tmpfs directory)
    │   └── bin/               (tmpfs directory)
    ├── home/                  (tmpfs directory)
    │   └── user/              (tmpfs directory)
    │       ├── readme.txt     (tmpfs file)
    │       └── documents/     (tmpfs directory)
    └── initramfs/  ← 掛載點   
        └─→ (進入 initramfs 檔案系統)
            ├── config.txt     (initramfs file - read only)
            ├── vfs1.img       (initramfs file - read only)
            └── syscall.img    (initramfs file - read only)
```
