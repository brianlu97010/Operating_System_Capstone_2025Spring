SECTIONS
{
    . = 0x80000;
    .text.boot : { KEEP(*(.text.boot)) }
    .text : { *(.text) }

    .rodata : { *(.rodata) }
    .data : { *(.data) }

    . = ALIGN(0x1000);  /* Align 4KB */
    bss_begin = .;
    .bss (NOLOAD) : { *(.bss) }
    bss_end = .;

    /* Initialize the address and the size of heap */
    . = ALIGN(0x1000);  /* Align 4KB */ 
    heap_begin = .;
    . = . + 0x100000;   /* Allocate 1MB to heap (pre-allocated memory pool) */
    heap_end = .;

    /* Initialize the address and size of stack */
    . = ALIGN(0x8);  /* For 64 bits data, aligned the memory address in 8 bytes */
    . = . + 0x400000; /* Allocate 4MB to stack */
    stack_top = .;   /* Stack grows from up to down */

    /* Stack for EL0 user programs */
    . = ALIGN(0x8);
    . = . + 0x100000; /* Allocate 1MB to EL0 stack */
    el0_stack_top = .;

    _end = .;
}