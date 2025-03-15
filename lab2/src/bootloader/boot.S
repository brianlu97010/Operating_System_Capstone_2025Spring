.section ".text.boot"

.global _start
_start:
    // Find the primary core (CPU 0)
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    cbz x0, primary_core
    b busy_loop                     // other cores do busy loop

primary_core:
    ldr x1, =0x80000                // Raspi3 starting address
    ldr x2, =0x60000                // Target address where relocate to

    // Get bootloader size using symbols from the linker script
    ldr x3, =_start                 // start address (0x80000)
    ldr x4, =bootloader_code_end    // end address of bootloader's code and data
    sub x5, x4, x3                  // size of bootloader's code and data (in bytes)

copy_loop:
    // Check whether bootloader's size less than 0
    cmp x5, #0                      // if = 0, Z flag == 1; if < 0, N flag ==1
    ble copy_done                   // Jump to copy_done if (Z==1) ||(N!=V) V:Overvlos

    // Load 8 bytes (64bits) data from start address 
    ldr x6, [x1], #8
    // store the data into reallocate address
    str x6, [x2], #8
    
    // Decrement the bootloader's size by 8 bytes
    sub x5, x5, #8

    // Continue until all datas are copied
    b copy_loop

copy_done:
    // Jump to relocated code
    ldr x7, =relocation         // Load the address of relocation label in x7 register
    sub x7, x7, x1              // Calculate the offset between the relocation label and start address (0x80000)
    add x7, x7, x2              // Adjust the PC address by the offset 
    br x7

relocation:
    // Recalculate BSS and stack addresses based on relocation
    ldr x1, =bss_begin           // Original BSS begin address
    ldr x2, =bss_end             // Original BSS end address
    ldr x3, =stack_top           // Original stack top address

    // Adjust these addresses based on the new location
    ldr x4, =0x80000  
    ldr x5, =0x60000     
    sub x6, x5, x4               // Calculate the offset
    add x1, x1, x6               // Adjusted BSS begin
    add x2, x2, x6               // Adjusted BSS end
    add x3, x3, x6               // Adjusted stack top

    // Initialize BSS segment 
    ldr x1, =bss_begin
    ldr x2, =bss_end
    bl setmemtozero

    // Initialize the stack pointer to 4MB, using the sybmol defined in linker script
    ldr x3, =stack_top
    mov sp, x3

    // Jump to bootloader main function
    bl bootloader_main
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