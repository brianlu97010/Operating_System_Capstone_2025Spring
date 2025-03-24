#include "exception.h"
#include "muart.h"

void exception_init(){
    // Call the assembly function to set the vector table base address to VBR_EL1
    set_exception_vector_table();
    muart_puts("Exception handling initialized successful !\r\n");

    // Call the assembly function to read the system register
    unsigned long vbar = get_vbar_el1();
    muart_puts("VBAR_EL1: 0x");
    muart_send_hex(vbar);
    muart_puts("\r\n");
}

void exception_entry(){
    // Call the assembly function to read the system register
    unsigned long spsr = get_spsr_el1();
    unsigned long elr = get_elr_el1();
    unsigned long esr = get_esr_el1();
    
    // Print exception information
    muart_puts("\r\n");
    muart_puts("Taken an Exception!\r\n");
    
    muart_puts("SPSR_EL1: 0x");
    muart_send_hex(spsr);
    muart_puts("\r\n");
    
    muart_puts("ELR_EL1: 0x");
    muart_send_hex(elr);
    muart_puts("\r\n");
    
    muart_puts("ESR_EL1: 0x");
    muart_send_hex(esr);
    muart_puts("\r\n");
    
    // Get the ESR.EC (Exception Class) from bits [31:26] (total 6 bits)
    unsigned long ec = (esr >> 26) & 0x3F;   // right shift 26 bits and get the last 6 bits
    
    if ( ec == 0x15 ){
        muart_puts("SVC instruction exception from AArch64 execution state\r\n");
    }
}