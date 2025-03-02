#ifndef _MAILBOX_H
#define _MAILBOX_H

void mailbox_call(volatile unsigned int* msg);

void get_board_revision();

void get_memory_info();

#endif