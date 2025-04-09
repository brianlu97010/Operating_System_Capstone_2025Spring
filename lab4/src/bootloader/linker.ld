SECTIONS
{
    . = 0x80000;
    .text.boot : { KEEP(*(.text.boot)) }
    .text : { *(.text) }

    .rodata : { *(.rodata) }
    .data : { *(.data) }
    
    /* Marks the end of bootloader's code and data */
    bootloader_code_end = .;  

    . = ALIGN(0x8);
    bss_begin = .;
    .bss (NOLOAD) : { *(.bss) }
    bss_end = .;
    
    . = ALIGN(0x8);
    . = . + 0x400000;
    stack_top = .;

    _end = .;
}