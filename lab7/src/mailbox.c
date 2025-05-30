#include "mailbox.h"
#include "registers.h"
#include "utils.h"
#include "muart.h"

void mailbox_call(unsigned int channel, volatile unsigned int* msg){
    unsigned long msg_addr = (unsigned long)msg;
    
    // Combine the message address (upper 28 bits) with channel number (lower 4 bits)  
    msg_addr = (msg_addr & ~0xF) | (channel & 0xF);
    
    // Check whether the Mailbox 0 status register’s full flag is set.
    while( regRead(MAILBOX_STATUS) & MAILBOX_FULL ){
        // Mailbox is Full: do nothing
    }
    // If not, then you can write the data to Mailbox 1 Read/Write register.
    regWrite(MAILBOX_WRITE, msg_addr);
    
    while(1){
        // Check whether the Mailbox 0 status register’s empty flag is set.
        while( regRead(MAILBOX_STATUS) & MAILBOX_EMPTY ){
            // Mailbox is Empty: do nothing
        }
        // If not, then you can read from Mailbox 0 Read/Write register.
        unsigned int response = regRead(MAILBOX_READ);
    
        // Check if the value is the same as you wrote in step 1.
        if((response & 0xF) == channel){
            return;
        }
    }
}

void get_board_revision(){
    const int SIZE = 7;
    volatile unsigned int __attribute__((aligned(16))) mailbox[SIZE];
    
    mailbox[0] = SIZE * 4;           // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_BOARD_REVISION; // tag identifier
    mailbox[3] = 4;                  // maximum of request and response value buffer's length. (in bytes)
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0;                  // value buffer
    // tags end
    mailbox[6] = END_TAG;
  
    mailbox_call(8, mailbox); // message passing procedure call

    if(mailbox[1] == REQUEST_SUCCEED) {
        muart_puts("Board Revision : ");    // it should be 0xa020d3 for rpi3 b+
        muart_send_hex(mailbox[5]);
        muart_puts("\r\n");
    } else {
        // Request is failed
        muart_puts("Request is failed\r\n");
    }
}

void get_memory_info(){
    const int SIZE = 8;
    volatile unsigned int __attribute__((aligned(16))) mailbox[SIZE];
    
    mailbox[0] = SIZE * 4;           // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_ARM_MEMORY;     // tag identifier
    mailbox[3] = 8;                  // reponse length 
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0;                  // value buffer
    mailbox[6] = 0;
    // tags end
    mailbox[7] = END_TAG;
  
    mailbox_call(8, mailbox); // message passing procedure call

    if(mailbox[1] == REQUEST_SUCCEED) {
        muart_puts("ARM memory base address : ");
        muart_send_hex(mailbox[5]);
        muart_puts("\r\n");
        muart_puts("ARM memory size : ");
        muart_send_hex(mailbox[6]);
        muart_puts("\r\n");
    } else {
        // Request is failed
        muart_puts("Request is failed\r\n");
    }
}