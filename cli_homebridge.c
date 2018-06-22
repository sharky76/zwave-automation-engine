#include "cli_homebridge.h"
#include "homebridge_manager.h"
#include <string.h>

bool    cmd_homebridge_control(vty_t* vty, variant_stack_t* params);

cli_command_t   homebridge_root_list[] = {
    {"homebridge start|stop|status",                   cmd_homebridge_control,     "Control homebridge module"},
    {NULL,                       NULL,  NULL},
};

void    cli_homebridge_init(cli_node_t* parent_node)
{
    cli_append_to_node(parent_node, homebridge_root_list);
}

bool    cmd_homebridge_control(vty_t* vty, variant_stack_t* params)
{
    const char* command = variant_get_string(stack_peek_at(params, 1));

    if(strcmp(command, "start") == 0)
    {
        homebridge_manager_init();
    }
    else if(strcmp(command, "stop") == 0)
    {
        homebridge_manager_stop();
    }
    else if(strcmp(command, "status") == 0)
    {
        if(homebridge_manager_is_running())
        {
            vty_write(vty, "Homebridge is running.%s", VTY_NEWLINE(vty));
        }
        else
        {
            vty_write(vty, "Homebridge is not running.%s", VTY_NEWLINE(vty));
        }
    }
    else
    {
        return false;
    }

    return true;
}
