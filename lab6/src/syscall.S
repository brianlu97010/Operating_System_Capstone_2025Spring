#include "syscall.h"

// syscall wrapper function (usually not included in the kernel itself — you are more likely to find them in the different language's standard libraries, such as glibc.)
// By convention: 
// registers x0 — x7 are used for syscall arguments
// register x8 is used to store syscall number,
// this allows a syscall to have up to 8 arguments.
.global call_sys_getpid
.global call_sys_uartread
.global call_sys_uartwrite
.global call_sys_exec
.global call_sys_fork
.global call_sys_exit
.global call_sys_mbox_call
.global call_sys_kill

call_sys_getpid:
    mov x8, #SYS_GETPID
    svc #0
    ret

call_sys_uartread:
    mov x8, #SYS_UARTREAD
    svc #0
    ret

call_sys_uartwrite:
    mov x8, #SYS_UARTWRITE
    svc #0
    ret              

call_sys_exec:
    mov x8, #SYS_EXEC
    svc #0
    ret

call_sys_fork:
// Todo
    mov x8, #SYS_FORK
    svc #0
    ret

call_sys_exit: 
    mov x8, #SYS_EXIT
    svc #0
    ret

call_sys_mbox_call:
    mov x8, #SYS_MBOX_CALL
    svc #0
    ret

call_sys_kill:
    mov x8, #SYS_KILL
    svc #0
    ret