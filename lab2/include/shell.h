#ifndef _SHELL_H
#define _SHELL_H

/* Define the type for command handler functions */
typedef int (*cmdHandler_t)(int argc, char* argv[]);

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

/* Parse command arguments */
void parse_args(char* cmd_line, int* argc, char* argv[], int max_args);

#endif 