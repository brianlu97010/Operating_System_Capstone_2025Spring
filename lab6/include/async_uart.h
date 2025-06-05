#ifndef _ASYNC_UART_H
#define _ASYNC_UART_H

#include "types.h"

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