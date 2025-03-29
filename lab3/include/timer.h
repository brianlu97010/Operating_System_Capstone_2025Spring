#ifndef _TIMER_H
#define _TIMER_H

/* --- C Functions --- */
/* The timer-specific handler : Print elapsed time since boot and reset timer */
void timer_irq_handler(void);


/* --- Assembly Functions --- */
/* Enable the core timer and enable the timer interrupt of the first level interrupt controller */
extern void enable_core_timer(void);
extern void enable_core_timer_int(void);
extern void disable_core_timer_int(void);
extern void reset_timer(void);

/* System register access functions */
extern unsigned long get_cntfrq_el0(void);
extern unsigned long get_cntpct_el0(void);

#endif