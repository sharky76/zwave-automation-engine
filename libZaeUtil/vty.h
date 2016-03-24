#pragma once
#include <stdio.h>
#include <stdbool.h>

typedef enum vty_type_e
{
    VTY_FILE,
    VTY_SOCKET,
    VTY_STD,
    VTY_HTTP,
} vty_type;

#define IN  0
#define OUT 1

typedef FILE* iopair[2];

typedef struct vty_data_t
{
    union {
        int     socket;
        FILE*   file;
        iopair  io_pair;
    } desc;
} vty_data_t;

//typedef struct cli_command_t cli_command_t;
//typedef struct cli_node_t    cli_node_t;

typedef struct vty_t
{
    vty_type type;
    void    (*write_cb)(struct vty_t*, const char*, va_list);
    char*   (*read_cb)(struct vty_t*);
    void    (*flush_cb)(struct vty_t*);
    vty_data_t* data;
    char*   prompt;
    bool    echo;
    bool    multi_line;
    char    multiline_stop_char;
    char*   buffer;
    char*   banner;
    int     buf_size;
} vty_t;

void    vty_signal_init();
vty_t*  vty_create(vty_type type, vty_data_t* data);
void    vty_set_echo(vty_t* vty, bool is_echo);
void    vty_set_multiline(vty_t* vty, bool is_multiline, char stop_char);
void    vty_set_banner(vty_t* vty, char* banner);
void    vty_display_banner(vty_t* vty);
void    vty_free(vty_t* vty);
void    vty_set_prompt(vty_t* vty, const char* format, ...);
void    vty_write(vty_t* vty, const char* format, ...);
char*   vty_read(vty_t* vty);
void    vty_error(vty_t* vty, const char* format, ...);
