#pragma once

/*
List of CLI commands used for interaction
*/
#include <stdbool.h>
#include <stack.h>
#include "vty.h"
#include "cli.h"

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
void    cli_command_exec_default(char* line);
char**  cli_command_completer(const char* text, int start, int stop);
char**  cli_command_completer_norl(const char* text, int size);
int     cli_command_describe();
int     cli_command_describe_norl(vty_t* vty);
int     cli_command_quit(int count, int key);
variant_stack_t*    cli_get_command_completions(const char* buffer, size_t size);
void    cmd_enter_root_node(vty_t* vty);

//void    cli_install_node(cli_node_t** node, cli_node_t* parent, cli_command_t command_list[], char* name, char* prompt);
//void    cli_append_to_node(cli_node_t* node, cli_command_t command_list[]);
//void    cli_assemble_line(variant_stack_t* params, int start, char* out_line);

