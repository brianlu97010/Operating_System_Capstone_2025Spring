#include "muart.h"
#include "exception.h"
#include "utils.h"
#include "registers.h"
#include "timer.h"

void exception_table_init(){
    // Get current EL
    unsigned long el = get_current_el();
    muart_puts("Current Exception Level : EL");
    muart_send_dec(el);
    muart_puts("\r\n");

    // Write the vector table base address to VBR_EL1
    set_exception_vector_table();
    muart_puts("Exception vector table initialized successful !\r\n");

    unsigned long vbar = get_vbar_el1();
    muart_puts("VBAR_EL1: ");
    muart_send_hex(vbar);
    muart_puts("\r\n");
}

void unexpected_irq_handler(){
    muart_puts("Unexpected IRQ \r\n");
}

void svc_handler(){
    // Call the assembly function to read the system register
    unsigned long spsr = get_spsr_el1();
    unsigned long elr = get_elr_el1();
    unsigned long esr = get_esr_el1();
    
    // Print exception information
    muart_puts("\r\n");
    muart_puts("Taken an Exception!\r\n");
    
    muart_puts("SPSR_EL1: ");
    muart_send_hex(spsr);
    muart_puts("\r\n");
    
    muart_puts("ELR_EL1: ");
    muart_send_hex(elr);
    muart_puts("\r\n");
    
    muart_puts("ESR_EL1: ");
    muart_send_hex(esr);
    muart_puts("\r\n");
    
    // Get the ESR.EC (Exception Class) from bits [31:26] (total 6 bits)
    unsigned long ec = (esr >> 26) & 0x3F;   // right shift 26 bits and get the last 6 bits
    
    muart_puts("Cause of exception : ");
    if( ec == 0x15 ){
        muart_puts("SVC instruction exception from AArch64 execution state\r\n");
    }
    else if( ec == 0 ){
        muart_puts("Exceptions with an unknown reason \r\n");
    }
    else{
        muart_puts("other cause \r\n");
    }
}

void irq_entry(void) {
    // Load the value of core 0 interrupt source
    unsigned int irq_status = regRead(CORE0_IRQ_SRC);
    
    // Check if it's a timer interrupt (bit 1 stands for CNTPNSIRQ interrupt)
    if (irq_status & 2){
        // If bit 1 is set, branch to timer-specific handler
        timer_irq_handler();
    }
    // Check if it's a GPU IRQ signals (bit 8 stands for GPU interrupt)
    else if ( irq_status & (1<<8) ){
        // Check if it's an AUX interrupt (bit 29 in IRQ_PEND1 stands for AUX int is pending)
        if ( regRead(IRQ_PEND1) & (1<<29) ){
            // Check if it's the mini UART interrupt (bit 0 stands the mini UART has an interrupt pending)
            if( regRead(AUXIRQ) & 1 ){ 
                // branch to uart-specific IRQ handler
                uart_irq_handler();
            }
            else{
                // If we reach here, it's an unexpected IRQ
                unexpected_irq_handler();         
            }
        }
        else{
            // If we reach here, it's an unexpected IRQ
            unexpected_irq_handler(); 
        }

    }
    else{
        // If we reach here, it's an unexpected IRQ
        unexpected_irq_handler();
    }
}