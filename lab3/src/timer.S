
.global enable_core_timer
.global enable_core_timer_int
.global disable_core_timer_int

#define CORE0_TIMER_IRQ_CTRL 0x40000040     // Core 0 Timers interrupt control

// Enable the core timer
enable_core_timer:
    // Set the cntp_ctl_el0.ENABLE bit to 1 i.e, Timer enabled
    mov x0, 1
    msr cntp_ctl_el0, x0

    // Set expired time for 2 sec, the timer interrupt asserted when CNTP_CVAL_EL0 >= CNTPCT_EL0
    mrs x0, cntfrq_el0      // Read the frequency of the system counter
    lsl x0, x0, #1          // Multiply by 2 (shift left by 1 bit)
    msr cntp_tval_el0, x0   // Write the ticks to cntp_tval_el0 that means CNTP_CVAL_EL0 is set to CNTPCT_EL0 + 2*system frequency
    ret

// Enable the core timer interrupt
enable_core_timer_int:
    // Enabled the nCNTPNSIRQ (Physical Timer Non-Secure state IRQ) by setting the bit[1]
    mov x0, 2
    ldr x1, =CORE0_TIMER_IRQ_CTRL
    str w0, [x1]            // 因為 Core 0 Timers interrupt control 這個 register 是 32-bit，因此用 w0 寫入
    ret

disable_core_timer_int:
    // disabled the nCNTPNSIRQ (Physical Timer Non-Secure state IRQ) by setting the bit[1]
    ldr x1, =CORE0_TIMER_IRQ_CTRL
    str wzr, [x1]            // 因為 Core 0 Timers interrupt control 這個 register 是 32-bit，因此用 w0 寫入
    ret
