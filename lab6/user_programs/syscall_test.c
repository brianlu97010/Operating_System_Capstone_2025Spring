#include "syscall.h"
#include <stdlib.h>

void main() {
    // 測試 getpid
    int pid = sys_getpid();
    sys_uartwrite("My PID: ", 8);
    char pid_str[10];
    int i = 0;
    while (pid > 0) {
        pid_str[i++] = (pid % 10) + '0';
        pid /= 10;
    }
    while (i > 0) {
        sys_uartwrite(&pid_str[--i], 1);
    }
    sys_uartwrite("\r\n", 2);

    // 測試 uart_write
    sys_uartwrite("Testing uart_write...\r\n", 22);

    // 測試 uart_read
    char buf[100];
    sys_uartwrite("Please input something: ", 23);
    int n = sys_uartread(buf, 100);
    sys_uartwrite("You typed: ", 11);
    sys_uartwrite(buf, n);
    sys_uartwrite("\r\n", 2);

    // 測試 fork
    int child_pid = sys_fork();
    if (child_pid == 0) {
        sys_uartwrite("I am child process\r\n", 20);
        sys_exit(0);
    } else {
        sys_uartwrite("I am parent process\r\n", 21);
    }

    // 測試 mbox_call
    unsigned int mbox[8] __attribute__((aligned(16)));
    mbox[0] = 8 * 4;
    mbox[1] = 0;
    mbox[2] = 0x00010002;  // Get board revision
    mbox[3] = 4;
    mbox[4] = 0;
    mbox[5] = 0;
    mbox[6] = 0;
    mbox[7] = 0;
    sys_mbox_call(8, mbox);
    sys_uartwrite("Board revision: ", 15);
    char rev_str[10];
    i = 0;
    unsigned int rev = mbox[5];
    while (rev > 0) {
        rev_str[i++] = (rev % 16) + (rev % 16 < 10 ? '0' : 'a' - 10);
        rev /= 16;
    }
    while (i > 0) {
        sys_uartwrite(&rev_str[--i], 1);
    }
    sys_uartwrite("\r\n", 2);

    // 測試 exec
    sys_uartwrite("Testing exec...\r\n", 17);
    sys_exec("hello", 0);

    // 如果 exec 失敗，則退出
    sys_exit(0);
} 