.section ".text.boot"

.global _start
_start:
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

    // Initialize the stack pointer to 4MB
    adr x3, _start
    add x3, x3, #0x400000
    mov sp, x3

    // Jump to main function
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