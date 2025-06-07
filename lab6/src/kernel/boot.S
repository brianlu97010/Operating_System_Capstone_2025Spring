#include "mmu.h"

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

    // Setup MMU configuration
    // Setup TCR_EL1
    ldr x0, =TCR_CONFIG_DEFAULT
    msr tcr_el1, x0

    // Setup MAIR_EL1
    ldr x0, =MAIR_VALUE
    msr mair_el1, x0
    
    // Setup identity paging
    // Setup page table base addresses
    mov x0, 0          // PGD at physical address 0x0000
    mov x1, 0x1000     // PUD at physical address 0x1000
    
    // Clear PGD, PUD page table memory (init to 0)
    mov x2, #0x2000    // Clear 8KB (2 pages)
    bl clear_page_tables
    
    // Reset base addresses
    // mov x0, 0          // PGD at physical address 0x0000
    // mov x1, 0x1000     // PUD at physical address 0x1000
    
    // Setup page descriptor of PGD
    ldr x2, =BOOT_PGD_ATTR
    orr x2, x1, x2     // Combine PUD physical address with attributes
    str x2, [x0]       // Store to the first entry of PGD
    
    // Setup the first entry of PUD: Map 0x00000000-0x3FFFFFFF 1GB block (RAM and GPU peripherals)
    ldr x2, =BOOT_PUD_ATTR
    mov x3, 0x00000000
    orr x3, x2, x3
    str x3, [x1]       // Store to PUD[0]
    
    // Setup the second entry of PUD: Map 0x40000000-0x7FFFFFFF 1GB block (ARM local peripherals)  
    mov x3, 0x40000000
    orr x3, x2, x3
    str x3, [x1, 8]    // Store to PUD[1] (offset 8 bytes = 64 bites)

    // Load PGD to both TTBR0 and TTBR1
    msr ttbr0_el1, x0  // User space (identity mapping)
    msr ttbr1_el1, x0  // Kernel space
    
    ldr x3, =kernel_space_entry // indirect branch to the virtual address  load the kernel space entry point address (will be virtual address, since the symbol is defined in the linker script, and the linker will place it in the virtual address space)

    // Enable MMU   
    mrs x2, sctlr_el1
    orr x2, x2, 1      // Set M bit to enable MMU
    msr sctlr_el1, x2   // Enable MMU, all address will be view as virtual address by CPU

    br x3
    /*
    // After Enable MMU, all address should in the upper address space 
    // * 下面這些指令仍在物理空間執行
    // Initialize the stack pointer to 4MB, using the sybmol defined in linker script
    ldr x3, =stack_top
    mov sp, x3

    // Procedure call will pass x0 ~ x7 as parameters
    mov x0, x21

    // Jump to main function with parameters x0
    bl main
    b busy_loop
    */


kernel_space_entry:
    // 🎯 現在我們在核心虛擬空間執行！
    
    // Initialize the stack pointer using virtual address
    ldr x3, =stack_top  // 現在這是虛擬地址，可以安全使用
    mov sp, x3

    // Procedure call will pass x0 ~ x7 as parameters
    mov x0, x21         // 恢復 DTB 地址

    // Jump to main function with parameters x0
    bl main             // 現在可以安全調用虛擬地址的 main
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


clear_page_tables:
    mov x3, x0         // Start address of PGD (0x0)
clear_loop:
    str xzr, [x3], #8  // Clear 8 bytes and increment
    subs x2, x2, #8    // Decrement remaining bytes
    bne clear_loop
    ret