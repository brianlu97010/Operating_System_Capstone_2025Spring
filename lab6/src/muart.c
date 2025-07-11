#include "muart.h"
#include "registers.h"
#include "utils.h"

/* Receive the cahr data transmitted from host */ 
char muart_receive(){
    while ( !(regRead(AUX_MU_LSR_REG) & 1) ){
        // do nothing
    }
    // The receive FIFO holds at least 1 symbol -> Read from AUX_MU_IO_REG
    return regRead(AUX_MU_IO_REG)&0xFF;
}

/* Transmit the char data to host */ 
void muart_send(const char c){
    while ( !(regRead(AUX_MU_LSR_REG) & 32) ){
        // do nothing
    }
    // The transmit FIFO can accept at least one byte -> Write to AUX_MU_IO_REG
    regWrite(AUX_MU_IO_REG, c);
    return;
}

/* Write the C string str to the transmit FIFO by mini UART */ 
void muart_puts(const char* str) {
    while(*str){
        muart_send(*str);
        str++;
    }
    return;
}

/* Transmit the int data to host in hex */
void muart_send_hex(unsigned int value) {
    // Display prefix of hex
    muart_send('0');
    muart_send('x');
    
    char hex_digits[8];
    
    for (int i = 7; i >= 0; i--) {
        int hex_digit = value & 0xF; // Get the lower 4 bits
        if (hex_digit < 10){
            // Convert to char by ASCII code  
            hex_digits[i] = '0' + hex_digit;
        }
        else
            hex_digits[i] = 'a' + (hex_digit - 10);
        
        value = value >> 4; // right shift 4 bits
    }
    
    // Display the value using muart_send
    for (int i = 0; i < 8; i++) {
        muart_send(hex_digits[i]);
    }
}

/* Transmit the int data to host in decimal */
void muart_send_dec(unsigned int num){
    // Handle the case where num is 0
    if (num == 0){
        muart_send('0');
        return;
    }
    
    // Calculate how many digits in the number
    unsigned int temp = num;
    unsigned int digit_count = 0;
    
    while (temp > 0){
        digit_count++;
        temp /= 10;
    }
    
    // Create a buffer to store the digits in reverse order
    char digits[10]; // Maximum 10 digits for 32-bit unsigned int
    
    // Extract digits in reverse order
    for (int i = digit_count - 1; i >= 0; i--){
        digits[i] = '0' + (num % 10);
        num /= 10;
    }
    
    // Print the digits
    for (unsigned int i = 0; i < digit_count; i++){
        muart_send(digits[i]);
    }
}

/* Initialize the mini UART */
void muart_init(){
    // Set GPIO pin 14 15 to ALT5
    unsigned int reg;
    reg = regRead(GPFSEL1);
    reg &= ~(7<<12);        // GPIO 14
    reg |= 2<<12;
    reg &= ~(7<<15);        // GPIO 15
    reg |= 2<<15;
    regWrite(GPFSEL1, reg);

    // Disable GPIO pull up/down
    regWrite(GPPUD, 0);
    waitCycle(150);
    reg = regRead(GPPUDCLK0);
    reg |= ( (1<<14)|(1<<15) );
    regWrite(GPPUDCLK0, reg);
    waitCycle(150);
    regWrite(GPPUDCLK0, 0);

    // Initialize mini UART registers
    regWrite(AUXENB, 1);                // Set AUXENB register to enable mini UART. Then mini UART register can be accessed.
    regWrite(AUX_MU_CNTL_REG, 0);       // Set AUX_MU_CNTL_REG to 0. Disable transmitter and receiver during configuration.
    regWrite(AUX_MU_IER_REG, 0);        // Set AUX_MU_IER_REG to 0. Disable interrupt because currently you don’t need interrupt.
    regWrite(AUX_MU_LCR_REG, 3);        // Set AUX_MU_LCR_REG to 3. Set the data size to 8 bit.
    regWrite(AUX_MU_MCR_REG, 0);        // Set AUX_MU_MCR_REG to 0. Don’t need auto flow control.
    regWrite(AUX_MU_BAUD, 270);         // Set AUX_MU_BAUD to 270. Set baud rate to 115200
    regWrite(AUX_MU_IIR_REG, 6);        // Set AUX_MU_IIR_REG to 6, clear the rx and tx FIFO
    regWrite(AUX_MU_CNTL_REG, 3);       // Set AUX_MU_CNTL_REG to 3. Enable the transmitter and receiver.
}
