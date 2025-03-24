.section ".text.boot"

.global _start
_start:
    // Save the fdt address passed in x0 before it gets overwritten
    mov x21, x0

    // Find the primary core (CPU 0)
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    cbz x0, primary_core
    b busy_loop // other cores do busy loop

primary_core:
    // Switch from EL2 to EL1
    bl from_el2_to_el1

    // Initialize BSS segment 
    adr x1, bss_begin
    adr x2, bss_end
    bl setmemtozero

    // Initialize the stack pointer to 4MB, using the sybmol defined in linker script
    ldr x3, =stack_top
    mov sp, x3

    // Procedure call will pass x0 ~ x7 as parameters
    mov x0, x21

    // Jump to main function with parameters x0
    bl main
    b busy_loop

busy_loop:
    wfe
    b busy_loop

// Store 0 to the memory addr of $x1
setmemtozero:
    str xzr, [x1], #8
    cmp x2, x1
    bgt setmemtozero
    ret

// Rpi3’s CPU runs in EL2 after being booted by default, switch to EL1 at the beginning
from_el2_to_el1:
    // When exception level taken to EL1, the execution state is determined by HCR_EL2.RW
    mov x0, #1<<31      // set RW bit to 1 (64 bit Execution state)
    msr hcr_el2, x0     // write the value into hcr_el2

    // Set EL1h with interrupt disabled
    mov x0, #0x3c5      // 0x3c5 = 001111000101, SPsel(Bits[0]) = 1: Use the stack pointer of EL1; 
                        //                       CurrentEL(Bits[3:2]) = 01: Current exception level is EL1; 
                        //                       DAIF(Bits[9:6]) = 1111: Disable all interrupts
    msr spsr_el2, x0    // Store the Process State into spsr registers
    msr elr_el2, lr     // Store the address in link register to elr_el
    eret                // Exception Return using the `ELR` and `SPSR` for the "current" Exception level i.e, EL2
                        // When executed, the PE restores PSTATE from the `SPSR`, and branches to the address held in the `ELR` (primary_core+4)