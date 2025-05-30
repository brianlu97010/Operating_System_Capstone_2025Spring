.global sys_getpid
.global sys_uartread
.global sys_uartwrite
.global sys_exec
.global sys_fork
.global sys_exit
.global sys_mbox_call
.global sys_kill

// System call wrapper macros
.macro syscall_wrapper name, number
    \name:
        mov x8, #\number
        svc #0
        ret
.endm

// Define system call wrappers
syscall_wrapper sys_getpid, 0
syscall_wrapper sys_uartread, 1
syscall_wrapper sys_uartwrite, 2
syscall_wrapper sys_exec, 3
syscall_wrapper sys_fork, 4
syscall_wrapper sys_exit, 5
syscall_wrapper sys_mbox_call, 6
syscall_wrapper sys_kill, 7 