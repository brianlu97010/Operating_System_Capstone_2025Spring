#include "sched.h"
#include "malloc.h"
#include "list.h"
#include "muart.h"
#include "utils.h"
#include "bitmap.h"
#include "pid.h"
#include "exception.h"
#include "syscall.h"
#include "cpio.h"
#include "mm.h"

/* Thread Mechanism Progress : 
 * use `kernel_thread` to create a new thread and add it to run queue
 * 當 thread 第一次被 scheduling 的時候 ( schedule() 挑到這個 thread )
 * 做 context switch : save current thread 的 cpu context and restore the next thread 的 cpu context
 * 接著 `ret` (Defaults to X30 if absent) 跳到 lr 裡面的 address 也就是 `ret_from_kernel_thread` <- 想成 kernel thread 的 entry point，每一個 context switch 完後的 task 都會從這裡開始執行
 * `ret_from_kernel_thread` 再帶著參數 (x20) 跳到指定的 function (x19)
 */




// Use doubly-linked list to maintain all tasks
struct list_head task_lists;

// Use a doubly-linked list to maintain the run queue
struct list_head rq;

static struct task_struct* idle_task = NULL;       // Idle thread


/* 
 * Create a new kernel thread: initialize the properties of the task struct and add it to the run queue
 * Return the thread ID or -1 if fail   
 */
pid_t kernel_thread(thread_func_t fn, void* arg) {
    struct task_struct* new_task;
    
    // Disable interrupt when creating a new thread (prevent race conditions when accessing global variable like run queue, pid_bitmap and task list)
    disable_irq_in_el1();

    // Allocate a memory space for the task descriptor of new task
    new_task = (struct task_struct*)dmalloc(sizeof(struct task_struct));
    if (!new_task) {
        muart_puts("Failed to allocate task structure\r\n");
        enable_irq_in_el1();
        return -1;
    }

    // Allocate PID
    new_task->pid = pid_alloc();
    if (new_task->pid < 0) {
        dfree(new_task);
        muart_puts("Failed to allocate PID\r\n");
        enable_irq_in_el1();
        return -1;
    }

    // Initialize the properties for the task descriptor of the task
    new_task->state = TASK_RUNNING;
    new_task->parent = (struct task_struct*)get_current_thread();
    
    // Allocate a memory space for the task's kernel stack
    new_task->kernel_stack = dmalloc(THREAD_STACK_SIZE);
    if (!new_task->kernel_stack) {
        pid_free(new_task->pid);
        dfree(new_task);
        muart_puts("Failed to allocate kernel stack\r\n");
        enable_irq_in_el1();
        return -1;
    }

    // Allocate a memory space for the task's user stack
    new_task->user_stack = dmalloc(THREAD_STACK_SIZE);
    if (!new_task->user_stack) {
        muart_puts("Failed to allocate new task user stack\r\n");
        enable_irq_in_el1();
        return -1;
    }

    // Initialize user program fields
    new_task->user_program = NULL;        // Will be set by cpio_load_program
    new_task->user_program_size = 0;      // Will be set by cpio_load_program

    // Set the cpu context of new task
    new_task->cpu_context.sp = (unsigned long)((char*)new_task->kernel_stack + THREAD_STACK_SIZE);          // Set the stack pointer point to the top of the task's kernel stack
    new_task->cpu_context.lr = (unsigned long)ret_from_kernel_thread;                                       // Set the link register store the address of ret_from_kernel_thread
    new_task->cpu_context.x19 = (unsigned long)fn;                                                          // Store the function address into the callee-saved register
    new_task->cpu_context.x20 = (unsigned long)arg;                                                         // Store the function argurment into the callee-saved register

    // Add the new task into the run queue
    INIT_LIST_HEAD(&new_task->list);
    list_add_tail(&new_task->list, &rq);
    /*
     * INIT_LIST_HEAD : 
     * new_task.list
        ├─next──→ new_task.list  // 指向自己
        └─prev──→ new_task.list  // 指向自己 
     
     * Add into tail of run queue : 
       run_queue ←→ task0 ←→ task1 ←→ "new task" ←→ runqueue
     */
    
    // Add the new task into the task list
    INIT_LIST_HEAD(&new_task->task);
    list_add_tail(&new_task->task, &task_lists);

    enable_irq_in_el1();
    return new_task->pid;
}

