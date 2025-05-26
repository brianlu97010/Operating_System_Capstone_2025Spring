.global cpu_switch_to
cpu_switch_to:
    // Save the current thread's context
    stp x19, x20, [x0, 16 * 0]
    stp x21, x22, [x0, 16 * 1]
    stp x23, x24, [x0, 16 * 2]
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]
    stp fp, lr, [x0, 16 * 5]
    mov x9, sp
    str x9, [x0, 16 * 6]

    // Load the next thread's context
    ldp x19, x20, [x1, 16 * 0]
    ldp x21, x22, [x1, 16 * 1]
    ldp x23, x24, [x1, 16 * 2]
    ldp x25, x26, [x1, 16 * 3]
    ldp x27, x28, [x1, 16 * 4]
    ldp fp, lr, [x1, 16 * 5]
    ldr x9, [x1, 16 * 6]
    mov sp, x9
    msr tpidr_el1, x1
    ret

.global get_current_thread
get_current_thread:
    mrs x0, tpidr_el1
    ret

// Entry point for kernel threads
.global ret_from_kernel_thread
ret_from_kernel_thread:
    // x19 contains the function pointer
    // x20 contains the argument
    bl enable_irq_in_el1
    cbz x19, ret_to_user
    mov x0, x20
    blr x19
    bl thread_exit