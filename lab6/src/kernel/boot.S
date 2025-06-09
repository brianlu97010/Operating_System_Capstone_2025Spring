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
    
    /*
    // Setup identity paging (Two level translation, each entry of PUD points to a 1GB block.)
    // Setup page table base addresses
    mov x0, 0          // PGD at physical address 0x0000
    mov x1, 0x1000     // PUD at physical address 0x1000
    
    // Clear PGD, PUD page table memory (init to 0)
    mov x2, #0x2000    // Clear 8KB (2 pages)
    bl clear_page_tables
    
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
    */
    
    // Three level translation: 
    // 0x0000: PGD (Level 0)
    // 0x1000: PUD (Level 1) 
    // 0x2000: PMD for 0x00000000-0x3FFFFFFF (Level 2)
    // 0x3000: PMD for 0x40000000-0x7FFFFFFF (Level 2)
    mov x0, 0           // PGD at physical address 0x0000
    mov x1, 0x1000      // PUD at physical address 0x1000
    mov x2, 0x2000      // PMD for 0x00000000-0x3FFFFFFF at physical address 0x2000
    mov x3, 0x3000      // PMD for 0x40000000-0x7FFFFFFF at physical address 0x3000

    // Clear PGD, PUD, PMD page table memory (init to 0)
    mov x4, #0x4000     // Clear 16KB (4 pages)
    bl clear_page_tables

    // Setup page descriptor of PGD, point to PUD
    ldr x4, =BOOT_PGD_ATTR
    orr x4, x1, x4      // Combine PUD physical address with attributes
    str x4, [x0]        // Store to the first entry of PGD

    // Setup page descriptor of PUD, cover the physical address: 0x00000000-0x3FFFFFFF
    ldr x4, =BOOT_PUD_ATTR
    orr x4, x2, x4      // Combine PMD physical address with attributes
    str x4, [x1]        // Store to PUD[0]

    // Setup page descriptor of PUD, cover the physical address: 0x40000000-0x7FFFFFFF
    ldr x4, =BOOT_PUD_ATTR
    orr x4, x3, x4      // Combine PMD physical address with attributes
    str x4, [x1, 8]     // Store to PUD[1] (offset 8 bytes = 64 bits)

    // Setup PMD
    bl setup_ram_pmd
    
    // Setup second PMD: Map 0x40000000-0x7FFFFFFF with 2MB blocks  
    bl setup_local_peripherals_pmd
    
    // Load PGD to both TTBR0 and TTBR1
    msr ttbr0_el1, x0  // User space
    msr ttbr1_el1, x0  // Kernel space
    
    ldr x3, =kernel_space_entry // indirect branch to the virtual address  load the kernel space entry point address (will be virtual address, since the symbol is defined in the linker script, and the linker will place it in the virtual address space)

    // Enable MMU   
    mrs x2, sctlr_el1
    orr x2, x2, 1      // Set M bit to enable MMU
    msr sctlr_el1, x2   // Enable MMU, all address will be view as virtual address by CPU

    br x3
    /*
    // After Enable MMU, all address should in the upper address space 
    // * 下面這些指令在 phyical address 執行
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
    // After set M bit in SCTLR_EL1, all address will be view as virtual address by CPU
    
    // Initialize the stack pointer using virtual address
    ldr x3, =stack_top  // Get the virtual address from linker script
    mov sp, x3

    // Procedure call will pass x0 ~ x7 as parameters
    mov x0, x21         // x21 contains the fdt address

    // Jump to main function with parameters x0
    bl main             // branch to main function (this symbol is virtual address)
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


// Setup first GB PMD (0x00000000-0x3FFFFFFF, 2MB blocks)
setup_ram_pmd:
    mov x5, 0x2000     // PMD base address
    mov x6, 0          // Current physical address
    mov x7, 0          // PMD entry index
    ldr x8, =RAM_END   // 0x3C000000, end of RAM
    
ram_pmd_loop:
    cmp x7, #512                // Check PMD entry limit
    b.ge ram_pmd_done
    
    // Determine memory attribute based on address
    cmp x6, x8            // 0x3C000000
    b.ge use_device_attr_ram    // >= 0x3C000000: Device Memory
    
    // Use Normal Memory attribute for RAM (< 0x3C000000)
    ldr x9, =BOOT_PMD_ATTR_RAM
    b set_pmd_entry_ram
    
use_device_attr_ram:
    // Use Device Memory for BCM2837 peripherals (>= 0x3C000000)
    ldr x9, =BOOT_PMD_ATTR_PERIPHERAL
    
set_pmd_entry_ram:
    orr x9, x6, x9              // Combine physical address with attributes
    str x9, [x5, x7, lsl #3]    // Store to PMD[index]
    
    ldr x10, =BLOCK_SIZE_2MB // 2MB block size
    add x6, x6, x10 // Next 2MB block
    add x7, x7, #1              // Next PMD entry
    b ram_pmd_loop              // Continue until 512 entries
    
ram_pmd_done:
    ret



// Setup local peripheral (0x40000000-0x7FFFFFFF, 2MB blocks, Device Memory)
setup_local_peripherals_pmd:
    mov x5, 0x3000              // Second PMD base address
    mov x6, 0x40000000          // Starting physical address (1GB)
    mov x7, 0                   // PMD entry index
    
local_peripherals_pmd_loop:
    cmp x7, #512                // PMD has 512 entries
    b.ge local_peripherals_pmd_done
    
    // Use Device Memory attribute for MMIO
    ldr x8, =BOOT_PMD_ATTR_PERIPHERAL
    orr x8, x6, x8              // Combine physical address with attributes
    str x8, [x5, x7, lsl #3]    // Store to PMD[index]
    
    ldr x10, =BLOCK_SIZE_2MB // 2MB block size
    add x6, x6, x10 // Next 2MB block
    add x7, x7, #1              // Next PMD entry
    b local_peripherals_pmd_loop
    
local_peripherals_pmd_done:
    ret



// Clear page table memory
// x0: start address, x4: size to clear
clear_page_tables:
    mov x5, x0          // Current address
    add x6, x0, x4      // End address
    
clear_loop:
    cmp x5, x6
    b.ge clear_done
    str xzr, [x5]       // Store zero
    add x5, x5, #8      // Next 8 bytes
    b clear_loop
    
clear_done:
    ret