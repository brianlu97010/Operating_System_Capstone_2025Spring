#ifndef _SHELL_H
#define _SHELL_H

/* Define the type for command handler functions */
typedef int (*cmdHandler_t)(void);

/* Define the structure for command table */
typedef struct {
    const char* name;
    const char* description;
    cmdHandler_t handler;
} cmd_t;

/* Start the shell */
void shell(void);

/* Execute a specific command */
void exec_cmd(const char* cmd);

#endif 