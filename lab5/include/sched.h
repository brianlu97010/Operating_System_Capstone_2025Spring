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
    void* stack;
    struct list_head list;    // For run queue
    struct list_head task;    // For task list
};

void sched_init(void);
pid_t kernel_thread(void (*fn)(void*), void* arg);
void schedule(void);
void thread_exit(void);
unsigned long get_current_thread(void);

#endif
