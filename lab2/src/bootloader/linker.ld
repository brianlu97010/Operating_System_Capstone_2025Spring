SECTIONS
{
    . = 0x60000;
    .text.boot : { KEEP(*(.text.boot)) }
    .text : { *(.text) }

    .rodata : { *(.rodata) }
    .data : { *(.data) }
    
    . = ALIGN(0x8);
    bss_begin = .;
    .bss (NOLOAD) : { *(.bss) }
    bss_end = .;
    
    . = ALIGN(0x8);
    . = . + 0x400000;
    stack_top = .;
}