#include "syscall.h"
#include "exception.h"

.global exec_user_program
.global set_exception_vector_table
.global get_spsr_el1
.global get_elr_el1
.global get_esr_el1
.global get_vbar_el1
.global get_current_el
.global enable_irq_in_el1
.global disable_irq_in_el1



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


.macro kernel_entry, el
    // if el0 switch to el1, then sp will be set as sp_el1 i.e, kernel stack's stack pointer by hardware  
    sub sp, sp, #TRAP_FRAME_SIZE           // Creates space on the kernel stack to store  31 general-purpose registers (x0-x30) current state and sp_el0, elr_el1, spsr_el1, each register is 64bit-width = 8bytes
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

    // 在不同的 EL 會用不同的 sp_elx 所以當我從 EL0 switch 到 EL1 時需要保存原本 EL0 在用的 sp 等到 exception 結束後將 原本 EL0 在用的 sp restore 
	// if didn't store sp_el0: 
	// thread A 執行 syscall，從 EL0 切換到 EL1
	// 在 kernel 中，發生中斷導致 context switch 到 thread B
	// task B 也執行 syscall
	// syscall 完成後，返回進程 B 的 user mode
	// then，scheduler 切換回 thread A，繼續執行之前被中斷的 syscall
	// 如果沒有正確保存和恢復 SP_EL0，當 task A 返回 EL0 時，thread A 可能會使用到 task B 的 sp_el0
	.if	\el == 0		// if el == 0, then save sp_el0 to x21 register
	    mrs	x21, sp_el0
	.else				// if el == 1, then 計算進入 exception 前的 stack pointer 值（當前 sp + 34*8)
	    add	x21, sp, #TRAP_FRAME_SIZE
	.endif

	// store the system registers
	mrs	x22, elr_el1
	mrs	x23, spsr_el1

	stp	x30, x21, [sp, #16 * 15] 	// store the link register and sp_el0 to stack
	stp	x22, x23, [sp, #16 * 16]	// store the elr_el1 and spsr_el1 to stack
.endm

.macro	kernel_exit, el
	ldp	x22, x23, [sp, #16 * 16]
	ldp	x30, x21, [sp, #16 * 15] 

	.if	\el == 0
        msr	sp_el0, x21
	.endif

	msr	elr_el1, x22			
	msr	spsr_el1, x23

	ldp	x0, x1, [sp, #16 * 0]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x14, x15, [sp, #16 * 7]
	ldp	x16, x17, [sp, #16 * 8]
	ldp	x18, x19, [sp, #16 * 9]
	ldp	x20, x21, [sp, #16 * 10]
	ldp	x22, x23, [sp, #16 * 11]
	ldp	x24, x25, [sp, #16 * 12]
	ldp	x26, x27, [sp, #16 * 13]
	ldp	x28, x29, [sp, #16 * 14]
	add	sp, sp, #TRAP_FRAME_SIZE		
	eret
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

    // Exception from the current EL while using SP_ELx
    b sync_exception_handler        // Synchronous
    .align 7            
    b irq_handler                   // IRQ/vIRQ
    .align 7
    b other_exception_handler       // FIQ/vFIQ
    .align 7
    b other_exception_handler       // SError/VSError
    .align 7

    // Exception from a lower EL and at least one lower EL is AArch64
    b el0_sync                      // Synchronous
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

// Read the ESR register to determine the type of exception, if is svc, then jump to `el0_svc`
// if not, then jump to `handle_invalid_entry`
el0_sync:
	kernel_entry 0					// argument 0 means EL0, store the registers to stack (trap frame)
	mrs	x25, esr_el1				// read the syndrome register
	lsr	x24, x25, 26		        // exception class (EC)
	cmp	x24, 0x15			        // SVC in 64-bit state
	b.eq	el0_svc				
	b       unexpected_irq_handler

el0_svc:
    bl	enable_irq_in_el1			    // enable interrupts

	// handle the syscall, get the address of the corresponding syscall function by using the syscall number as the index of the syscall table
    mov x0, sp                      // 把 sp (trap frame 的 start address) 作為參數傳到 syscall_handler
    bl  syscall_handler
    b	ret_from_syscall				// when the syscall is done, jump to `ret_from_syscall` to return to the user process

// After the syscall is done, cpu will return to this label
// first enable the interrupts
// then restore the user context in the stack
ret_from_syscall:
	bl	enable_irq_in_el1				
	kernel_exit 0					// restore the user context in the stack (because we save the return value of the syscall in the stack (trap frame) in syscall_handler(), now 
									// x0 regiser is the return value of the syscall), finally, called `eret` instruction to return to the user process (switching to EL0)


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

get_current_el:
    mrs x0, CurrentEL
    lsr x0, x0, #2
    ret    

enable_irq_in_el1: 
    msr DAIFClr, 0xF
    ret

disable_irq_in_el1:
    msr DAIFSet, 0xF
    ret