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

/* --- Async mini UART --- */
// Initialize asynchronous UART with interrupt support
void async_uart_init(void);

// Non-blocking UART read function - returns number of bytes read
size_t async_uart_read(char* buffer, size_t size);

// Non-blocking UART write function - returns number of bytes queued
size_t async_uart_write(const char* buffer, size_t size);

// Non-blocking UART write for strings (null-terminated)
size_t async_uart_puts(const char* str);

// UART interrupt handler - called from IRQ handler
void uart_irq_handler(void);

void async_uart_example(void);

#endif