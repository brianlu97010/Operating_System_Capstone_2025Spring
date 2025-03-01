#include "shell.h"
#include "utils.h"
#include "string.h"
#include "muart.h"

// Declaration
static int cmd_help(void);
static int cmd_hello(void);
static int cmd_reboot(void);

// Define a command table
static const cmd_t cmdTable[] = {
    {"help", "\t: print this help menu\n", cmd_help},
    {"hello", "\t: print Hello World !\n", cmd_hello},
    {"reboot", "\t: reboot the device\n", cmd_reboot},
    {NULL, NULL, NULL}
};

static int cmd_help(void){
    for(const cmd_t* cmdTable_entry = cmdTable;
        cmdTable_entry->name != NULL; 
        cmdTable_entry++ ){
        puts(cmdTable_entry->name);
        puts(cmdTable_entry->description);
    }
    return 0;
}

static int cmd_hello(void){
    puts("Hello World ! \n");
    return 0;
}

static int cmd_reboot(void){
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