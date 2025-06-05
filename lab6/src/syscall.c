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
#include "registers.h"
#include "utils.h"


void syscall_handler(struct trap_frame* tf) {
    // Get the syscall number from X8 register
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

    // Disable interrupts when updating the trap frame
    disable_irq_in_el1();

    // Update trap frame to start executing new program
    struct trap_frame* regs = task_tf(current);
    
    // Reset user context
    memzero((unsigned long)regs, sizeof(*regs));
    
    // Set new program entry point
    regs->elr_el1 = new_program_addr;
    regs->sp_el0 = (unsigned long)current->user_stack + THREAD_STACK_SIZE;
    regs->spsr_el1 = 0;  // EL0t mode
    
    enable_irq_in_el1();

    muart_puts("exec() successful, new program loaded at: ");
    muart_send_hex(new_program_addr);
    muart_puts("\r\n");
    
    return 0;
}

int sys_fork() {
    disable_irq_in_el1();
    
    muart_puts("Fork system call invoked\r\n");
    struct task_struct* parent = (struct task_struct*)get_current_thread();
    struct task_struct* child;

    // Create a child task
    child = (struct task_struct*)dmalloc(sizeof(struct task_struct));
    if (!child) {
        muart_puts("fork: Failed to allocate task structure\r\n");
        enable_irq_in_el1();
        return -1;
    }

    child->pid = pid_alloc();
    if (child->pid < 0) {
        dfree(child);
        muart_puts("fork: Failed to allocate PID\r\n");
        enable_irq_in_el1();
        return -1;
    }
    
    child->kernel_stack = dmalloc(THREAD_STACK_SIZE);
    if (!child->kernel_stack) {
        pid_free(child->pid);
        dfree(child);
        muart_puts("fork: Failed to allocate kernel stack\r\n");
        enable_irq_in_el1();
        return -1;
    }
    
    // Allocate user stack for child
    child->user_stack = dmalloc(THREAD_STACK_SIZE);
    if (!child->user_stack) {
        dfree(child->kernel_stack);
        pid_free(child->pid);
        dfree(child);
        muart_puts("fork: Failed to allocate user stack\r\n");
        enable_irq_in_el1();
        return -1;
    }

    // Copy the stacks
    memcpy(child->kernel_stack, parent->kernel_stack, THREAD_STACK_SIZE);
    memcpy(child->user_stack, parent->user_stack, THREAD_STACK_SIZE);

    child->user_program = parent->user_program;  // Share user program pointer
    child->user_program_size = parent->user_program_size; 

    child->parent = parent;
    child->state = TASK_RUNNING;

    /*
    // Calculate memory offsets
    long kstack_offset = (long)child->kernel_stack - (long)parent->kernel_stack;
    long ustack_offset = (long)child->user_stack - (long)parent->user_stack;
    */

    // Get trap frames
    struct trap_frame* parent_tf = task_tf(parent);
    struct trap_frame* child_tf = task_tf(child);
    
    // Copy the parent's trap frame to the child's trap frame
    memcpy(child_tf, parent_tf, sizeof(struct trap_frame));

    child_tf->regs[19] = 0;
    child_tf->regs[20] = 0;

    // Adjust child's SP_EL0 to point to child's user stack with same offset
    unsigned long parent_sp_offset = parent_tf->sp_el0 - (unsigned long)parent->user_stack;
    child_tf->sp_el0 = (unsigned long)child->user_stack + parent_sp_offset;

    /* 靠邀 結果是 mailbox 的問題 我這邊不改好像也沒差 ==
    // Adjust all registers in child's trap frame that point to parent's memory
    for (int i = 0; i < 31; i++) {
        unsigned long reg_val = parent_tf->regs[i];
        
        // Check if register points into parent's user stack
        if (reg_val >= (unsigned long)parent->user_stack && 
            reg_val < (unsigned long)parent->user_stack + THREAD_STACK_SIZE) {
            
            child_tf->regs[i] = reg_val + ustack_offset;
            
            muart_puts("  Adjusted child X");
            muart_send_dec(i);
            muart_puts(": ");
            muart_send_hex(reg_val);
            muart_puts(" -> ");
            muart_send_hex(child_tf->regs[i]);
            muart_puts("\r\n");
        }
    }

 
    // 調整 child stacks 中的指標
    unsigned long* child_ustack_ptr = (unsigned long*)child->user_stack;
    unsigned long* child_kstack_ptr = (unsigned long*)child->kernel_stack;

    // 調整 child user stack 中的指標
    for (int i = 0; i < THREAD_STACK_SIZE / sizeof(unsigned long); i++) {
        unsigned long val = child_ustack_ptr[i];
        
        // Check if this value looks like a pointer to parent's user stack
        if (val >= (unsigned long)parent->user_stack && 
            val < (unsigned long)parent->user_stack + THREAD_STACK_SIZE) {
            
            child_ustack_ptr[i] = val + ustack_offset;
        }
        // Check if this value looks like a pointer to parent's kernel stack
        else if (val >= (unsigned long)parent->kernel_stack && 
                val < (unsigned long)parent->kernel_stack + THREAD_STACK_SIZE) {
            
            child_ustack_ptr[i] = val + kstack_offset;
        }
    }

    // 調整 child kernel stack 中的指標
    for (int i = 0; i < THREAD_STACK_SIZE / sizeof(unsigned long); i++) {
        unsigned long val = child_kstack_ptr[i];
        
        // Check if this value looks like a pointer to parent's user stack
        if (val >= (unsigned long)parent->user_stack && 
            val < (unsigned long)parent->user_stack + THREAD_STACK_SIZE) {
            
            child_kstack_ptr[i] = val + ustack_offset;
        }
        // Check if this value looks like a pointer to parent's kernel stack
        else if (val >= (unsigned long)parent->kernel_stack && 
                val < (unsigned long)parent->kernel_stack + THREAD_STACK_SIZE) {
            
            child_kstack_ptr[i] = val + kstack_offset;
        }
    }
    */
    

    // Set the return values
    child_tf->regs[0] = 0;  // Child returns 0
    parent_tf->regs[0] = child->pid;  // Parent returns child PID

    // Set up child's CPU context
    memzero((unsigned long)&child->cpu_context, sizeof(struct cpu_context));
    child->cpu_context.lr = (unsigned long)ret_to_user; 
    child->cpu_context.sp = (unsigned long)child_tf;

    // Add the child task to the run queue
    INIT_LIST_HEAD(&child->list);
    list_add_tail(&child->list, &rq);
    
    // Add the child task to the task list
    INIT_LIST_HEAD(&child->task);
    list_add_tail(&child->task, &task_lists);

    enable_irq_in_el1();    // Will enter schedule() since timer interrupt

    return child->pid;  // Return child's PID to the parent
}

