#include "shell.h"
#include "utils.h"
#include "string.h"
#include "muart.h"
#include "mailbox.h"
#include "reboot.h"
#include "cpio.h"
#include "malloc.h"
#include "types.h"
#include "timer.h"

// Declaration of command
static int cmd_help(int argc, char* argv[]);
static int cmd_hello(int argc, char* argv[]);
static int cmd_reboot(int argc, char* argv[]);
static int cmd_mailbox(int argc, char* argv[]);
static int cmd_ls(int argc, char* argv[]);
static int cmd_cat(int argc, char* argv[]);
static int cmd_memAlloc(int argc, char* argv[]);
static int cmd_exec_prog(int argc, char* argv[]);
static int cmd_async_uart(int argc, char* argv[]);
static int cmd_set_timeout(int argc, char* argv[]);

// Define a command table
static const cmd_t cmdTable[] = {
    {"help",    "\t\t: print this help menu\r\n",             cmd_help},
    {"hello",   "\t\t: print Hello World !\r\n",              cmd_hello},
    {"reboot",  "\t\t: reboot the device\r\n",                cmd_reboot},
    {"mailbox", "\t\t: show the mailbox info\r\n",            cmd_mailbox},
    {"ls",      "\t\t: list information about the FILEs\r\n", cmd_ls},
    {"cat",     "\t\t: view the content of the file \r\n"
                "\t\t  Usage: cat <filename> \r\n",           cmd_cat},
    {"memAlloc","\t: a simple allocator, will returns "
                "a pointer points to a continuous "
                "space for requested size\r\n",               cmd_memAlloc},
    {"exec", "\t\t: execute a user program at EL0\r\n\t\t  Usage: exec <filename>\r\n", cmd_exec_prog},
    {"auart", "\t\t: Example of using async UART for reading/writing data\r\n", cmd_async_uart},
    {"setTimeout", "\t: set a timeout to display a message\r\n\t\t  Usage: setTimeout \"MESSAGE\" SECONDS\r\n", cmd_set_timeout},
    {NULL, NULL, NULL}
};

// Definition of command
static int cmd_help(int argc, char* argv[]){
    for(const cmd_t* cmdTable_entry = cmdTable;
        cmdTable_entry->name != NULL; 
        cmdTable_entry++ ){
        muart_puts(cmdTable_entry->name);
        muart_puts(cmdTable_entry->description);
    }
    return 0;
}

static int cmd_hello(int argc, char* argv[]){
    muart_puts("Hello World ! \r\n");
    return 0;
}

static int cmd_reboot(int argc, char* argv[]){
    muart_puts("Reboot the Raspi .... \r\n");
    reset();
    return 0;
}

static int cmd_mailbox(int argc, char* argv[]){
    get_board_revision();
    get_memory_info();
    return 0;
}

static int cmd_ls(int argc, char* argv[]){
    void* initranfs_addr = get_cpio_addr();
    cpio_ls(initranfs_addr);
    return 0;
}

static int cmd_cat(int argc, char* argv[]){
    void* initranfs_addr = get_cpio_addr();
    if(argc < 2){
        muart_puts("Usage: cat <filename>\r\n");
        return -1;
    }
    
    cpio_cat(initranfs_addr, argv[1]);
    return 0;
}

static int cmd_memAlloc(int argc, char* argv[]){
    if(argc != 2){
        muart_puts("Usage: memAlloc <size>\r\n");
        return -1;
    }

    // Convert the size argument from a string to an integer
    int size = atoi(argv[1]);

    // Allocate the memory and store the start address in `mem`
    void* mem = simple_alloc(size);

    // Check the results
    if(mem){
        muart_puts("Allocated memory at: ");
        muart_send_hex((unsigned int)mem);
        muart_puts("\r\n");
    }
    else{
        muart_puts("Failed to allocate memory\r\n");
    }
    return 0;
}

