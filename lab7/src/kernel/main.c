#include "muart.h"
#include "shell.h"
#include "fdt.h"
#include "cpio.h"
#include "exception.h"
#include "timer.h"
#include "malloc.h"
#include "sched.h"
#include "vfs.h"

/* Global buddy system instance */
buddy_system_t buddy;

/* Page array for buddy system */
page_t page_array[BUDDY_MEM_SIZE / PAGE_SIZE];

extern void kernel_fork_process();
extern void kernel_fork_process_cpio(void*);

int syscall_test(){
    muart_puts("Starting system call test ...\r\n");
    
    // Create a kernel thread that will move to user mode
    pid_t pid = kernel_thread(kernel_fork_process, NULL);

    if (pid < 0) {
        muart_puts("Error: Failed to create kernel thread\r\n");
        return -1;
    }
    
    muart_puts("Created kernel thread with PID: ");
    muart_send_dec(pid);
    muart_puts("\r\n");
    
    schedule();
    
    return 0;
}

int video_player_test(void* fdt) {
    muart_puts("Starting system call test with initramfs.cpio program ...\r\n");
    
    // Create a kernel thread that will move to user mode
    // pid_t pid = kernel_thread(kernel_fork_process, NULL);

    // Create initialization data
    struct task_init_data* init_data = (struct task_init_data*)dmalloc(sizeof(struct task_init_data));
    if (!init_data) {
        muart_puts("Error: Failed to allocate init data\r\n");
        return -1;
    }
    
    init_data->filename = "syscall.img";
    init_data->initramfs_addr = get_initramfs_address(fdt);

    // Create a kernel thread that will load and execute syscall.img
    pid_t pid = kernel_thread(kernel_fork_process_cpio, init_data);
    if (pid < 0) {
        dfree(init_data);
        muart_puts("Error: Failed to create kernel thread\r\n");
        return -1;
    }
    
    muart_puts("Created kernel thread with PID: ");
    muart_send_dec(pid);
    muart_puts("\r\n");
    
    schedule();
    
    return 0;
}
/*
void vfs_test(void) {
    muart_puts("\r\n=== VFS Test Started ===\r\n");
    
    setup_root_filesystem();

    struct vnode* root;
    int ret = vfs_lookup("/", &root);
    if (ret != VFS_OK) {
        muart_puts("Failed to lookup root directory\r\n");
        return;
    }
    muart_puts("Root directory lookup: OK\r\n");
    
    struct file* file;

    // ==================== ORIGINAL TESTS ====================
    
    // 測試 1: 創建和寫入文件
    muart_puts("\n--- Test 1: Create and Write ---\r\n");
    ret = vfs_open("/test.txt", O_CREAT | O_RDWR, &file);
    if (ret != VFS_OK) {
        muart_puts("Failed to create file\r\n");
        return;
    }
    muart_puts("File created: OK\r\n");
    
    ret = vfs_write(file, "Hello", 5);
    if (ret < 0) {
        muart_puts("Failed to write file\r\n");
        vfs_close(file);
        return;
    }
    muart_puts("Write to file: OK\r\n");
    
    // 重設文件位置到開頭進行讀取
    vfs_lseek64(file, 0, SEEK_SET);
    // 或者關閉後重新打開
    vfs_close(file);
    
    // 測試 2: 重新打開並讀取
    muart_puts("\n--- Test 2: Reopen and Read ---\r\n");
    ret = vfs_open("/test.txt", O_RDONLY, &file);
    if (ret != VFS_OK) {
        muart_puts("Failed to reopen file\r\n");
        return;
    }
    muart_puts("File reopened: OK\r\n");
    
    char buf[64] = {0};
    ret = vfs_read(file, buf, 64);
    if (ret < 0) {
        muart_puts("Failed to read file\r\n");
        vfs_close(file);
        return;
    }
    
    muart_puts("Read from file: ");
    muart_puts(buf);
    muart_puts("\r\n");
    vfs_close(file);
    
    // 測試 3: 追加模式測試
    muart_puts("\n--- Test 3: Append Mode ---\r\n");
    ret = vfs_open("/test.txt", O_WRONLY | O_APPEND, &file);
    if (ret != VFS_OK) {
        muart_puts("Failed to open file in append mode\r\n");
        return;
    }
    
    ret = vfs_write(file, " World!", 7);
    if (ret < 0) {
        muart_puts("Failed to append to file\r\n");
        vfs_close(file);
        return;
    }
    muart_puts("Append to file: OK\r\n");
    vfs_close(file);
    
    // 測試 4: 讀取完整內容
    muart_puts("\n--- Test 4: Read Full Content ---\r\n");
    ret = vfs_open("/test.txt", O_RDONLY, &file);
    if (ret == VFS_OK) {
        char full_buf[64] = {0};
        ret = vfs_read(file, full_buf, 64);
        muart_puts("Final content: ");
        muart_puts(full_buf);
        muart_puts("\r\n");
        vfs_close(file);
    }
    
    // 測試 5: 截斷測試
    muart_puts("\n--- Test 5: Truncate Test ---\r\n");
    ret = vfs_open("/test.txt", O_WRONLY | O_TRUNC, &file);
    if (ret == VFS_OK) {
        ret = vfs_write(file, "New Content", 11);
        muart_puts("Truncate and write: OK\r\n");
        vfs_close(file);
        
        // 驗證截斷結果
        ret = vfs_open("/test.txt", O_RDONLY, &file);
        if (ret == VFS_OK) {
            char trunc_buf[64] = {0};
            ret = vfs_read(file, trunc_buf, 64);
            muart_puts("After truncate: ");
            muart_puts(trunc_buf);
            muart_puts("\r\n");
            vfs_close(file);
        }
    }

    // ==================== NEW MULTI-LEVEL VFS TESTS ====================
    
    // 測試 6: 創建子目錄 (Create subdirectories)
    muart_puts("\n--- Test 6: Create Subdirectories ---\r\n");
    
    ret = vfs_mkdir("/home");
    if (ret == VFS_OK) {
        muart_puts("Create /home: OK\r\n");
    } else {
        muart_puts("Create /home: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    ret = vfs_mkdir("/usr");
    if (ret == VFS_OK) {
        muart_puts("Create /usr: OK\r\n");
    } else {
        muart_puts("Create /usr: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    ret = vfs_mkdir("/home/user");
    if (ret == VFS_OK) {
        muart_puts("Create /home/user: OK\r\n");
    } else {
        muart_puts("Create /home/user: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    ret = vfs_mkdir("/usr/bin");
    if (ret == VFS_OK) {
        muart_puts("Create /usr/bin: OK\r\n");
    } else {
        muart_puts("Create /usr/bin: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    ret = vfs_mkdir("/home/user/documents");
    if (ret == VFS_OK) {
        muart_puts("Create /home/user/documents: OK\r\n");
    } else {
        muart_puts("Create /home/user/documents: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    // 測試 7: 路徑名查找 (Look up pathnames)
    muart_puts("\n--- Test 7: Pathname Lookup ---\r\n");
    
    struct vnode* vnode = NULL;
    
    ret = vfs_lookup("/home", &vnode);
    if (ret == VFS_OK) {
        muart_puts("Lookup /home: OK\r\n");
    } else {
        muart_puts("Lookup /home: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    ret = vfs_lookup("/usr/bin", &vnode);
    if (ret == VFS_OK) {
        muart_puts("Lookup /usr/bin: OK\r\n");
    } else {
        muart_puts("Lookup /usr/bin: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    ret = vfs_lookup("/home/user/documents", &vnode);
    if (ret == VFS_OK) {
        muart_puts("Lookup /home/user/documents: OK\r\n");
    } else {
        muart_puts("Lookup /home/user/documents: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    // 測試不存在的路徑
    ret = vfs_lookup("/nonexistent/path", &vnode);
    if (ret == VFS_ENOENT) {
        muart_puts("Lookup nonexistent path: OK (correctly failed)\r\n");
    } else {
        muart_puts("Lookup nonexistent path: UNEXPECTED RESULT (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    // 測試 8: 在子目錄中創建文件
    muart_puts("\n--- Test 8: Files in Subdirectories ---\r\n");
    
    ret = vfs_open("/home/user/readme.txt", O_CREAT | O_WRONLY, &file);
    if (ret == VFS_OK) {
        ret = vfs_write(file, "User home directory!", 20);
        vfs_close(file);
        muart_puts("Create /home/user/readme.txt: OK\r\n");
    } else {
        muart_puts("Create /home/user/readme.txt: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    ret = vfs_open("/usr/bin/program", O_CREAT | O_WRONLY, &file);
    if (ret == VFS_OK) {
        ret = vfs_write(file, "Binary program data", 19);
        vfs_close(file);
        muart_puts("Create /usr/bin/program: OK\r\n");
    } else {
        muart_puts("Create /usr/bin/program: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    ret = vfs_open("/home/user/documents/notes.txt", O_CREAT | O_WRONLY, &file);
    if (ret == VFS_OK) {
        ret = vfs_write(file, "Personal notes", 14);
        vfs_close(file);
        muart_puts("Create /home/user/documents/notes.txt: OK\r\n");
    } else {
        muart_puts("Create /home/user/documents/notes.txt: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    // 測試 9: 從子目錄讀取文件
    muart_puts("\n--- Test 9: Read Files from Subdirectories ---\r\n");
    
    ret = vfs_open("/home/user/readme.txt", O_RDONLY, &file);
    if (ret == VFS_OK) {
        char subdir_buf[64] = {0};
        int bytes_read = vfs_read(file, subdir_buf, 63);
        if (bytes_read > 0) {
            muart_puts("Content of /home/user/readme.txt: ");
            muart_puts(subdir_buf);
            muart_puts("\r\n");
        }
        vfs_close(file);
    }
    
    ret = vfs_open("/home/user/documents/notes.txt", O_RDONLY, &file);
    if (ret == VFS_OK) {
        char notes_buf[64] = {0};
        int bytes_read = vfs_read(file, notes_buf, 63);
        if (bytes_read > 0) {
            muart_puts("Content of /home/user/documents/notes.txt: ");
            muart_puts(notes_buf);
            muart_puts("\r\n");
        }
        vfs_close(file);
    }
    
    // 測試 10: 掛載文件系統 (Mount file systems on directories)
    muart_puts("\n--- Test 10: Mount File Systems ---\r\n");
    
    ret = vfs_mkdir("/mnt");
    if (ret == VFS_OK) {
        muart_puts("Create mount point /mnt: OK\r\n");
    } else {
        muart_puts("Create mount point /mnt: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    ret = vfs_mount("/mnt", "tmpfs");
    if (ret == VFS_OK) {
        muart_puts("Mount tmpfs at /mnt: OK\r\n");
    } else {
        muart_puts("Mount tmpfs at /mnt: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
    // 測試掛載點是否工作
    ret = vfs_open("/mnt/mounted_file.txt", O_CREAT | O_WRONLY, &file);
    if (ret == VFS_OK) {
        ret = vfs_write(file, "File in mounted fs", 18);
        vfs_close(file);
        muart_puts("Create file in mounted filesystem: OK\r\n");
    } else {
        muart_puts("Create file in mounted filesystem: FAILED (");
        muart_send_dec(ret);
        muart_puts(")\r\n");
    }
    
//     // 驗證掛載的文件系統
//     ret = vfs_open("/mnt/mounted_file.txt", O_RDONLY, &file);
//     if (ret == VFS_OK) {
//         char mount_buf[64] = {0};
//         int bytes_read = vfs_read(file, mount_buf, 63);
//         if (bytes_read > 0) {
//             muart_puts("Content of mounted file: ");
//             muart_puts(mount_buf);
//             muart_puts("\r\n");
//         }
//         vfs_close(file);
//     }
    
//     // 測試 11: 錯誤處理
//     muart_puts("\n--- Test 11: Error Handling ---\r\n");
    
//     ret = vfs_mkdir("/home/user");  // 已存在的目錄
//     if (ret == VFS_EEXIST) {
//         muart_puts("Create existing directory: OK (correctly failed)\r\n");
//     } else {
//         muart_puts("Create existing directory: UNEXPECTED RESULT (");
//         muart_send_dec(ret);
//         muart_puts(")\r\n");
//     }
    
//     ret = vfs_mkdir("/nonexistent/newdir");  // 父目錄不存在
//     if (ret == VFS_ENOENT) {
//         muart_puts("Create dir with missing parent: OK (correctly failed)\r\n");
//     } else {
//         muart_puts("Create dir with missing parent: UNEXPECTED RESULT (");
//         muart_send_dec(ret);
//         muart_puts(")\r\n");
//     }
    
//     muart_puts("\n=== VFS Test Summary ===\r\n");
//     muart_puts("Basic file operations: TESTED\r\n");
//     muart_puts("Multi-level directories: TESTED\r\n");
//     muart_puts("Pathname lookup: TESTED\r\n");
//     muart_puts("File system mounting: TESTED\r\n");
//     muart_puts("Error handling: TESTED\r\n");
//     muart_puts("\n=== VFS Test Completed ===\r\n");
// }

// // 簡化版測試（如果你沒有實現 lseek）
// void vfs_test_simple(void) {
//     muart_puts("=== Simple VFS Test ===\r\n");
    
//     setup_root_filesystem();
    
//     struct file* file;
//     int ret;
    
//     // 創建並寫入
//     ret = vfs_open("/simple.txt", O_CREAT | O_WRONLY, &file);
//     if (ret == VFS_OK) {
//         vfs_write(file, "Hello World", 11);
//         vfs_close(file);
//         muart_puts("Write completed\r\n");
//     }
    
//     // 重新打開並讀取
//     ret = vfs_open("/simple.txt", O_RDONLY, &file);
//     if (ret == VFS_OK) {
//         char buf[64] = {0};
//         int bytes_read = vfs_read(file, buf, 64);
//         muart_puts("Read: ");
//         muart_puts(buf);
//         muart_puts("\r\n");
//         vfs_close(file);
//     }
    
//     muart_puts("=== Test Done ===\r\n");
}
*/

