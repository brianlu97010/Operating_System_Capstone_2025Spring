.global exec_user_program
.global set_exception_vector_table
.global get_spsr_el1
.global get_elr_el1
.global get_esr_el1
.global get_vbar_el1
.global get_cntfrq_el0
.global get_cntpct_el0
.global get_current_el



// save general registers to stack
.macro save_all_reg 
    sub sp, sp, 32 * 8           // Creates space on the stack to store all 31 general-purpose registers (x0-x30), each register is 64bit-width = 8bytes
    stp x0, x1, [sp ,16 * 0]     // Use "Store Pairs" instruction to store two 64bit registers in one instruction, note that [<SP>, #imm] means storing in <SP> + #imm (先加再存)
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]
.endm

// load general registers from stack
.macro load_all_reg
    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    add sp, sp, 32 * 8
.endm


// Exception vector table, branch to the high-level exception handler
.align 11                           // Exception Vector table must be aligned to 0x800 i.e, 2^11 bytes
exception_vector_table:
    // Exception from the current EL while using SP_EL0
    b sync_exception_handler        // Synchronous
    .align 7                        // Each entry in Vector Table has size 0x80 (128) bytes, so we need to align to 2^7 bytes in memory (byte-addressing)
    b irq_handler                   // IRQ/vIRQ
    .align 7
    b other_exception_handler       // FIQ/vFIQ
    .align 7
    b other_exception_handler       // SError/VSError
    .align 7

    // Exception from the current EL while using SP_ELx <- lab3
    b sync_exception_handler        // Synchronous
    .align 7            
    b irq_handler                   // IRQ/vIRQ
    .align 7
    b other_exception_handler       // FIQ/vFIQ
    .align 7
    b other_exception_handler       // SError/VSError
    .align 7

    // Exception from a lower EL and at least one lower EL is AArch64 <- lab3 (ex1, ex2)
    b sync_exception_handler        // Synchronous
    .align 7            
    b irq_handler                   // IRQ/vIRQ
    .align 7
    b other_exception_handler       // FIQ/vFIQ
    .align 7
    b other_exception_handler       // SError/VSError
    .align 7

    // Exception from a lower EL and all lower ELs are AArch32
    b sync_exception_handler        // Synchronous
    .align 7            
    b irq_handler                   // IRQ/vIRQ
    .align 7
    b other_exception_handler       // FIQ/vFIQ
    .align 7
    b other_exception_handler       // SError/VSError
    .align 7

// Set exception vector table, store the starting address in VBAR register
set_exception_vector_table:
    adr x0, exception_vector_table
    msr vbar_el1, x0
    ret

// high-level handler of Synchronous Exception 
// store the registers before handling exception and check the exception type then branch to specific exception handler
sync_exception_handler:
    save_all_reg                // Save the registers before handling exception
    bl svc_handler
    load_all_reg                // Restore the registers after handling exception
    eret

// high-level handler of IRQ
// store the registers before handling interrupt and check the IRQ type then branch to specific IRQ handler
irq_handler:
    save_all_reg                // Save the registers before handling exception
    bl irq_entry                // branch to irq_entry in exception.c, determine the IRQ type
    load_all_reg                // Restore the registers after handling exception
    eret

other_exception_handler:
    save_all_reg
    bl unexpected_irq_handler
    load_all_reg                // Restore the registers after handling exception
    eret


// System register access functions
get_spsr_el1:
    mrs x0, spsr_el1
    ret

get_elr_el1:
    mrs x0, elr_el1
    ret

get_esr_el1:
    mrs x0, esr_el1
    ret

get_vbar_el1:
    mrs x0, vbar_el1
    ret

get_cntfrq_el0:
    mrs x0, cntfrq_el0
    ret

get_cntpct_el0:
    mrs x0, cntpct_el0
    ret

get_current_el:
    mrs x0, CurrentEL
    lsr x0, x0, #2
    ret

/*
// Switch to EL0 and execute the user program at EL0
// x0 = program address
exec_user_program:
    // Save program address in the callee saved registers
    mov x19, x0
    
    // Set up stack for user program
    ldr x1, =el0_stack_top // user_stack_top declared in linker script
    msr sp_el0, x1
    
    // Set EL0t (using SP_EL0) with interrupt disabled
    mov x0, #0x3c0      // 0x3c0 = 001111000000, SPsel(Bits[0]) = 0: Use the SP_EL0 
                        //                       CurrentEL(Bits[3:2]) = 00: Current exception level is EL0; 
                        //                       DAIF(Bits[9:6]) = 1111: Disable all interrupts
    msr spsr_el1, x0    // Store the Process State into spsr registers
    msr elr_el1, x19    // Store the start address of user program to elr_el
    eret                // Exception Return using the `ELR` and `SPSR` for the "current" Exception level i.e, EL1
                        // When executed, the PE restores PSTATE from the `SPSR`, and branches to the address held in the `ELR` (start address of user program)
*/


// Switch to EL0 and execute the user program at EL0
// x0 = program address
exec_user_program:
    // Save program address in the callee saved registers
    mov x19, x0
    
    // Set up stack for user program
    ldr x1, =el0_stack_top // user_stack_top declared in linker script
    msr sp_el0, x1
    
    // Set EL0t (using SP_EL0) with interrupt enabled
    mov x0, 0           // SPsel(Bits[0]) = 0: Use the SP_EL0 
                        // CurrentEL(Bits[3:2]) = 00: Current exception level is EL0; 
                        // DAIF(Bits[9:6]) = 0000: Enables all interrupts
    msr spsr_el1, x0    // Store the Process State into spsr registers
    msr elr_el1, x19    // Store the start address of user program to elr_el
    eret                // Exception Return using the `ELR` and `SPSR` for the "current" Exception level i.e, EL1
                        // When executed, the PE restores PSTATE from the `SPSR`, and branches to the address held in the `ELR` (start address of user program)
