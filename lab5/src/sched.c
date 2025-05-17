#include "sched.h"
#include "malloc.h"
#include "list.h"
#include "muart.h"
#include "utils.h"
#include "bitmap.h"
#include "pid.h"
/* 先做 non-preemtive 的 schedule */

// Thread Create Progress : 
// use `kernel_thread` to create a new thread and add it to run queue
// 當 thread 第一次被 scheduling 的時候 ( schedule() 挑到這個 thread )
// 做 context switch : save current thread 的 cpu context and restore the next thread 的 cpu context
// 接著 `ret` (Defaults to X30 if absent) 跳到 lr 裡面的 address 也就是 `ret_from_kernel_thread`，
// `ret_from_kernel_thread` 再帶著參數跳到指令的 function
// 可以把 `ret_from_kernel_thread` 想成是一個 entry point，每一個 context switch 完後的 task 都會從這裡開始執行

// 先預先定義整個 system 只能有 64 個 tasks，然後用一個 static global array maintain (之後考慮用 task list ?)
// #define NR_TASKS 64

// Using doubly-linked list to maintain all task list
struct list_head task_lists;

// static struct task_struct* tasks[NR_TASKS];
// static int nr_tasks = 0;
// PID 先這樣實作 : 每次 create 新的 thread， pid 就會++
// static int next_pid = 1;    // PID 0 is reserved for the idle task

// PID bitmap for managing PIDs
static pid_bitmap_t pid_bitmap;

// run queue 先只用一個 list 維護
// Run queue use a circular doubly linked list mantain
struct list_head rq;

static struct task_struct* idle_task = NULL;       // Idle thread

// Functions that thread needs to do
typedef void (*thread_func_t)(void *);

/* --- Function defined in sched.S --- */
/* Jump to the address in x19, with the argument in x20 */
extern void ret_from_kernel_thread();

/* Switch from the prev thread to the next thread */
extern void cpu_switch_to(struct task_struct* prev, struct task_struct* next);

/* Get the current thread from the system register tpidr_el1 */
extern unsigned long get_current_thread();


/* 
 * Create a new kernel thread: initialize the properties of the task struct and add it to the run queue
 * Return the thread ID or -1 if fail   
 */
pid_t kernel_thread(thread_func_t fn, void* arg) {
    struct task_struct* new_task;

    // Allocate a memory space for the task descriptor of this task
    new_task = (struct task_struct*)dmalloc(sizeof(struct task_struct));
    if (!new_task) {
        muart_puts("Failed to allocate task structure\r\n");
        return -1;
    }

    // Allocate PID
    new_task->pid = pid_alloc();
    if (new_task->pid < 0) {
        dfree(new_task);
        muart_puts("Failed to allocate PID\r\n");
        return -1;
    }

    // Initialize the properties for the task descriptor of the task
    new_task->state = TASK_RUNNING;
    new_task->parent = (struct task_struct*)get_current_thread();
    
    // Allocate a memory space for the task's stack
    new_task->stack = dmalloc(THREAD_STACK_SIZE);
    if (!new_task->stack) {
        pid_free(new_task->pid);
        dfree(new_task);
        muart_puts("Failed to allocate stack\r\n");
        return -1;
    }

    // Set the cpu context of idle task
    new_task->cpu_context.sp = (unsigned long)((char*)new_task->stack + THREAD_STACK_SIZE);          // Set the stack pointer point to the top of the task's stack
    new_task->cpu_context.lr = (unsigned long)ret_from_kernel_thread;                           // Set the link register store the address of ret_from_kernel_thread
    new_task->cpu_context.x19 = (unsigned long)fn;                                     // Store the function address into the callee-saved register
    new_task->cpu_context.x20 = (unsigned long)arg;                                    // Store the function argurment into the callee-saved register

    // Add the new task into the run queue
    /*
     * new_task.list
        ├─next──→ new_task.list  // 指向自己
        └─prev──→ new_task.list  // 指向自己 
     
     * Add in to tail of run queue : 
       runqueue ←→ idle_thread ←→ task0 ←→ "new task" ←→ (back to runqueue)
     */
    INIT_LIST_HEAD(&new_task->list);
    list_add_tail(&new_task->list, &rq);

    // Add the new task into the task list
    INIT_LIST_HEAD(&new_task->task);
    list_add_tail(&new_task->task, &task_lists);

    // Store into the task arrays
    // if (nr_tasks < NR_TASKS) {
    //     tasks[nr_tasks++] = new_task;
    // }

    return new_task->pid;
}

/* Choose a thread to run 
 * 目前 schedule() 會在以下幾種情況被呼叫，現在只有實現 non-preemtive
 * 1. Thread Voluntary Yielding CPU
 * 2. Thread Exit (called in funtion `thread_exit()`)
 */
void schedule(){
    struct task_struct* prev = (struct task_struct*)get_current_thread();
    struct task_struct* next = NULL;    // the next task to run
    struct list_head* ptr;

    // if current thread is not idle task and still runnable, move it to the tail of run queue
    if( (prev != NULL) && (prev != idle_task) && (prev->state==TASK_RUNNING) ){
        list_del(&prev->list);
        list_add_tail(&prev->list, &rq);
    }

    // if run queue is empty, choose the idle task to run
    if( list_empty(&rq) ){
        next = idle_task;
    }
    else{
        // Find the next runnable task (現在沒有 time slice，需要靠 task 主動放棄 CPU)
        // Todo : 時間片機制、時鐘中斷、搶占式切換
        ptr = rq.next;

        /* If we reached the end of run queue, wrap around */
        if (ptr == &rq) {
            ptr = ptr->next;
        }

        next = list_entry(ptr, struct task_struct, list);
    }
    
    /* If no task found, choose idle task */
    if (next == NULL) {
        next = idle_task;
    }

    /* Don't switch to the same task */
    if (next != prev) {
        muart_puts("Switching from thread ");
        muart_send_dec(prev ? prev->pid : 0);
        muart_puts(" to thread ");
        muart_send_dec(next->pid);
        muart_puts("\r\n");
        
        /* Perform context switch */
        cpu_switch_to(prev, next);
    }
}

