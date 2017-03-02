#include "cli_rest.h"
#include <string.h>
#include <stdlib.h>

cli_node_t*     rest_node;

bool    cmd_query_resources(vty_t* vty, variant_stack_t* params);

cli_command_t   rest_root_list[] = {
    {"rest v1 resource query",  cmd_query_resources,  "Query rest resources"},
    {NULL,                     NULL,                            NULL}
};

void    cli_rest_init(cli_node_t* parent_node)
{
    rest_cli_commands = stack_create();
    cli_install_custom_node(rest_cli_commands, &rest_root_node, NULL, rest_root_list, "", "config");
}

bool    cmd_query_resources(vty_t* vty, variant_stack_t* params)
{
    vty_write(vty, "{ bababa: \"bobobo\"}\n");
    return true;
}
