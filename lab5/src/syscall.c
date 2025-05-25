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
#include "mm.h"
#include "pid.h"


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
    struct task_struct* current = (struct task_struct*)get_current_thread();
    
    // Clean up current user program
    if (current->user_program) {
        dfree(current->user_program);
        current->user_program = NULL;
        current->user_program_size = 0;
    }
    
    // Load new program from initramfs
    const void* initramfs_addr = get_cpio_addr();
    unsigned long new_program_addr = cpio_load_program(initramfs_addr, name);

    if (!new_program_addr) {
        muart_puts("Error: Failed to load program: ");
        muart_puts(name);
        muart_puts("\r\n");
        return -1;
    }

    // Update trap frame to start executing new program
    struct trap_frame* regs = task_tf(current);
    
    // Reset user context
    memzero((unsigned long)regs, sizeof(*regs));
    
    // Set new program entry point
    regs->elr_el1 = new_program_addr;
    regs->sp_el0 = (unsigned long)current->user_stack + THREAD_STACK_SIZE;
    regs->spsr_el1 = 0;  // EL0t mode
    
    muart_puts("exec() successful, new program loaded at: ");
    muart_send_hex(new_program_addr);
    muart_puts("\r\n");
    
    return 0;
}

int sys_fork(void) {
    muart_puts("Fork system call invoked\r\n");
    struct task_struct* parent = (struct task_struct*)get_current_thread();
    struct task_struct* child;

    // Create a child task
    child = (struct task_struct*)dmalloc(sizeof(struct task_struct));
    if (!child) {
        muart_puts("fork: Failed to allocate task structure\r\n");
        return -1;
    }

    child->pid = pid_alloc();
    if (child->pid < 0) {
        dfree(child);
        muart_puts("fork: Failed to allocate PID\r\n");
        return -1;
    }
    
    child->kernel_stack = dmalloc(THREAD_STACK_SIZE);
    if (!child->kernel_stack) {
        pid_free(child->pid);
        dfree(child);
        muart_puts("fork: Failed to allocate kernel stack\r\n");
        return -1;
    }

    // Allocate 一個 memory space 給 child 的 user program 使用
    child->user_program = dmalloc(parent->user_program_size);

    // copy whole parent's user program data to child's user program memory space
    if (!child->user_program) {
        dfree(child->kernel_stack);
        pid_free(child->pid);
        dfree(child);
        muart_puts("fork: Failed to allocate user program\r\n");
        return -1;
    }
    memcpy(child->user_program, parent->user_program, parent->user_program_size);
    child->user_program_size = parent->user_program_size;

    // Allocate user stack for child
    child->user_stack = dmalloc(THREAD_STACK_SIZE);
    if (!child->user_stack) {
        dfree(child->kernel_stack);
        pid_free(child->pid);
        dfree(child);
        dfree(child->user_program);
        muart_puts("fork: Failed to allocate user stack\r\n");
        return -1;
    }
    // Copy parent's user stack data to child's user stack
    memcpy(child->user_stack, parent->user_stack, THREAD_STACK_SIZE);
    
    child->parent = parent;
    child->state = TASK_RUNNING;

    // Copy CPU context from parent to child
    child->cpu_context = parent->cpu_context;
    
    // Set the child task's stack pointer to the top of its user stack
    child->cpu_context.sp = (unsigned long)((char*)child->kernel_stack + THREAD_STACK_SIZE);

    // Set the child task's trap frame
    struct trap_frame* parent_tf = task_tf(parent);
    struct trap_frame* child_tf = task_tf(child);

    // Copy the parent's trap frame to the child's trap frame
    memcpy(child_tf, parent_tf, sizeof(struct trap_frame));

    // Set the return value in the child's trap frame to 0 (indicating fork success)
    child_tf->regs[0] = 0;

    // Set the return value in the parent's trap frame to the child's PID
    parent_tf->regs[0] = child->pid;

    // Set the child's sp_el0 according to the parent's user stack's usage
    long stack_offset = parent_tf->sp_el0 - (unsigned long)parent->user_stack;
    child_tf->sp_el0 = (unsigned long)child->user_stack + stack_offset;
    
    // Set the child's elr_el1 to the parent's elr_el1 offset by the child's user program address (continue the execution from the same point)
    long pc_offset = parent_tf->elr_el1 - (unsigned long)parent->user_program;
    child_tf->elr_el1 = (unsigned long)child->user_program + pc_offset;

    // Set the child's spsr_el1 to 0 (EL0t mode)
    child_tf->spsr_el1 = 0;  // EL0t mode

    // Add the child task to the run queue
    INIT_LIST_HEAD(&child->list);
    list_add_tail(&child->list, &rq);
    
    // Add the child task to the task list
    INIT_LIST_HEAD(&child->task);
    list_add_tail(&child->task, &task_lists);
    
    return child->pid;  // Return child's PID to the parent
}

void sys_exit() {
    thread_exit();
}

int sys_mbox_call(unsigned int ch, unsigned int *mbox) {
    mailbox_call(ch, mbox);
    return 0;
}

// Todo
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