static int cmd_exec_prog(int argc, char* argv[]){
    if(argc < 2){
        muart_puts("Usage: exec <filename>\r\n");
        return -1;
    }
    
    // Set core timer expired time to 2 secs
    // set_core_timer();    

    // Enable the timer interrupt of the first level interrupt controller 
    enable_core_timer_int();
    
    // muart_puts("Timer interrupts enabled. Will print time every 2 seconds when in EL0.\r\n");
    
    muart_puts("Executing user program: ");
    muart_puts(argv[1]);
    muart_puts("\r\n");
    
    // Get the initramfs address
    void* initramfs_addr = get_cpio_addr();
    
    // Execute the user program from the initramfs
    cpio_exec(initramfs_addr, argv[1]);

    // Disable core timer interrupt
    disable_core_timer_int();

    return 0;
}

static int cmd_async_uart(int argc, char* argv[]){
    async_uart_example();
    return 0;
}

int cmd_set_timeout(int argc, char* argv[]){    
    if (argc < 3){
        muart_puts("Usage: setTimeout \"MESSAGE\" SECONDS\r\n");
        return -1;
    }
    
    const char* message = argv[1];
    int seconds = atoi(argv[2]);
    
    if (seconds <= 0){
        muart_puts("Error: Timeout must be a positive integer\r\n");
        return -1;
    }

    setTimeout(message, seconds);
    
    muart_puts("Timeout set: \"");
    muart_puts(message);
    muart_puts("\" will be displayed after ");
    muart_send_dec(seconds);
    muart_puts(" seconds\r\n");
    
    return 0;
}

void parse_args(char* cmd_line, int* argc, char* argv[], int max_args){
    *argc = 0;
    
    // Empty line
    if(*cmd_line == 0){
        return;  
    }
    
    // Parse command and arguments
    argv[*argc] = cmd_line;  // Let the argv[0] point to the start address of cmd_line 
    (*argc)++;

    while(*cmd_line){
        if(*cmd_line == ' '){
            // When encounter a space, it means that the argument ends. 
            // Pointer go forward and add a string terminate (NULL) character
            *cmd_line = 0;
            cmd_line++;
            
            if(*cmd_line == 0){
                break;  // End of line
            }
            
            if(*argc < max_args){
                argv[*argc] = cmd_line;  // Put the next argument into argv
                (*argc)++;
            } 
            else{
                muart_puts("Too many arguments, now only accept 8 arguments \r\n");
                break; 
            }
        }
        else{
            cmd_line++;
        }
    }
}

void exec_cmd(const char* cmd_line){
    char buffer[64];
    char* argv[8];
    int argc = 0;
    
    // Make a copy of cmd_line since parse_args function will replace the space with NULL character
    unsigned int len = 0;
    while(cmd_line[len] && len < 63){
        buffer[len] = cmd_line[len];
        len++;
    }
    buffer[len] = 0;
    
    // Parse the command line
    parse_args(buffer, &argc, argv, 8);
    
    // Empty command
    if(argc == 0){
        return;  
    }
    
    // Check all the command in the command table
    for(const cmd_t* cmdTable_entry = cmdTable;
        cmdTable_entry->name != NULL; 
        cmdTable_entry++ ){
        if(strcmp(argv[0], cmdTable_entry->name) == 0){
            // Execute the specific command handler function
            cmdTable_entry->handler(argc, argv);
            return;
        }
    }
    
    muart_puts("Command not found: ");
    muart_puts(argv[0]);
    muart_puts(" Type help to see all available commands \r\n");
    return;
}


void shell(){
    // Waiting for the user type anything in screen (if using screen monitor)
    // muart_receive();
    
    muart_puts("\r\nWelcome to OSC simple shell !!!\r\n");
    muart_puts("Type help to see all available commands \r\n");
    
    // Start shell
    while(1){
        char buffer[64] = {0};
        int i = 0;
        char c = 0;
        muart_puts("# ");

        // Get the input in a line
        while(1){
            c = muart_receive();   // Get the input and store into a temp char
            muart_send(c);         // Echo the char the host typed
            // Press "Backspace"
            if ((c == '\b' || c == 127)){
                if(i>0){
                    muart_puts("\b \b");
                    i--;               // The index in buffer should be moved forward
                    buffer[i] = 0;
                }
            }
            // Press "Enter"
            else if (c == '\r'){    
                muart_puts("\r\n");
                // Execute command e.g. buffer = "cat file1"
                exec_cmd(buffer);
                break;
            }
            else buffer[i++] = c;  // Store into buffer
        }
    }
}