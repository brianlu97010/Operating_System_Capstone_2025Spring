#ifndef _SCHED_H
#define _SCHED_H

#include "types.h"
#include "list.h"
#include "pid.h"

#define THREAD_STACK_SIZE 4096

/* Task states */
#define TASK_RUNNING    0
#define TASK_ZOMBIE     1
#define TASK_DEAD       2

/* Functions that thread needs to do */
typedef void (*thread_func_t)(void *);

/* CPU context structure */
struct cpu_context {
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;      // x29
    unsigned long lr;      // x30
    unsigned long sp;
};

/* Task structure */
struct task_struct {
    struct cpu_context cpu_context; 
    pid_t pid;
    int state;
    struct task_struct* parent;
    void* kernel_stack;
    void* user_stack;
    void* user_program;           // Pointer to allocated user program memory
    size_t user_program_size;     // Size of user program for cleanup
    struct list_head list;    // For run queue
    struct list_head task;    // For task list
};

/* Pass initramfs address through task structure */
struct task_init_data {
    const char* filename;
    unsigned long initramfs_addr;
};


// External variables (defined in sched.c and will be used in other files) 
extern struct list_head task_lists;
extern struct list_head rq;


/* Initialize the thread mechanism */
void sched_init(void);

/* 
 * Create a new kernel thread: initialize the properties of the task struct and add it to the run queue
 * Return the thread ID or -1 if fail   
 */
pid_t kernel_thread(void (*fn)(void*), void* arg);

/* 
 * Choose a thread to run 
 * 目前 schedule() 只會在以下幾種情況被呼叫
 * 1. Thread Voluntary Yielding CPU
 * 2. Thread Exit (called in funtion `thread_exit()`)
 */
void schedule(void);

/* When a thread exit, set the state to ZOMBIE and remove it from the run queue */
void thread_exit(void);


/* --- Function defined in sched.S --- */
/* Jump to the address in x19, with the argument in x20 */
void ret_from_kernel_thread();

/* Switch from the prev thread to the next thread */
void cpu_switch_to(struct task_struct* prev, struct task_struct* next);

/* Get the current thread from the system register tpidr_el1 */
unsigned long get_current_thread();

#endif
