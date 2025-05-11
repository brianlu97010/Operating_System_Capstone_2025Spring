#ifndef _MUART_H
#define _MUART_H

#include "types.h"

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
void muart_send_dec(unsigned int);

/*
 ****************************
 *      Async mini UART     *
 ****************************
 */

/* Initialize asynchronous UART with interrupt support */
void async_uart_init(void);

/* Non-blocking read : Read the data from the Receive Buffer */
size_t async_uart_read(char* buffer, size_t size);

/* Non-blocking write : Write the data into the Transmit Buffer */ 
size_t async_uart_write(const char* buffer, size_t size);

/* Non-blocking puts for strings */
size_t async_uart_puts(const char* str);

/* The UART-specific handler, it will determine the interrupt type and read the data into RX buffer or transmit the data from TX buffer */
void uart_irq_handler(void);

/* Example of using async UART for reading/writing data */
void async_uart_example(void);

#endif