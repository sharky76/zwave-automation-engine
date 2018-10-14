#include "cli_homebridge.h"
#include "homebridge_manager.h"
#include <string.h>
#include "logger.h"

USING_LOGGER(HomebridgeManager);

cli_node_t*     homebridge_node;
extern bool    cmd_exit_node(vty_t* vty, variant_stack_t* params);
extern bool    cmd_enter_node(vty_t* vty, variant_stack_t* params);

bool    cmd_homebridge_control(vty_t* vty, variant_stack_t* params);
bool    cmd_homebridge_show(vty_t* vty, variant_stack_t* params);
bool    cmd_homebridge_enable(vty_t* vty, variant_stack_t* params);
bool    cmd_homebridge_disable(vty_t* vty, variant_stack_t* params);

cli_command_t   homebridge_root_list[] = {
    {"homebridge",                   cmd_homebridge_control,     "Control homebridge module"},
    {"show homebridge", cmd_homebridge_show, "Show Homebridge service"},
    {NULL,                       NULL,  NULL},
};

cli_command_t   homebridge_command_list[] = {
    {"enable",      cmd_homebridge_enable, "Enable homebridge service"},
    {"no enable",   cmd_homebridge_disable, "Disable homebridge service"},
    {NULL, NULL, NULL},
};

void    cli_homebridge_init(cli_node_t* parent_node)
{
    cli_append_to_node(parent_node, homebridge_root_list);
    cli_install_node(&homebridge_node, parent_node, homebridge_command_list, "homebridge", "homebridge");
}

bool    cmd_homebridge_control(vty_t* vty, variant_stack_t* params)
{
    cli_enter_node(vty, "homebridge");
    /*const char* command = variant_get_string(stack_peek_at(params, 2));

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
    }*/

    return true;
}

bool    cmd_homebridge_show(vty_t* vty, variant_stack_t* params)
{
    vty_write(vty, "homebridge%s", VTY_NEWLINE(vty));
    if(homebridge_manager_is_running())
    {
        vty_write(vty, " enable%s", VTY_NEWLINE(vty));
    }
    else 
    {
        vty_write(vty, " no enable%s", VTY_NEWLINE(vty));
    }

    return true;
}

bool    cmd_homebridge_enable(vty_t* vty, variant_stack_t* params)
{
    homebridge_manager_set_start_state(true);

    if(vty->is_interactive)
    {
        homebridge_manager_init();
    }
    return true;
}

bool    cmd_homebridge_disable(vty_t* vty, variant_stack_t* params)
{
    homebridge_manager_set_start_state(false);
    homebridge_manager_stop();
    return true;
}
