#include "syscall.h"
#include "sched.h"
#include "muart.h"
#include "mailbox.h"
#include "cpio.h"
#include "utils.h"
#include "malloc.h"
#include "list.h"
#include "string.h"
#include "exception.h"


void syscall_handler(struct trap_frame* tf) {
    // Get the system call number from x8 in the trap frame
    unsigned long syscall_num = tf->regs[8];
    

    // Handle the system call, the arguments from x0-x7 in the trap frame
    unsigned long ret = 0;
    switch(syscall_num) {
        case SYS_GETPID:
            ret = sys_getpid();
            break;
        case SYS_UARTREAD:
            ret = sys_uartread((char*)tf->regs[0], (size_t)tf->regs[1]);
            break;
        case SYS_UARTWRITE:
            ret = sys_uartwrite((const char*)tf->regs[0], (size_t)tf->regs[1]);
            break;
        case SYS_EXEC:
            ret = sys_exec((const char*)tf->regs[0], (char *const*)tf->regs[1]);
            break;
        case SYS_FORK:
            ret = sys_fork();
            break;
        case SYS_EXIT:
            sys_exit();
            break;
        case SYS_MBOX_CALL:
            ret = sys_mbox_call((unsigned char)tf->regs[0], (unsigned int*)tf->regs[1]);
            break;
        case SYS_KILL:
            sys_kill((int)tf->regs[0]);
            break;
        default:
            muart_puts("Unknown system call\r\n");
            break;
    }
    
    // Set the return value in the trap frame
    tf->regs[0] = ret;
}

// System call implementations
int sys_getpid(void) {
    struct task_struct* current = (struct task_struct*)get_current_thread();
    return current->pid;
}

size_t sys_uartread(char buf[], size_t size) {
    for(size_t i = 0; i < size; i++) {
        buf[i] = muart_receive();
    }
    return size;
}

size_t sys_uartwrite(const char buf[], size_t size) {
    for(size_t i = 0; i < size; i++) {
        muart_send(buf[i]);
    }
    return size;
}

int sys_exec(const char* name, char *const argv[]) {
    void* initramfs_addr = get_cpio_addr();
    cpio_exec(initramfs_addr, name);
    return 0;  // 成功執行返回 0
}

int sys_fork(void) {
    muart_puts("Fork system call invoked\r\n");
    
    return 0;
}

void sys_exit() {
    thread_exit();
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox) {
    mailbox_call(mbox);
    return 0;  // 成功執行返回 0
}

void sys_kill(int pid) {
    struct list_head* pos;
    struct task_struct* task;
    
    list_for_each(pos, &task_lists) {
        task = list_entry(pos, struct task_struct, task);
        if (task->pid == pid) {
            task->state = TASK_ZOMBIE;
            list_del(&task->list);
            return;
        }
    }
} 