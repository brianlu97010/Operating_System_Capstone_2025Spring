#ifndef _MUART_H
#define _MUART_H

/* Receive the cahr data transmitted from host */ 
char muart_receive();

/* Transmit the char data to host */ 
void muart_send(const char);

/* Initialize the mini UART */
void muart_init();

/* Write the C string str to the transmit FIFO by mini UART */ 
void muart_puts(const char*);

/* Transmit the int data to host in hex */
void muart_send_hex(unsigned int);

/* Transmit the int data to host in decimal */
void muart_send_dec(int);

#endif