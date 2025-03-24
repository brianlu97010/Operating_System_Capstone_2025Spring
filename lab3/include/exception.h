#ifndef _EXCEPTION_H
#define _EXCEPTION_H

/* --- C functions --- */
/* The API to initialize the exception vector table */
void exception_init();

/* The SVC-specific handler: print the system reg's content */ 
void exception_entry(void);


/* --- Assembly functions --- */
/* Switch to EL0 and execute the user program at EL0 */
void exec_user_program(void* program_addr);

/* Set exception vector table */
void set_exception_vector_table(void);

/* System register access functions */
unsigned long get_spsr_el1(void);
unsigned long get_elr_el1(void);
unsigned long get_esr_el1(void);
unsigned long get_vbar_el1(void);

#endif