void sys_exit() {
    thread_exit();
}

int sys_mbox_call(unsigned int ch, unsigned int *mbox) {
    disable_irq_in_el1();

    unsigned long msg_addr = (unsigned long)mbox;
    
    // Combine the message address (upper 28 bits) with channel number (lower 4 bits)  
    msg_addr = (msg_addr & ~0xF) | (ch & 0xF);
    
    // Check whether the Mailbox 0 status register’s full flag is set.
    while( regRead(MAILBOX_STATUS) & MAILBOX_FULL ){
        // Mailbox is Full: do nothing
    }
    // If not, then you can write the data to Mailbox 1 Read/Write register.
    regWrite(MAILBOX_WRITE, msg_addr);
    
    while(1){
        // Check whether the Mailbox 0 status register’s empty flag is set.
        while( regRead(MAILBOX_STATUS) & MAILBOX_EMPTY ){
            // Mailbox is Empty: do nothing
        }
        // If not, then you can read from Mailbox 0 Read/Write register.
        unsigned int response = regRead(MAILBOX_READ);
    
        // Check if the value is the same as you wrote in step 1.
        if((response & 0xF) == ch){
            return;     // 這邊不要亂加 return 0，加了會狂跳 exception，原因 I don't know ==
        }
    }
    enable_irq_in_el1();
    return 0;
}

void sys_kill(int pid) {
    disable_irq_in_el1();
    struct list_head* pos;
    struct list_head* n;
    struct task_struct* task;
    
    list_for_each_safe(pos, n, &task_lists) {
        task = list_entry(pos, struct task_struct, task);
        if (task->pid == pid) {
            muart_puts("Killing task PID: ");
            muart_send_dec(pid);
            muart_puts("\r\n");
            
            task->state = TASK_ZOMBIE;
            
            if (!list_empty(&task->list)) {
                list_del(&task->list);
            }
            
            if (task == get_current_thread()) {
                schedule();
            }
    
            enable_irq_in_el1();
            return;
        }
    }
}