/* 
 * Choose a thread to run 
 * 目前 schedule() 只會在以下幾種情況被呼叫
 * 1. Thread Voluntary Yielding CPU
 * 2. Thread Exit (called in funtion `thread_exit()`)
 * 3. Timer interrupt (called in `timer_irq_handler()`) 
 */
void schedule(){
    // Disable interrupt when pick a thread in run queue (prevent race conditions when accessing global variable like run queue, pid_bitmap and task list)
    disable_irq_in_el1();

    // debug msg
    // muart_puts("\r\nEnter schedule()\r\n");

    struct task_struct* prev = (struct task_struct*)get_current_thread();
    struct task_struct* next = NULL;    // the next task to run
    struct list_head* ptr;

    // if current thread is not idle task and still runnable, move it to the tail of run queue (RR)
    if( (prev != NULL) && (prev != idle_task) && (prev->state==TASK_RUNNING) ){
        list_del(&prev->list);
        list_add_tail(&prev->list, &rq);
    }

    // If run queue is empty, choose the idle task to run
    if( list_empty(&rq) ){
        next = idle_task;
    }
    // If run queue is non-empty, find the next runnable task 
    else{
        ptr = rq.next;

        // If we reached the end of run queue, wrap around
        if (ptr == &rq) {
            ptr = ptr->next;
        }

        next = list_entry(ptr, struct task_struct, list);
    }
    
    // If no task found, choose idle task
    if (next == NULL) {
        next = idle_task;
    }

    // If next thread to be executing is same as current thread, don't switch
    if (next != prev) {
        // debug
        // muart_puts("Switching from thread ");
        // muart_send_dec(prev ? prev->pid : 0);
        // muart_puts(" to thread ");
        // muart_send_dec(next->pid);
        // muart_puts("\r\n");
        
        // Perform context switch
        cpu_switch_to(prev, next);
        return;
    }
    else {
        // If no switch occurred, just enable interrupt and return
        enable_irq_in_el1();
        return;
    }
}

/* When a thread exit, set the state to ZOMBIE and remove it from the run queue */
void thread_exit(){
    // Disable interrupts when modifying task state and removing from run queue
    // Will enable interrupt when picking the next thread to run (in ret_from_kernel_thread)
    disable_irq_in_el1();

    struct task_struct* current = (struct task_struct*)get_current_thread();
    current->state = TASK_ZOMBIE;

    // Remove from the run queue
    list_del(&current->list);

    muart_puts("Thread ");
    muart_send_dec(current->pid);
    muart_puts(" exited\r\n");
    
    // Pick the next thread to run
    schedule();
    
    // Should never reach here
    while(1) {}
}

/* 
 * The function which the idle task will run
 * When the idle thread is scheduled, it checks if there is any zombie thread : 
 * If yes, it recycles the resources 
 * If there is no zombie task, just yield the CPU
 */
void idle_task_fn(){
    muart_puts("Idle task started\r\n");
    
    while(1) {
        disable_irq_in_el1();

        // Check if there is any zombie task in the task list
        // If there is no zombie task, just yield the CPU
        // If there is a zombie task, clean it  up
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
                if (zombie->kernel_stack) {
                    dfree(zombie->kernel_stack);
                }

                if (zombie->user_stack) {
                    dfree(zombie->user_stack);
                }

                // Free the zombie task's user program memory
                if (zombie->user_program) {
                    muart_puts("  Freeing user program (size: ");
                    muart_send_dec(zombie->user_program_size);
                    muart_puts(" bytes)\r\n");
                    dfree(zombie->user_program);
                    zombie->user_program = NULL;
                    zombie->user_program_size = 0;
                }
                
                // Free the zombie task's pid
                pid_free(zombie->pid);

                // Mark the state of dead
                zombie->state = TASK_DEAD;
                
                // Remove from task list 
                list_del(&zombie->task);
                
                // Free the zombie itself
                dfree(zombie);
            }
        }
       
        // Yield the CPU, pick the next thread to run
        schedule();
    }
}

