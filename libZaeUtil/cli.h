#pragma once

#include "stack.h"

typedef struct vty_t vty_t;

typedef struct cli_command_t
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
} cli_node_t;

typedef enum
{
    TYPE_CMD_PART,
    TYPE_WORD,
    TYPE_LINE,
    TYPE_INT,
    TYPE_TERM
} NodeType;

typedef enum
{
    CMD_FULL_MATCH,
    CMD_PARTIAL_MATCH,
    CMD_NO_MATCH
} CmdMatchStatus;

typedef struct value_range_t
{
    variant_t*  start;
    variant_t*  end;
} value_range_t;

typedef struct node_data_t
{
    NodeType    type;
    bool        has_range;
    value_range_t   value_range;
    char*       name;
    cli_command_t*  command;
} node_data_t;

typedef struct cmd_tree_node_t
{
    node_data_t*        data;
    variant_stack_t*    children;
} cmd_tree_node_t;

extern variant_stack_t*    cli_node_list;
extern cli_node_t*         root_node;

void    cli_install_node(cli_node_t** node, cli_node_t* parent, cli_command_t command_list[], char* name, char* prompt);
void    cli_append_to_node(cli_node_t* node, cli_command_t command_list[]);
void    cli_assemble_line(variant_stack_t* params, int start, char* out_line);
variant_stack_t*    create_cmd_vec(const char* cmdline);
bool    cli_exit_node(vty_t* vty, variant_stack_t* params);

