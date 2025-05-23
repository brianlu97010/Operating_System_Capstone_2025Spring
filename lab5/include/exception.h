#ifndef _EXCEPTION_H
#define _EXCEPTION_H


#define CORE0_IRQ_SRC        0x40000060     // Core 0 interrupt source 
#define TRAP_FRAME_SIZE      272            // Size of trap frame

#ifndef __ASSEMBLER__

// Trap frame
struct trap_frame {
    unsigned long regs[31];  // General purpose registers x0-x30
    unsigned long sp;        // Stack pointer
    unsigned long pc;        // Program counter
    unsigned long pstate;    // Processor state
};

/* --- C functions --- */
/* The API to initialize the exception vector table */
void exception_table_init(void);

/* The SVC-specific handler: print the system reg's content */ 
void svc_handler(void);

/* High-level IRQ handler, determines the interrupt source and calls the specific handler */
void irq_entry(void); 



/* --- Assembly functions --- */
/* Switch to EL0 and execute the user program at EL0 */
extern void exec_user_program(void* program_addr);

/* Set exception vector table */
extern void set_exception_vector_table(void);

/* Enable IRQ in EL1 */
extern void enable_irq_in_el1(void);

/* Disable IRQ in EL1 */
extern void disable_irq_in_el1(void);

/* System register access functions */
extern unsigned long get_spsr_el1(void);
extern unsigned long get_elr_el1(void);
extern unsigned long get_esr_el1(void);
extern unsigned long get_vbar_el1(void);
extern unsigned long get_current_el(void);

/* Enble / Disable el1 interrupts */
extern void enable_irq_in_el1(void);
extern void disable_irq_in_el1(void);
#endif

#endif