/* Create and initialize the idle task */
static void create_idle_task(){
    disable_irq_in_el1();

    // Allocate the memory space of idle task
    idle_task = (struct task_struct*)dmalloc(sizeof(struct task_struct));
    if (!idle_task) {
        muart_puts("Failed to allocate idle task\r\n");
        enable_irq_in_el1();
        return;
    }
    
    idle_task->kernel_stack = dmalloc(THREAD_STACK_SIZE);
    if (!idle_task->kernel_stack) {
        muart_puts("Failed to allocate idle task kernel stack\r\n");
        enable_irq_in_el1();
        return;
    }

    idle_task->user_stack = dmalloc(THREAD_STACK_SIZE);
    if (!idle_task->user_stack) {
        muart_puts("Failed to allocate idle task user stack\r\n");
        enable_irq_in_el1();
        return;
    }
    
    idle_task->pid = 0;
    idle_task->parent = NULL;
    idle_task->state = TASK_RUNNING;
    
    // Initialize user program fields for idle task (not used, but for consistency)
    idle_task->user_program = NULL;
    idle_task->user_program_size = 0;

    // Set the cpu context of idle task
    idle_task->cpu_context.sp = (unsigned long)((char*)idle_task->kernel_stack + THREAD_STACK_SIZE);          // Set the stack pointer point to the top of the task's kernel stack
    idle_task->cpu_context.lr = (unsigned long)ret_from_kernel_thread;                                        // Set the link register store the address of ret_from_kernel_thread
    idle_task->cpu_context.x19 = (unsigned long)idle_task_fn;                                                 // Store the function address into the callee-saved register
    INIT_LIST_HEAD(&idle_task->list);
    
    // Add the idle task into the task list
    INIT_LIST_HEAD(&idle_task->task);
    list_add_tail(&idle_task->task, &task_lists);

    enable_irq_in_el1();
}

