#include "syscall.h"
#include "sched.h"
#include "muart.h"
#include "mailbox.h"
#include "cpio.h"
#include "utils.h"
#include "malloc.h"
#include "list.h"
#include "string.h"

// void* const sys_call_table[] = {sys_getpid, sys_uartread, sys_uartwrite, sys_exec, sys_fork, sys_exit, sys_mbox_call, sys_kill};


// System call handler
void syscall_handler(void) {
    // Get the system call number from x8
    unsigned long syscall_num;
    asm volatile("mov %0, x8" : "=r"(syscall_num));
    
    // Get the arguments from x0-x7
    unsigned long args[8];
    asm volatile(
        "mov %0, x0\n"
        "mov %1, x1\n"
        "mov %2, x2\n"
        "mov %3, x3\n"
        "mov %4, x4\n"
        "mov %5, x5\n"
        "mov %6, x6\n"
        "mov %7, x7"
        : "=r"(args[0]), "=r"(args[1]), "=r"(args[2]), "=r"(args[3]),
          "=r"(args[4]), "=r"(args[5]), "=r"(args[6]), "=r"(args[7])
    );
    
    // Handle the system call
    unsigned long ret = 0;
    switch(syscall_num) {
        case SYS_GETPID:
            ret = sys_getpid();
            break;
        case SYS_UARTREAD:
            ret = sys_uartread((char*)args[0], (size_t)args[1]);
            break;
        case SYS_UARTWRITE:
            ret = sys_uartwrite((const char*)args[0], (size_t)args[1]);
            break;
        case SYS_EXEC:
            ret = sys_exec((const char*)args[0], (char *const*)args[1]);
            break;
        case SYS_FORK:
            ret = sys_fork();
            break;
        case SYS_EXIT:
            sys_exit((int)args[0]);
            break;
        case SYS_MBOX_CALL:
            ret = sys_mbox_call((unsigned char)args[0], (unsigned int*)args[1]);
            break;
        case SYS_KILL:
            sys_kill((int)args[0]);
            break;
        default:
            muart_puts("Unknown system call\r\n");
            break;
    }
    
    // Set the return value in x0
    asm volatile("mov x0, %0" : : "r"(ret));
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
    struct task_struct* current = (struct task_struct*)get_current_thread();
    struct task_struct* new_task = (struct task_struct*)dmalloc(sizeof(struct task_struct));
    
    if (!new_task) {
        return -1;
    }
    
    // Copy the parent's context
    *new_task = *current;
    
    // Allocate new stack
    new_task->stack = dmalloc(THREAD_STACK_SIZE);
    if (!new_task->stack) {
        dfree(new_task);
        return -1;
    }
    
    // Allocate new PID
    new_task->pid = pid_alloc();
    if (new_task->pid < 0) {
        dfree(new_task->stack);
        dfree(new_task);
        return -1;
    }
    
    // Copy the stack content
    memcpy(new_task->stack, current->stack, THREAD_STACK_SIZE);
    
    // Update stack pointer
    new_task->cpu_context.sp = (unsigned long)((char*)new_task->stack + THREAD_STACK_SIZE);
    
    // Add to run queue
    INIT_LIST_HEAD(&new_task->list);
    list_add_tail(&new_task->list, &rq);
    
    // Add to task list
    INIT_LIST_HEAD(&new_task->task);
    list_add_tail(&new_task->task, &task_lists);
    
    return new_task->pid;
}

void sys_exit(int status) {
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