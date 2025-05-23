#include "muart.h"
#include "timer.h"
#include "exception.h"
#include "string.h"
#include "malloc.h"

static timer_t* timer_list = NULL;

/* Enable the core timer and disable the core timer interrupt at initialization first */
void core_timer_init() {
    // Enable the core timer
    enable_core_timer();
    disable_core_timer_int();
    
    // Reset timer with frequency shifted right by 5 bits
    asm volatile (
        "mrs x0, cntfrq_el0\n\t"  // Read the frequency of the system counter
        "lsr x0, x0, #5\n\t"      // Shift right by 5 bits
        "msr cntp_tval_el0, x0\n\t" // Set CNTP_TVAL_EL0 for next timer interrupt
    );

    // Let the user program in EL0 can directly access the frequency register and physical counter register in EL1 
    unsigned long tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
}


/* Get the elapsed time since boot as system time */
unsigned long get_system_time(){
    // Get elapsed time since boot
    unsigned long timer_freq = get_cntfrq_el0();
    unsigned long timer_count = get_cntpct_el0();
    
    return timer_count / timer_freq;
}

/* Timer basic specific IRQ handler: Print elapsed time since boot and reset timer  */
void timer_basic_irq_handler(){
    // Print elapsed time
    muart_puts("Time since boot: ");
    muart_send_dec(get_system_time());
    muart_puts(" seconds\r\n");
    
    // Reset timer for next interrupt
    set_core_timer();
}

/* The timer-specific handler */
void timer_irq_handler(void){
    // If Timer Queue has timer, use the timer multiplexing
    if (timer_list != NULL) {
        timer_mul_irq_handler();
    }  
    // If Timer Queue has no timer, use the basic core timer IRQ handler for basic exercise 2
    else{
        timer_basic_irq_handler();
    }
}


/* Insert new timer into the timer queue */
static void insert_timer(timer_t* new_timer){
    // Enter Critical Section to protect the global timer queue
    disable_irq_in_el1();

    // Timer Queue is empty
    if( timer_list == NULL ){
        timer_list = new_timer;
        timer_list->next = NULL;

        // Reset the expired time of core timer
        unsigned long timer_freq = get_cntfrq_el0();
        unsigned long diff_count = timer_freq * (timer_list->expired_time - get_system_time());
        __asm__ volatile(
            "msr cntp_tval_el0, %0"
            :: "r" (diff_count)
        );

        // Enable the timer interrupt of the first level interrupt controller 
        enable_core_timer_int();
    }
    // The expired time of the new timer is the smallest of the Timer Queue
    else if( new_timer->expired_time < timer_list->expired_time ){
        // Insert the new timer at the front of the Timer Queue
        new_timer->next = timer_list;
        timer_list = new_timer;

        // Reset the expired time of core timer
        unsigned long timer_freq = get_cntfrq_el0();
        unsigned long diff_count = timer_freq * (timer_list->expired_time - get_system_time());
        __asm__ volatile(
            "msr cntp_tval_el0, %0"
            :: "r" (diff_count)
        );
    }
    // Find the appropriate place in Timer Queue
    else{
        timer_t* current = timer_list;
        // Find first node in the Timer Queue which the expired time is smaller than new_timer until reaching the tail of queue
        while( current->next != NULL && (current->next)->expired_time <= new_timer->expired_time ){
            current = current->next;    // Advance the current node 
        }

        // Insert the new timer
        new_timer->next = current->next;
        current->next = new_timer;
    }

    // Exit Critical Section
    enable_irq_in_el1();
}

/* Timer Multiplexing specific IRQ handler */
void timer_mul_irq_handler(){
    unsigned long current_time = get_system_time();

    // Execute all expired timer (some might be time-sensative)
    while( timer_list != NULL && timer_list->expired_time <= current_time ){
        timer_t* expired_timer = timer_list;
        timer_list = timer_list->next;

        timer_callback_t callback = expired_timer->callback;
        void* data = expired_timer->data;

        // Exit Critical Section, for nested loop (Todo)
        // enable_irq_in_el1();

        // execute the callback function
        callback(data);

        // Enter Critical Section to protect the global timer queue
        // disable_irq_in_el1();
    }

    // If Timer Queue exists timer not executed, reset the core timer
    if( timer_list != NULL ){
        // Reset the expired time of core timer
        unsigned long timer_freq = get_cntfrq_el0();
        unsigned long diff_count = timer_freq * (timer_list->expired_time - get_system_time());
        __asm__ volatile(
            "msr cntp_tval_el0, %0"
            :: "r" (diff_count)
        );
    }
    // If no more timer, mask the timer interrupt. Avoid the unexpected timeout
    else{
        disable_core_timer_int();
    }
  
    return;
}

/* The API for adding the new timer with specific callback function and seconds */
void addTimer(timer_callback_t callback, unsigned int seconds, void* data){
    // Use a pre-allocated memory pool to allocate the memory space of new timer
    timer_t* new_timer = (timer_t*)simple_alloc(sizeof(timer_t));

    if (new_timer == NULL){
        muart_puts("Error: Failed to allocate memory for timer\r\n");
        return;
    }

    // Set the properties of timer
    new_timer->expired_time = get_system_time() + seconds;  // The printing order is determined by the command executed time and the user-specified SECONDS
    new_timer->callback = callback;
    new_timer->data = data;
    new_timer->next = NULL;

    // Insert to the Timer Queue
    insert_timer(new_timer);
}

/* Callback Function of timeout interrupt - Print the message */
static void print_timeout_message(void* data){
    // Casting to the structure of time out message
    timeout_message_t* msg = (timeout_message_t*)data;
    
    unsigned long now = get_system_time();
    unsigned long elapsed = now - msg->creation_time;
    
    muart_puts("\r\n");
    muart_puts("===== Message =====\r\n");
    muart_puts("Message:\t");
    muart_puts(msg->message);
    muart_puts("\r\n");
    muart_puts("Create time:\t");
    muart_send_dec(msg->creation_time);
    muart_puts(" sec\r\n");
    muart_puts("Current time:\t");
    muart_send_dec(now);
    muart_puts(" sec\r\n");
    muart_puts("Elapsed time:\t");
    muart_send_dec(elapsed);
    muart_puts(" sec\r\n");
    muart_puts("====================\r\n");
}


int setTimeout(const char* message, unsigned int seconds){
    timeout_message_t* msg = (timeout_message_t*)simple_alloc(sizeof(timeout_message_t));

    if (msg == NULL) {
        muart_puts("Error: Failed to allocate memory for message\r\n");
        return -1;
    }
    
    strncpy(msg->message, message, sizeof(msg->message)-1);
    // Null-terminate the destination string
    msg->message[sizeof(msg->message) - 1] = '\0';
    msg->creation_time = get_system_time();
    
    disable_core_timer_int();
    addTimer(print_timeout_message, seconds, msg);
    enable_core_timer_int();
    return 0;
}
