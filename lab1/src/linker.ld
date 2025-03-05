SECTIONS
{
    . = 0x80000;
    .text.boot : { KEEP(*(.text.boot)) }
    .text : { *(.text) }

    .rodata : { *(.rodata) }
    .data : { *(.data) }

    . = ALIGN(0x8);  /* For 64 bits data, aligned the memory address in 8 bytes */
    bss_begin = .;
    .bss (NOLOAD) : { *(.bss) }
    bss_end = .;

    /* Initialize the address and size of stack */
    . = ALIGN(0x8);  /* For 64 bits data, aligned the memory address in 8 bytes */
    . = . + 0x400000; /* Allocate 4MB to stack */
    stack_top = .;   /* Stack grows from up to down */

    _end = .;
}