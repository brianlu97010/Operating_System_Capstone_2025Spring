#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "types.h"
#include "exception.h"

#define NR_SYS_CALLS 8

/* System call numbers */
#define SYS_GETPID      0
#define SYS_UARTREAD    1
#define SYS_UARTWRITE   2
#define SYS_EXEC        3
#define SYS_FORK        4
#define SYS_EXIT        5
#define SYS_MBOX_CALL   6
#define SYS_KILL        7

#ifndef __ASSEMBLER__
/* System call function declarations */
int sys_getpid(void);
size_t sys_uartread(char buf[], size_t size);
size_t sys_uartwrite(const char buf[], size_t size);
int sys_exec(const char* name, char *const argv[]);
int sys_fork(void);
void sys_exit(void);
int sys_mbox_call(unsigned char ch, unsigned int *mbox);
void sys_kill(int pid);

/* System call handler */
// void syscall_handler(void);
void syscall_handler(struct trap_frame*);


int call_sys_getpid(void);
size_t call_sys_uartread(char buf[], size_t size);
size_t call_sys_uartwrite(const char buf[], size_t size);
int call_sys_exec(const char* name, char *const argv[]);
int call_sys_fork(void);
void call_sys_exit(void);
int call_sys_mbox_call(unsigned char ch, unsigned int *mbox);
void call_sys_kill(int pid);
#endif

#endif