/* Initialize the thread mechanism */
void sched_init(){
    // Initialize task lists
    INIT_LIST_HEAD(&task_lists);

    // Initialize runqueue
    INIT_LIST_HEAD(&rq);

    // Initialize bitmap of PID management
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
        struct task_struct* current = (struct task_struct*)get_current_thread();
        muart_puts("Thread id : ");
        muart_send_dec(current->pid);
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

/* Basic Exercise 2 */
// 等下要把 strcut trap_frame 的 data 放到 task's 的 kernel stack 的最上層
// Get pointer to trap_frame at the top of a task's kernel stack
struct trap_frame* task_tf(struct task_struct* tsk) {
    // trap frame is stored at the top of the kernel stack
    unsigned long p = (unsigned long)((char*)tsk->kernel_stack + THREAD_STACK_SIZE - TRAP_FRAME_SIZE);
    return (struct trap_frame*)p;
}

// Move the current thread to user mode and execute the user program
int move_to_user_mode(unsigned long user_program_addr) {
    disable_irq_in_el1();
    struct task_struct* current = (struct task_struct*)get_current_thread();
    struct trap_frame* regs = task_tf(current);
    
    // Initialize the trap frame
    memzero((unsigned long)regs, sizeof(*regs));
    
    // Debug user function address
    muart_puts("User function address check:\r\n");
    muart_puts("Function address: ");
    muart_send_hex((unsigned int)user_program_addr);
    muart_puts("\r\n");
    
    // muart_puts("First bytes of function: ");
    // unsigned char* func = (unsigned char*)pc;
    // for (int i = 0; i < 8; i++) {
    //     muart_send_hex(func[i]);
    //     muart_puts(" ");
    // }
    // muart_puts("\r\n");
    

    __asm__(
        "msr tpidr_el1, %0\n\t" // Hold the "kernel(el1)" thread structure information
        "msr elr_el1, %1\n\t"   // Store the user function address into elr_el1
        "msr sp_el0, %2\n\t"
        "mov sp, %3\n\t"        // stack pointer set to the top of the task's kernel stack
        "msr spsr_el1, xzr\n\t"   // Enable interrupt in EL0 and use user's stack pointer
        "eret\n\t" ::           // Switch to EL0, jump to elr_el1 (user program address)
        "r"(current),
        "r"(user_program_addr), 
        "r"(current->user_stack + THREAD_STACK_SIZE),
        "r"(current->kernel_stack + THREAD_STACK_SIZE)
    ); 

    return 0;
}

void sys_get_pid_test() {
    muart_puts("\r\nFork Test, pid ");
    int pid = call_sys_getpid();
    muart_send_dec(pid);
    muart_puts("\r\n");

    // // Test the UART system call
    // char write_buf[] = "Hello, UART!\n";
    // char read_buf[64];
    
    // // Test write
    // size_t written = call_sys_uartwrite(write_buf, sizeof(write_buf) - 1);
    
    // // Test read (this will block waiting for input)
    // call_sys_uartwrite("Enter some text: ", 17);
    // size_t read_count = call_sys_uartread(read_buf, 2);    // Read up to 10 bytes, it must enter 10 characters to exit (just for test)
    
    // // Echo back what was read
    // call_sys_uartwrite("You entered: ", 13);
    // call_sys_uartwrite(read_buf, read_count);
    // call_sys_uartwrite("\n", 1);

    int cnt = 1;
    int ret = 0;
    if ((ret = call_sys_fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        // printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt, &cnt, cur_sp);
        muart_puts("first child pid: ");
        muart_send_dec(call_sys_getpid());
        muart_puts(", cnt: ");
        muart_send_dec(cnt);
        muart_puts(", ptr: ");
        muart_send_hex((unsigned int)cnt);
        muart_puts(", sp: ");
        muart_send_hex((unsigned int)cur_sp);
        muart_puts("\r\n");
        ++cnt;

        if ((ret = call_sys_fork()) != 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            // printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt, &cnt, cur_sp);
            muart_puts("first child pid: ");
            muart_send_dec(call_sys_getpid());
            muart_puts(", cnt: ");
            muart_send_dec(cnt);
            muart_puts(", ptr: ");
            muart_send_hex((unsigned int)cnt);
            muart_puts(", sp: ");
            muart_send_hex((unsigned int)cur_sp);
            muart_puts("\r\n");
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                // printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt, &cnt, cur_sp);
                muart_puts("second child pid: ");
                muart_send_dec(call_sys_getpid());
                muart_puts(", cnt: ");
                muart_send_dec(cnt);
                muart_puts(", ptr: ");
                muart_send_hex((unsigned int)cnt);
                muart_puts(", sp: ");
                muart_send_hex((unsigned int)cur_sp);
                muart_puts("\r\n"); 
                waitCycle(1000000);
                ++cnt;
            }
        }
        call_sys_exit();
    }
    else {
        // printf("parent here, pid %d, child %d\n", get_pid(), ret);
        muart_puts("parent here, pid ");
        muart_send_dec(call_sys_getpid());
        muart_puts(", child ");
        muart_send_dec(ret);
        muart_puts("\r\n");
    }

    // user thread exit
    call_sys_exit();
}

// Kernel thread function that will move to user mode
void kernel_fork_process() {
    muart_puts("Kernel process executing at EL ");
    muart_send_dec(get_current_el());
    muart_puts("\r\n");
    muart_puts("Kernel process started. Preparing to move to user mode...\r\n");
    int err = move_to_user_mode((unsigned long)&sys_get_pid_test);
    if (err < 0) {
        muart_puts("Error while moving process to user mode\r\n");
    }
}

/* Create a kernel thread execute `syscall.img` in EL0 */

// Modified kernel thread function that uses cpio_exec approach
void kernel_fork_process_cpio(void* arg) {
    struct task_init_data* init_data = (struct task_init_data*)arg;
    if (!init_data) {
        muart_puts("Error: No initialization data provided\r\n");
        return;
    }
    
    muart_puts("Kernel process executing at EL ");
    muart_send_dec(get_current_el());
    muart_puts("\r\n");
    muart_puts("Loading ");
    muart_puts(init_data->filename);
    muart_puts(" from initramfs at ");
    muart_send_hex(init_data->initramfs_addr);
    muart_puts("\r\n");
    
    // Load program using provided initramfs address
    unsigned long user_program_addr = cpio_load_program((const void*)init_data->initramfs_addr, init_data->filename);
    
    // Free initialization data
    dfree(init_data);
    
    if (user_program_addr == 0) {
        muart_puts("Error: Failed to load program from initramfs\r\n");
        call_sys_exit();
        return;
    }
    
    muart_puts("Successfully loaded program at address: ");
    muart_send_hex((unsigned int)user_program_addr);
    muart_puts("\r\n");
    
    // Move to user mode
    int err = move_to_user_mode(user_program_addr);
    if (err < 0) {
        muart_puts("Error while moving process to user mode\r\n");
    }
}