int vfs_img_test(void* fdt) {
    muart_puts("Starting vfs img test with initramfs.cpio program ...\r\n");    

    // Create initialization data
    struct task_init_data* init_data = (struct task_init_data*)dmalloc(sizeof(struct task_init_data));
    if (!init_data) {
        muart_puts("Error: Failed to allocate init data\r\n");
        return -1;
    }
    
    init_data->filename = "vfs1.img";
    init_data->initramfs_addr = get_initramfs_address(fdt);

    // Create a kernel thread that will load and execute syscall.img
    pid_t pid = kernel_thread(kernel_fork_process_cpio, init_data);
    if (pid < 0) {
        dfree(init_data);
        muart_puts("Error: Failed to create kernel thread\r\n");
        return -1;
    }
    
    muart_puts("Created kernel thread with PID: ");
    muart_send_dec(pid);
    muart_puts("\r\n");
    
    schedule();
    
    return 0;
}

void main(void* fdt){    
    // Initialize mini UART
    muart_init();

    // Print the dtb loading address
    muart_puts("Kernel : flatten device tree address is at ");
    muart_send_hex(((unsigned long)fdt));
    muart_puts("\r\n");

    // Get the initramfs address from the device tree
    unsigned long initramfs_addr = get_initramfs_address(fdt);
    if (initramfs_addr == 1) {
        muart_puts("Error: Failed to get initramfs address from device tree\r\n");
        return;
    }
    muart_puts("Initramfs_addr is at ");
    muart_send_hex(initramfs_addr);
    muart_puts("\r\n");

    // Update the initramfs address in CPIO module
    set_initramfs_address(initramfs_addr);

    // Initialize the vector table
    exception_table_init();
    
    // Enable the core timer and enable the core timer interrupt
    core_timer_init();
    muart_puts("Core timer initialized successful !\r\n");

    // Initialize the buddy allocator and memory pools
    buddy_init(&buddy, (void *)BUDDY_MEM_START, BUDDY_MEM_SIZE, page_array);    
    memory_pools_init();
    muart_puts("Dynamic allocator initialized successful !\r\n");

    // Initialize the scheduler
    sched_init();

    // Demo of the dynamic allocator
    // dynamic_allocator_demo();

    // Test thread mechanism
    // muart_puts("\r\n=== Starting Thread Test ===\r\n");
    // thread_test();
    // muart_puts("=== Thread Test Completed ===\r\n");

    // syscall test
    // syscall_test();
    
    // video player test
    // video_player_test(fdt);

    // Set the tmpfs as the root file system
    setup_root_filesystem();

    // Initialize the initramfs
    init_initramfs();

    // Virtual File System test
    // vfs_test();

    // Demo of vfs1.img
    vfs_img_test(fdt);

    // Start Simple Shell
    shell();
}