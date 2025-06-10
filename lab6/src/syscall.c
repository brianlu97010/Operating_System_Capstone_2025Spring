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
#include "vm.h"
#include "mailbox.h"


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
    disable_irq_in_el1();

    struct task_struct* current = (struct task_struct*)get_current_thread();
    
    // Clean up current user program
    if (current->user_program) {
        dfree(current->user_program);
        current->user_program = NULL;
        current->user_program_size = 0;
    }

    current->user_stack = NULL;
    current->user_program = NULL;
    current->user_program_size = 0;
    
    // Load new program from initramfs
    const void* initramfs_addr = get_cpio_addr();
    
    // Checke if initramfs address is VA or PA, if PA, convert it to VA in kernel space
    if ((unsigned long)(initramfs_addr) < KERNEL_VA_BASE) {
        initramfs_addr = (const void*)(PHYS_TO_VIRT(initramfs_addr));
    }
    unsigned long new_program_addr = cpio_load_program(initramfs_addr, name);

    if (mappages(current->pgd, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, PERIPHERAL_START) != 0) {
        muart_puts("Error: Failed to map peripheral memory to user virtual address space\r\n");
    }

    // Update trap frame to start executing new program
    struct trap_frame* regs = task_tf(current);
    
    // Reset user context
    memzero((unsigned long)regs, sizeof(*regs));
    
    // Set new program entry point
    regs->elr_el1 = USER_CODE_BASE;
    regs->sp_el0 = USER_STACK_TOP;  
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
    child->user_stack = dmalloc(USER_STACK_SIZE);
    if (!child->user_stack) {
        dfree(child->kernel_stack);
        pid_free(child->pid);
        dfree(child);
        muart_puts("fork: Failed to allocate user stack\r\n");
        enable_irq_in_el1();
        return -1;
    }
    memzero((unsigned long)child->user_stack, USER_STACK_SIZE);  // Initialize user stack to zero

    // Allocate a memory space for the task's user PGD page table
    child->pgd = dmalloc(PAGE_SIZE);
    if (!child->pgd) {
        pid_free(child->pid);
        dfree(child->kernel_stack);
        dfree(child->user_stack);
        dfree(child);
        enable_irq_in_el1();
        return -1;
    }
    // Initialze the content in PGD to zero
    memzero((unsigned long)child->pgd, PAGE_SIZE);

    // Allocate a physical memory space for the user program
    child->user_program_size = parent->user_program_size; 
    child->user_program = dmalloc(child->user_program_size);
    if (!child->user_program) {
        pid_free(child->pid);
        dfree(child->kernel_stack);
        dfree(child->user_stack);
        dfree(child);
        dfree(child->pgd);
        enable_irq_in_el1();
        return -1;
    } 

    // Copy the parent's user program data to the child's user program
    memcpy(child->user_program, parent->user_program, child->user_program_size);
    
    // Mapping the physical memory space of the user program to the user virtual address
    unsigned long user_program_pa = (unsigned long)VIRT_TO_PHYS(child->user_program);
    if (mappages(child->pgd, USER_CODE_BASE, child->user_program_size, user_program_pa) != 0) {
        dfree(child->user_program);
        muart_puts("Error: Failed to map user program to virtual address space\r\n");
        return 0;
    }
    
    // Copy the stack state
    memcpy(child->kernel_stack, parent->kernel_stack, THREAD_STACK_SIZE);
    memcpy(child->user_stack, parent->user_stack, USER_STACK_SIZE);
        
    // Mapping the user stack to the user virtual address
    unsigned long user_stack_pa = (unsigned long)VIRT_TO_PHYS(child->user_stack);
    if (mappages(child->pgd, USER_STACK_BASE, USER_STACK_SIZE, user_stack_pa) != 0) {
        dfree(child->user_program);
        muart_puts("Error: Failed to map user stack to virtual address space\r\n");
        return 0;
    }
    
    // Mapping the VA 0x3c000000 ~ 0x3fffffff in user mode to 0x3c000000 ~ 0x3fffffff (identity mapping)
    if (mappages(child->pgd, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, PERIPHERAL_START) != 0) {
        muart_puts("Error: Failed to map peripheral memory to user virtual address space\r\n");
        return 0;
    }
    child->parent = parent;
    child->state = TASK_RUNNING;

   
    // Get trap frames
    struct trap_frame* parent_tf = task_tf(parent);
    struct trap_frame* child_tf = task_tf(child);


    // Copy the parent's trap frame to the child's trap frame
    memcpy(child_tf, parent_tf, sizeof(struct trap_frame));

    child_tf->regs[19] = 0;
    child_tf->regs[20] = 0;

    // Adjust child's SP_EL0 to point to child's user stack with same offset
    unsigned long parent_sp_offset = parent_tf->sp_el0 - USER_STACK_BASE;
    child_tf->sp_el0 = USER_STACK_BASE + parent_sp_offset;
    
           
    // Set the return values
    child_tf->regs[0] = 0;  // Child returns 0
    parent_tf->regs[0] = child->pid;  // Parent returns child PID
    
    // Set up child's CPU context
    memzero((unsigned long)&child->cpu_context, sizeof(struct cpu_context));
    child->cpu_context.lr = (unsigned long)ret_to_user; 
    child->cpu_context.sp = (unsigned long)child_tf;
    child->cpu_context.phy_addr_pgd = (unsigned long)VIRT_TO_PHYS(child->pgd);

    
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

// Input will be ch=0x8, mbox=0xffffffffedd0 in lab 6
int sys_mbox_call(unsigned int ch, unsigned int *mbox) {
    disable_irq_in_el1();

    // Get the mailbox size from the first element of the user-provided mailbox buffer 
    unsigned int mailbox_size = mbox[0];   // mbox[0] represents the buffer size in bytes (including the header values, the end tag and padding)
    unsigned int mailbox_num = mailbox_size / 4; // Each content in the mailbox buffer is 32 bits (4 bytes), hence the number of elements in the mailbox buffer is mailbox_size / 4

    // Declare a message buffer for the communication between the CPU and GPU
    volatile unsigned int __attribute__((aligned(16))) mailbox[64]; // 35 in the vm.img case

    memcpy((unsigned int*)&mailbox, mbox, mailbox_size); // Copy the user-provided mailbox buffer to the kernel space mailbox buffer    
    mailbox_call(ch, (volatile unsigned int*)VIRT_TO_PHYS(&mailbox));
    memcpy(mbox, (unsigned int*)&mailbox, mailbox_size); // Copy the mailbox buffer back to the user-provided mailbox buffer
    
    enable_irq_in_el1();
    return 8;
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