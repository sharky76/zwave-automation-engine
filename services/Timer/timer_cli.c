#include "timer_cli.h"
#include "timer_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

bool    cmd_timer_enable(vty_t* vty, variant_stack_t* params);
bool    cmd_timer_disable(vty_t* vty, variant_stack_t* params);

char** config_list = NULL;

cli_node_t* timer_node;

cli_command_t    timer_command_list[] = {
    "enable",       cmd_timer_enable,  "Enable timer service",
    "no enable",    cmd_timer_disable, "Disable timer service",
    //"end",          cli_exit_node,      "End current configuration",
    NULL,           NULL,               NULL
};

void    timer_cli_init(cli_node_t* parent_node)
{
    cli_install_node(&timer_node, parent_node, timer_command_list, "Timer", "service-timer");
}

char**  timer_cli_get_config()
{
    if(NULL != config_list)
    {
        char* cfg;
        int i = 0;
        while(cfg = config_list[i++])
        {
            free(cfg);
        }

        free(config_list);
    }

    config_list = calloc(2, sizeof(char*));

    if(timer_enabled)
    {
        config_list[0] = strdup("enable");
    }
    else
    {
        config_list[0] = strdup("no enable");
    }

    return config_list;
}

bool    cmd_timer_enable(vty_t* vty, variant_stack_t* params)
{
    timer_enabled = true;
}

bool    cmd_timer_disable(vty_t* vty, variant_stack_t* params)
{
    timer_enabled = false;
}

