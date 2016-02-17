#pragma once 

/*
 
Define virtual TTY handle for CLI command module 
 
*/
#include <stdio.h>

typedef enum vty_type_e
{
    VTY_FILE,
    VTY_SOCKET,
    VTY_STD,
} vty_type;

typedef FILE* iopair[2];

typedef struct vty_data_t
{
    union {
        int     socket;
        FILE*   file;
        iopair  io_pair;
    } desc;
} vty_data_t;

typedef struct cli_command_t cli_command_t;
typedef struct cli_node_t    cli_node_t;

typedef struct vty_t
{
    vty_type type;
    void    (*write_cb)(struct vty_t*, const char*, va_list);
    char*   (*read_cb)(struct vty_t*);
    vty_data_t* data;
    char*   prompt;
    cli_command_t*      command;
    cli_node_t*         node;
} vty_t;

void    vty_signal_init();
vty_t*  vty_create(vty_type type, vty_data_t* data);
void    vty_free(vty_t* vty);
void    vty_set_prompt(vty_t* vty, const char* format, ...);
void    vty_write(vty_t* vty, const char* format, ...);
char*   vty_read(vty_t* vty);
void    vty_error(vty_t* vty, const char* format, ...);


