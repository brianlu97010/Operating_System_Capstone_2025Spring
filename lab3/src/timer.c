#include "muart.h"
#include "timer.h"

// Handle timer interrupt
void timer_irq_handler(void){
    // Get elapsed time since boot
    unsigned long timer_freq = get_cntfrq_el0();
    unsigned long timer_count = get_cntpct_el0();
    unsigned long elapsed_seconds = timer_count / timer_freq;
    
    // Print elapsed time
    muart_puts("Time since boot: ");
    muart_send_dec(elapsed_seconds);
    muart_puts(" seconds\r\n");
    
    // Reset timer for next interrupt
    reset_timer();
}