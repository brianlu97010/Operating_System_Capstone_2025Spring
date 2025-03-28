#ifndef _TIMER_H
#define _TIMER_H

/* Enable the core timer and enable the timer interrupt of the first level interrupt controller */
extern void enable_core_timer(void);
extern void enable_core_timer_int(void);
extern void disable_core_timer_int(void);

#endif