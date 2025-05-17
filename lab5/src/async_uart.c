#include "muart.h"
#include "registers.h"
#include "utils.h"
#include "types.h"
#include "string.h"
#include "exception.h"
#include "timer.h"

/*
 ****************************
 *      Async mini UART     *
 ****************************
 */
#define UART_BUFFER_SIZE 256

// Use queue (circular array) to implement the RX and TX buffer
static char rx_buffer[UART_BUFFER_SIZE];    // Receive Buffer
static volatile int rx_head = 0;            // Point to the index can write
static volatile int rx_tail = 0;            // Point to the index can read

static char tx_buffer[UART_BUFFER_SIZE];    // Transmit Buffer
static volatile int tx_head = 0;            // Point to the index can write
static volatile int tx_tail = 0;            // Point to the index can read

// Check if RX buffer is full : return value 1 means RX buffer is full
static inline int rx_buffer_is_full(){
    return ( (rx_head + 1) % UART_BUFFER_SIZE ) == rx_tail;
}

// Check if TX buffer is full : return value 1 means TX buffer is full
static inline int tx_buffer_is_full(){
    return ( (tx_head + 1) % UART_BUFFER_SIZE ) == tx_tail;
}

/* Initialize asynchronous UART with interrupt support */
void async_uart_init(){
    // Reset buffer indices
    rx_head = rx_tail = 0;
    tx_head = tx_tail = 0;
    
    // Enable UART RX interrupts
    // Bit 0 enables RX interrupts, Bit 1 enables TX interrupts
    // Initially only enable RX interrupts
    regWrite(AUX_MU_IER_REG, 1);
    
    // Enable UART interrupt at the second-level controller (bit 29)
    regWrite(ENABLE_IRQS1, (1 << 29));
}

void disable_uart_int() {
    // Disable both RX and TX interrupts in the UART
    regWrite(AUX_MU_IER_REG, 0);
    
    // Flush the RX and TX FIFO
    regWrite(AUX_MU_IIR_REG, 6);        // Set AUX_MU_IIR_REG to 6, clear the rx and tx FIFO

    // Disable UART interrupt at the second-level controller (bit 29)
    regWrite(DISABLE_IRQS1, (1 << 29));
}

/* The UART-specific handler, it will determine the interrupt type and read the data into RX buffer or transmit the data from TX buffer */
void uart_irq_handler(void){
    // Determine interrupt type : On read this register bits[2:1] shows the interrupt ID bit
    unsigned int interrupt_id = ( regRead(AUX_MU_IIR_REG) >> 1) & 3;
    
    // Receiver holds valid byte (interrupt ID : 0b10) i.e, UART Receive FIFO has some data
    if (interrupt_id == 2){
        // Read the datas until RX FIFO is empty and store them into RX buffer if RX buffer has enough space
        while( regRead(AUX_MU_LSR_REG) & 0x1 ){ // Check if RX FIFO is empty
            // Read the data from RX FIFO
            char data = regRead(AUX_MU_IO_REG) & 0xFF;
            
            // Store the data into RX buffer if RX buffer has enough space
            if( !rx_buffer_is_full() ){
                rx_buffer[rx_head] = data;
                rx_head = (rx_head + 1) % UART_BUFFER_SIZE; // Advance the head in RX buffer
            }
        }
    }
    
    // Transmit holding register empty (interrupt ID : 0b01) i.e, UART Transmit FIFO is empty
    else if( interrupt_id == 1 ){
        // If the TX buffer has data need to be transmit, then write the data from TX buffer into UART TX FIFO  
        if( tx_head != tx_tail ){
            regWrite(AUX_MU_IO_REG, tx_buffer[tx_tail]);    // Write the data from TX buffer into UART TX FIFO 
            tx_tail = (tx_tail + 1) % UART_BUFFER_SIZE;     // Advance the tail in TX buffer
        }
        
        // If TX buffer is empty, i.e, no data need to be transmitted, then disable TX interrupt (Enable TX interrupt when we need to transmit data)
        if( tx_head == tx_tail ){
            unsigned int ier = regRead(AUX_MU_IER_REG);
            regWrite(AUX_MU_IER_REG, ier & ~2);
        }
    }
}

/* Non-blocking read : Read the data from the Receive Buffer */
size_t async_uart_read(char* buffer, size_t size){
    size_t count = 0;
    
    // Receive buffer is not empty, read the data from receive buffer and store into buffer
    while (count < size && rx_head != rx_tail ){
        buffer[count++] = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % UART_BUFFER_SIZE;
    }
    
    return count;   // How many bytes we read
}

/* Non-blocking write : Write the data into the Transmit Buffer */ 
size_t async_uart_write(const char* buffer, size_t size) {
    size_t count = 0;
    int tx_was_empty = (tx_head == tx_tail);

    while(count < size && !tx_buffer_is_full()){
        tx_buffer[tx_head] = buffer[count++];
        tx_head = (tx_head + 1) % UART_BUFFER_SIZE;
    }
    
    // 如果 TX buffer 原本是 empty, 且現在放了 data 進去, 代表有資料要傳了, then enable TX interrupt
    if (tx_was_empty && tx_head != tx_tail) {
        // Enable transmitter interrupt
        unsigned int ier = regRead(AUX_MU_IER_REG);
        regWrite(AUX_MU_IER_REG, ier | 0x2);
    }
    
    return count;   // How many bytes we send
}

/* Non-blocking puts for strings */
size_t async_uart_puts(const char* str) {
    if (str == NULL) {
        return 0;
    }
    
    size_t len = strlen(str);
    return async_uart_write(str, len);
}



/* Example of using async UART for reading/writing data */
void async_uart_example(){
    // Disable the timer interrupt for unexpected msg
    disable_core_timer_int();

    // Initialize async UART
    async_uart_init();
    
    // Enable interrupts in EL1 
    enable_irq_in_el1();
    
    // Buffer for the reading data
    char buffer[64] = {0};
    int i = 0;
    char c = '0';
    
    // Welcome message
    async_uart_puts("Async UART Example - Type something and press Enter, Type 'exit' for exit this example\r\n");
    
    while (1) {
        // Check whether the RX buffer has data, if no data, then continue executing next instruction
        if ( rx_head != rx_tail ){
            char c;
            if( async_uart_read(&c, 1) > 0 ){   // Read the receive buffer and store into a temp char
                // Echo the character
                async_uart_write(&c, 1);

                // Press "Backspace"
                if ((c == '\b' || c == 127)){
                    if(i>0){
                        async_uart_puts("\b \b");
                        i--;               // The index in buffer should be moved forward
                        buffer[i] = 0;
                    }
                }
                // Press "Enter"
                else if (c == '\r'){    
                    // End of line, process the input
                    async_uart_puts("\r\nYou typed: ");
                    buffer[i] = '\0';
                    async_uart_puts(buffer);
                    async_uart_puts("\r\n");
                    
                    // Wait for TX buffer to empty
                    while (tx_head != tx_tail) {
                        waitCycle(10);
                    }

                    // Check for exit command
                    if (i == 4 && 
                        buffer[0] == 'e' && 
                        buffer[1] == 'x' && 
                        buffer[2] == 'i' && 
                        buffer[3] == 't') {
                        break;
                    }
                    
                    // Reset buffer
                    i = 0;
                }
                else buffer[i++] = c;  // Store into buffer
            }
        }
        
        // muart_puts("\r\nNext Instruction\r\n");
        waitCycle(100);
    }
    
    // Disable EL1 interrupts
    disable_irq_in_el1();

    // Disable muart interrupt
    disable_uart_int();
    
    muart_puts("Exiting async UART example\r\n");
}