#ifndef _MAILBOX_H
#define _MAILBOX_H

void mailbox_call(unsigned int channel, volatile unsigned int* msg);

void get_board_revision();

void get_memory_info();

#endif