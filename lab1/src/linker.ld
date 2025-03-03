SECTIONS
{
    . = 0x80000;
    .text.boot : { KEEP(*(.text.boot)) }
    .text : { *(.text) }

    .rodata : { *(.rodata) }
    .data : { *(.data) }

    . = ALIGN(0x8);
    bss_begin = .;
    .bss (NOLOAD) : { *(.bss) }
    bss_end = .;
    _end = .;

}