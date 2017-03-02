#pragma once
#include <stdio.h>
#include <stdbool.h>
#include "stack.h"

typedef enum vty_type_e
{
    VTY_FILE,
    VTY_SOCKET,
    VTY_STD,
    VTY_HTTP,
} vty_type;

#define IN  0
#define OUT 1
#define BUFSIZE 64000

typedef FILE* iopair[2];

typedef struct vty_data_t
{
    union {
        int     socket;
        FILE*   file;
        iopair  io_pair;
    } desc;
} vty_data_t;

typedef struct cli_node_t cli_node_t;

//typedef struct cli_command_t cli_command_t;
//typedef struct cli_node_t    cli_node_t;

typedef struct vty_t
{
    vty_type type;
    void    (*write_cb)(struct vty_t*, const char*, size_t size);
    char*   (*read_cb)(struct vty_t*);
    void    (*flush_cb)(struct vty_t*);
    void    (*erase_char_cb)(struct vty_t*); // erase last char
    void    (*erase_line_cb)(struct vty_t*);
    void    (*show_history_cb)(struct vty_t*);
    void    (*cursor_left_cb)(struct vty_t*);
    void    (*cursor_right_cb)(struct vty_t*);
    vty_data_t* data;
    char*   prompt;
    bool    echo;
    bool    multi_line;
    char    multiline_stop_char;
    char*   buffer;
    int     cursor_pos;
    char*   banner;
    int     buf_size;
    bool    error;
    variant_stack_t*    history;
    int     history_size;
    int     history_index;  // 0 - most recent entry
    cli_node_t* current_node;
} vty_t;

void    vty_signal_init();
vty_t*  vty_create(vty_type type, vty_data_t* data);
void    vty_set_echo(vty_t* vty, bool is_echo);
void    vty_set_multiline(vty_t* vty, bool is_multiline, char stop_char);
void    vty_set_banner(vty_t* vty, char* banner);
void    vty_display_banner(vty_t* vty);
void    vty_free(vty_t* vty);
void    vty_set_prompt(vty_t* vty, const char* format, ...);
void    vty_display_prompt(vty_t* vty);
void    vty_write(vty_t* vty, const char* format, ...);
char*   vty_read(vty_t* vty);
void    vty_error(vty_t* vty, const char* format, ...);
void    vty_flush(vty_t* vty);
void    vty_set_error(vty_t* vty, bool is_error);
bool    vty_is_error(vty_t* vty);
void    vty_add_history(vty_t* vty);
const char*   vty_get_history(vty_t* vty, bool direction); // true - to newest, false - to oldest
void    vty_set_history_size(vty_t* vty, int size);
void    vty_insert_char(vty_t* vty, char ch);
void    vty_append_char(vty_t* vty, char ch);
void    vty_append_string(vty_t* vty, const char* str);
void    vty_erase_char(vty_t* vty);
void    vty_redisplay(vty_t* vty, const char* new_buffer);
void    vty_clear_buffer(vty_t* vty);
void    vty_show_history(vty_t* vty);
void    vty_cursor_left(vty_t* vty);
void    vty_cursor_right(vty_t* vty);

