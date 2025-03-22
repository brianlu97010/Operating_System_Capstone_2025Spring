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