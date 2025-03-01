#ifndef _MUART_H
#define _MUART_H

/* Receive the cahr data transmitted from host */ 
char muart_receive();

/* Transmit the char data to host */ 
void muart_send(const char);

/* Initialize the mini UART */
void muart_init();

#endif