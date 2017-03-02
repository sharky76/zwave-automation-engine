#include <stack.h>
#include "cli_commands.h"

variant_stack_t*    rest_cli_commands;
cli_node_t*         rest_root_node;

void    cli_rest_init(cli_node_t* parent_node);
