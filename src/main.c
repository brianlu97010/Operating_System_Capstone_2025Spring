#define PHYADDR                                                  0x3F000000

#define GPFSEL1                 (volatile unsigned int*)(PHYADDR+0x00200004)
#define GPPUD                   (volatile unsigned int*)(PHYADDR+0x00200094)
#define GPPUDCLK0               (volatile unsigned int*)(PHYADDR+0x00200098)

#define AUXENB                  (volatile unsigned int*)(PHYADDR+0x00215004)
#define AUX_MU_CNTL_REG         (volatile unsigned int*)(PHYADDR+0x00215060)
#define AUX_MU_IER_REG          (volatile unsigned int*)(PHYADDR+0x00215044)
#define AUX_MU_LCR_REG          (volatile unsigned int*)(PHYADDR+0x0021504C)
#define AUX_MU_MCR_REG          (volatile unsigned int*)(PHYADDR+0x00215050)
#define AUX_MU_BAUD             (volatile unsigned int*)(PHYADDR+0x00215068)
#define AUX_MU_IIR_REG          (volatile unsigned int*)(PHYADDR+0x00215048)
#define AUX_MU_LSR_REG          (volatile unsigned int*)(PHYADDR+0x00215054)
#define AUX_MU_IO_REG           (volatile unsigned int*)(PHYADDR+0x00215040)

#define NULL ((void*)0)

void regWrite(volatile unsigned int* addr, int val){
    *addr = val;
    return;
}

int regRead(volatile unsigned int* addr){
    return *addr;
}

void waitcycle(unsigned int cycles) {
    for (volatile unsigned int i = 0; i < cycles; i++) {
        __asm__ volatile ("nop \n");
    }
    return;
}

// Receive the cahr data transmitted from host
char muart_receive(){
    while ( !(regRead(AUX_MU_LSR_REG) & 1) ){
        // do nothing
    }
    // The receive FIFO holds at least 1 symbol -> Read from AUX_MU_IO_REG
    return regRead(AUX_MU_IO_REG)&0xFF;
}

// Transmit the char data to host
void muart_send(const char c){
    while ( !(regRead(AUX_MU_LSR_REG) & 32) ){
        // do nothing
    }
    // The transmit FIFO can accept at least one byte -> Write to AUX_MU_IO_REG
    regWrite(AUX_MU_IO_REG, c);
    return;
}

// Write the C string str to the transmit FIFO by mini UART
void puts(const char* str) {
    while(*str){
        muart_send(*str);
        str++;
    }
    return;
}


/**
 * Compares two C strings.
 * 
 * @return <0 if str1 < str2; 0 if equal; >0 if str1 > str2
 * @ref Based on GNU C library implementation (https://github.com/lattera/glibc/blob/master/string/strcmp.c)
*/
int strcmp(const char* str1 , const char* str2){
    const unsigned char* u_str1 = (const unsigned char*)str1;
    const unsigned char* u_str2 = (const unsigned char*)str2;
    unsigned char c1, c2;

    do{
        c1 = *u_str1++;
        c2 = *u_str2++;
        if(!c1){
            return c1 - c2;  
        }
    }while( c1 == c2 );
    
    return c1 - c2;
}


// Define the type for command handler functions
typedef int (*cmdHandler_t)(void);

// Define the structure for command table 
typedef struct{
    const char* name;
    const char* description;
    cmdHandler_t handler;
}cmd_t;

// Declaration
int cmd_help(void);

int cmd_hello(void);

int cmd_reboot(void);

// Define a command table which is a static and const array 
static const cmd_t cmdTable[] = {
    {"help", "\t: print this help menu\n", cmd_help},
    {"hello", "\t: print Hello World !\n", cmd_hello},
    {"reboot", "\t: reboot the device\n", cmd_reboot},
    {NULL, NULL, NULL}
};

int cmd_help(void){
    for(const cmd_t* cmdTable_entry = cmdTable;
        cmdTable_entry->name != NULL; 
        cmdTable_entry++ ){
        puts(cmdTable_entry->name);
        puts(cmdTable_entry->description);
    }
    return 0;
}

int cmd_hello(void){
    puts("Hello World ! \n");
    return 0;
}

int cmd_reboot(void){
    puts("Reboot the Raspi .... \n");
    return 0;
}

void exec_cmd(const char* cmd){
    // Check all the command in the command table
    for(const cmd_t* cmdTable_entry = cmdTable;
        cmdTable_entry->name != NULL; 
        cmdTable_entry++ ){
        if(strcmp(cmd, cmdTable_entry->name) == 0){
            // Execute the specific command handler function
            cmdTable_entry->handler();
            return;
        }
    }
    puts("Command not found: ");
    puts(cmd);
    puts(" Type help to see all available commands \n");
    return;
}


void shell(){
    puts("Welcome to OSC simple shell !!!\n");
    puts("Type help to see all available commands \n");
    
    // Start shell
    while(1){
        char buffer[64] = {0};
        int i = 0;
        char c = ' ';
        puts("# ");

        // Get the input in a line
        while(1){
            c = muart_receive();   // Get the input and store into a temp char
            muart_send(c);         // Echo the char the host typed
            // Press "Backspace"
            if ((c == '\b' || c == 127)){
                if(i>0){
                    puts("\b \b");
                    i--;               // The index in buffer should be moved forward 
                }
            }
            // Press "Enter"
            else if (c == '\r'){    
                puts("\n");
                // Execute command
                exec_cmd(buffer);
                break;
            }
            else buffer[i++] = c;  // Store into buffer
        }
    }
}



void muart_init(){
    // Set GPIO pin 14 15 to ALT5
    int reg;
    reg = regRead(GPFSEL1);
    reg &= ~(7<<12);        // GPIO 14
    reg |= 5<<12;
    reg &= ~(7<<15);        // GPIO 15
    reg |= 5<<15;
    regWrite(GPFSEL1, reg);

    // Disable GPIO pull up/down
    regWrite(GPPUD, 0);
    waitcycle(150);
    reg = regRead(GPPUDCLK0);
    reg |= ( (1<<14)|(1<<15) );
    regWrite(GPPUDCLK0, reg);
    waitcycle(150);
    regWrite(GPPUDCLK0, 0);

    // Initialize mini UART registers
    regWrite(AUXENB, 1);                // Set AUXENB register to enable mini UART. Then mini UART register can be accessed.
    regWrite(AUX_MU_CNTL_REG, 0);       // Set AUX_MU_CNTL_REG to 0. Disable transmitter and receiver during configuration.
    regWrite(AUX_MU_IER_REG, 0);        // Set AUX_MU_IER_REG to 0. Disable interrupt because currently you don’t need interrupt.
    regWrite(AUX_MU_LCR_REG, 3);        // Set AUX_MU_LCR_REG to 3. Set the data size to 8 bit.
    regWrite(AUX_MU_MCR_REG, 0);        // Set AUX_MU_MCR_REG to 0. Don’t need auto flow control.
    regWrite(AUX_MU_BAUD, 270);         // Set AUX_MU_BAUD to 270. Set baud rate to 115200
    regWrite(AUX_MU_IIR_REG, 6);        // Set AUX_MU_IIR_REG to 6.
    regWrite(AUX_MU_CNTL_REG, 3);       // Set AUX_MU_CNTL_REG to 3. Enable the transmitter and receiver.
}

void main(){
    // Todo : Implement mini UART
    muart_init();

    // Todo : Simple Shell
    shell();
    
    // Todo : Mailbox
}