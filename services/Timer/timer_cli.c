#include "timer_cli.h"
#include "timer_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

bool    cmd_timer_enable(vty_t* vty, variant_stack_t* params);
bool    cmd_timer_disable(vty_t* vty, variant_stack_t* params);
bool    cmd_timer_set_timeout(vty_t* vty, variant_stack_t* params);
bool    cmd_timer_set_interval(vty_t* vty, variant_stack_t* params);
bool    cmd_timer_del_timeout(vty_t* vty, variant_stack_t* params);
bool    cmd_timer_del_interval(vty_t* vty, variant_stack_t* params);

char** config_list = NULL;

cli_node_t* timer_node;

cli_command_t    timer_command_list[] = {
    {"enable",       cmd_timer_enable,  "Enable timer service"},
    {"no enable",    cmd_timer_disable, "Disable timer service"},
    {"timeout INT scene LINE", cmd_timer_set_timeout, "Start timer"},
    {"interval INT scene LINE", cmd_timer_set_interval, "Start periodic timer"},
    {"no timeout scene LINE", cmd_timer_del_timeout, "Stop timer"},
    {"no interval scene LINE", cmd_timer_del_interval, "Stop periodic timer"},
    {NULL,           NULL,               NULL}
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

    config_list = calloc(timer_list->count + 2, sizeof(char*));

    if(timer_enabled)
    {
        config_list[0] = strdup("enable");
    }
    else
    {
        config_list[0] = strdup("no enable");
    }

    int i = 1;
    stack_for_each(timer_list, timer_variant)
    {
        timer_info_t* timer_info = (timer_info_t*)variant_get_ptr(timer_variant);
        char timer_config_buf[512] = {0};
        sprintf(timer_config_buf, "%s %d scene %s\n", 
                (timer_info->singleshot)? "timeout" : "interval",
                timer_info->timeout,
                timer_info->name);
        config_list[i++] = strdup(timer_config_buf);
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

bool    cmd_timer_set_timeout(vty_t* vty, variant_stack_t* params)
{
    char scene_name[256] = {0};
    cli_assemble_line(params, 3, scene_name);
    service_call_method(self, "Start", variant_create_string(scene_name), stack_peek_at(params, 1));
}

bool    cmd_timer_set_interval(vty_t* vty, variant_stack_t* params)
{
    char scene_name[256] = {0};
    cli_assemble_line(params, 3, scene_name);
    service_call_method(self, "StartInterval", variant_create_string(scene_name), stack_peek_at(params, 1));
}

bool    cmd_timer_del_timeout(vty_t* vty, variant_stack_t* params)
{
    char scene_name[256] = {0};
    cli_assemble_line(params, 3, scene_name);
    service_call_method(self, "Stop", variant_create_string(scene_name));
}

bool    cmd_timer_del_interval(vty_t* vty, variant_stack_t* params)
{
    char scene_name[256] = {0};
    cli_assemble_line(params, 3, scene_name);
    service_call_method(self, "Stop", variant_create_string(scene_name));
}

