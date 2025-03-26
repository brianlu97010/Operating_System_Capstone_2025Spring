#ifndef _EXCEPTION_H
#define _EXCEPTION_H

/* --- C functions --- */
/* The API to initialize the exception vector table and enable the core's timer interrupt*/
void exception_init(void);

/* The SVC-specific handler: print the system reg's content */ 
void svc_handler(void);

/* The timer-specific handler : Print elapsed time since boot and reset timer */
void timer_irq_handler(void);



/* --- Assembly functions --- */
/* Switch to EL0 and execute the user program at EL0 */
void exec_user_program(void* program_addr);

/* Set exception vector table */
void set_exception_vector_table(void);

/* Enable the core timer and enable the timer interrupt of the first level interrupt controller */
void enable_core_timer(void);

/* System register access functions */
unsigned long get_spsr_el1(void);
unsigned long get_elr_el1(void);
unsigned long get_esr_el1(void);
unsigned long get_vbar_el1(void);
unsigned long get_cntfrq_el0(void);
unsigned long get_cntpct_el0(void);
unsigned long get_current_el(void);


#endif