/* When a thread exit : set the state to ZOMBIE and remove from the run queue */
void thread_exit(){
    struct task_struct* cur = (struct task_struct*)get_current_thread();
    cur->state = TASK_ZOMBIE;

    // Remove from the run queue
    list_del(&cur->list);

    muart_puts("Thread ");
    muart_send_dec(cur->pid);
    muart_puts(" exited\r\n");
    
    /* Schedule to run another task */
    schedule();
    
    /* Should never reach here */
    while(1) {}
}

/* When the idle thread is scheduled, it checks if there is any zombie thread. If yes, it recycles them as follows. */
void idle_task_fn(){
    muart_puts("Idle task started\r\n");
    
    while(1) {
        /* 從全部的 task list 中 check 有沒有 zombie tasks and clean them up */
        // for (int i = 0; i < nr_tasks; i++) {
        //     if (tasks[i] && tasks[i]->state == TASK_ZOMBIE) {
        //         struct task_struct* zombie = tasks[i];
                
        //         muart_puts("Cleaning up zombie thread ");
        //         muart_send_dec(zombie->pid);
        //         muart_puts("\r\n");
                
        //         /* Free resources : free the task's stack */
        //         if (zombie->stack) {
        //             /* In a real system, we'd free the stack here */
        //             /* free(zombie->stack); */
        //             dfree(zombie->stack);
        //         }
                
        //         /* Mark as fully dead */
        //         zombie->state = TASK_DEAD;
                
        //         /* Remove from tasks array */
        //         tasks[i] = NULL;
        //         nr_tasks--;
        //     }
        // }
        /* 從全部的 task list 中 check 有沒有 zombie tasks and clean them up */
        struct list_head* pos;
        struct list_head* tmp;
        struct task_struct* zombie;

        list_for_each_safe(pos, tmp, &task_lists) {
            // Find the zombie task
            zombie = list_entry(pos, struct task_struct, task);
            
            if (zombie->state == TASK_ZOMBIE) {
                muart_puts("Cleaning up zombie thread ");
                muart_send_dec(zombie->pid);
                muart_puts("\r\n");
                
                // Free the resoures : free the zombie task's stack
                if (zombie->stack) {
                    dfree(zombie->stack);
                }
                
                // Mark as fully dead
                zombie->state = TASK_DEAD;
                
                // Remove from task list 
                list_del(&zombie->task);
                
                // Free the zombie itself
                dfree(zombie);
            }
        }
        
        // Yield the CPU
        schedule();
    }
}

/* Create and initialize the idle task */
static void create_idle_task(){
    // Allocate the memory space of idle task
    idle_task = (struct task_struct*)dmalloc(sizeof(struct task_struct));
    if (!idle_task) {
        muart_puts("Failed to allocate idle task\r\n");
        return;
    }
    
    idle_task->stack = dmalloc(THREAD_STACK_SIZE);
    if (!idle_task->stack) {
        muart_puts("Failed to allocate idle task stack\r\n");
        return;
    }
    
    idle_task->pid = 0;
    idle_task->parent = NULL;
    idle_task->state = TASK_RUNNING;
    
    // Set the cpu context of idle task
    idle_task->cpu_context.sp = (unsigned long)((char*)idle_task->stack + THREAD_STACK_SIZE);          // Set the stack pointer point to the top of the task's stack
    idle_task->cpu_context.lr = (unsigned long)ret_from_kernel_thread;                           // Set the link register store the address of ret_from_kernel_thread
    idle_task->cpu_context.x19 = (unsigned long)idle_task_fn;                                     // Store the function address into the callee-saved register
    INIT_LIST_HEAD(&idle_task->list);
    INIT_LIST_HEAD(&idle_task->task);
    // Add the idle task into the task list
    list_add_tail(&idle_task->task, &task_lists);
    
    // /* Store in tasks array */
    // if (nr_tasks < NR_TASKS) {
    //     tasks[nr_tasks++] = idle_task;
    // }
    // 初始化並加入執行佇列
}

void sched_init(){
    // Initialize task lists
    INIT_LIST_HEAD(&task_lists);

    // Initialize runqueue
    INIT_LIST_HEAD(&rq);

    // Initialize PID bitmap
    pid_bitmap_init();

    // Create an idle task
    create_idle_task();
    
    // Set the current thread be the idle task
    __asm__ volatile("msr tpidr_el1, %0"::"r" (idle_task));

    muart_puts("Thread scheduler initialized\r\n");
}

/* Basic Exercise 1 : Test the thread */
void foo(void* data){
    for(int i = 0; i < 10; ++i){
        struct task_struct* cur = (struct task_struct*)get_current_thread();
        muart_puts("Thread id : ");
        muart_send_dec(cur->pid);
        muart_puts(", ");
        muart_send_dec(i);
        muart_puts("\r\n");
        waitCycle(1000000);
        schedule();
    }
}

void thread_test(){
    sched_init();

    // Create 3 threads
    for(int i = 0; i < 3; ++i){ 
        kernel_thread(foo, NULL);
    }

    idle_task_fn();

    while(1) {};
}