#include <stack.h>
#include "cli_commands.h"

variant_stack_t*    rest_cli_commands;
cli_node_t*         rest_root_node;

void    cli_rest_init(cli_node_t* parent_node);
void    cli_rest_handle_connect_event(int socket, void* context);
void    cli_rest_handle_data_event(int socket, void* context);

