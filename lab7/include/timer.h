#ifndef _TIMER_H
#define _TIMER_H

/* --- Type definition --- */
/* Callback function of specific timer tasks */
typedef void (*timer_callback_t)(void* data);

/* Timer structure */
typedef struct timer_t{
    unsigned long expired_time;     // Expired time
    timer_callback_t callback;      // Callback function
    void* data;                     // Data passed to the callback function
    struct timer_t* next;                   // Point to the next timer structure
}timer_t;

/* Timeout message structure */
typedef struct {
    char message[128];
    unsigned long creation_time;
} timeout_message_t;


/* --- C Functions --- */
/* Enable the core timer and disable the core timer interrupt at initialization first */
void core_timer_init(void);

/* The timer-specific handler */
void timer_irq_handler(void);

/* Timer basic specific IRQ handler: Print elapsed time since boot and reset timer  */
void timer_basic_irq_handler(void);

/* The API for adding the new timer with specific callback function and seconds */
void addTimer(timer_callback_t callback, unsigned int seconds, void* data);

/* Timer Multiplexing specific IRQ handler */
void timer_mul_irq_handler();

/* Get the elapsed time since boot as system time */
unsigned long get_system_time();

int setTimeout(const char* message, unsigned int seconds);


/* --- Assembly Functions --- */
/* Enable the core timer and enable the timer interrupt of the first level interrupt controller */
extern void enable_core_timer(void);
extern void enable_core_timer_int(void);
extern void disable_core_timer_int(void);
extern void set_core_timer(void);

/* System register access functions */
extern unsigned long get_cntfrq_el0(void);
extern unsigned long get_cntpct_el0(void);

#endif