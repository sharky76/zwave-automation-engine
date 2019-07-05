#pragma once

/*
List of CLI commands used for interaction
*/
#include <stdbool.h>
#include <stack.h>
#include <poll.h>
#include "vty.h"
#include "cli.h"
#include "event_dispatcher.h"

/*typedef struct cli_command_t
{
    char*   name;
    bool    (*func)(vty_t*, variant_stack_t*);
    char*   help;
} cli_command_t;

typedef struct cli_node_t
{
    struct cli_node_t*  parent;
    char*               name;
    char*               prompt;
    char*               context;
    variant_stack_t*    command_tree;
} cli_node_t;*/

void    cli_init();
void    cli_load_config();
void    cli_set_vty(vty_t* vty);
bool    cli_command_exec(vty_t* vty, char* line);
bool    cli_command_exec_custom_node(cli_node_t* node, vty_t* vty, char* line);
void    cli_command_exec_default(char* line);
char**  cli_command_completer(const char* text, int start, int stop);
char**  cli_command_completer_norl(vty_t* vty, const char* text, int size);
int     cli_command_describe();
int     cli_command_quit(int count, int key);
//variant_stack_t*    cli_get_command_completions(vty_t* vty, const char* buffer, size_t size);
void    cmd_enter_root_node(vty_t* vty);

void    cli_commands_handle_connect_event_old(int cli_socket, void* context);
void    cli_commands_handle_data_event_old(int cli_socket, void* context);
void    cli_commands_handle_http_data_event(event_pump_t* pump, int fd, void* context);

void    cli_commands_handle_connect_event(event_pump_t* pump, int fd, void* context);
void    cli_commands_handle_data_write_event(event_pump_t* pump, int fd, void* context);
void    cli_commands_handle_data_read_event(event_pump_t* pump, int fd, void* context);


