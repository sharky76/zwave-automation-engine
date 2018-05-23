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

variant_stack_t* timer_list_static;
cli_node_t* timer_node;

cli_command_t    timer_command_list[] = {
    {"enable",       cmd_timer_enable,  "Enable timer service"},
    {"no enable",    cmd_timer_disable, "Disable timer service"},
    {"timeout INT scene|command LINE", cmd_timer_set_timeout, "Start timer"},
    {"interval INT scene|command LINE", cmd_timer_set_interval, "Start periodic timer"},
    {"no timeout INT scene|command LINE", cmd_timer_del_timeout, "Stop timer"},
    {"no interval INT scene|command LINE", cmd_timer_del_interval, "Stop periodic timer"},
    {NULL,           NULL,               NULL}
};

void    timer_cli_init(cli_node_t* parent_node)
{
    cli_install_node(&timer_node, parent_node, timer_command_list, "Timer", "service-timer");
    timer_list_static = stack_create();
}

char**  timer_cli_get_config(vty_t* vty)
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

    config_list = calloc(timer_list_static->count + 2, sizeof(char*));

    if(timer_enabled)
    {
        config_list[0] = strdup("enable");
    }
    else
    {
        config_list[0] = strdup("no enable");
    }

    int i = 1;
    stack_for_each(timer_list_static, timer_variant)
    {
        timer_info_t* timer_info = (timer_info_t*)variant_get_ptr(timer_variant);
        char timer_config_buf[512] = {0};
        sprintf(timer_config_buf, "%s %d %s %s", 
                (timer_info->singleshot)? "timeout" : "interval",
                timer_info->timeout,
                (timer_info->event_type == SCENE)? "scene" : "command",
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
    cli_assemble_line(params, 3, scene_name, 512);

    timer_info_t* timer = (timer_info_t*)malloc(sizeof(timer_info_t));
    timer->timeout = variant_get_int(stack_peek_at(params, 1));
    timer->name = strdup(scene_name);

    const char* event_type = variant_get_string(stack_peek_at(params, 2));

    if(strcmp(event_type, "scene") == 0)
    {
        timer->event_type = SCENE;
        timer->event_name = strdup(SCENE_ACTIVATION_EVENT);
    }
    else if(strcmp(event_type, "command") == 0)
    {
        timer->event_type = COMMAND;
        timer->event_name = strdup(COMMAND_ACTIVATION_EVENT);
    }

    timer->singleshot = true;
    stack_push_back(timer_list_static, variant_create_ptr(DT_TIMER, timer, &timer_delete_timer));
    
    variant_t* scene_variant = variant_create_string(scene_name);
    variant_t* ret = NULL;

    switch(timer->event_type)
    {
    case SCENE:
        ret = service_call_method("Timer", "Start", scene_variant, stack_peek_at(params, 1));
        break;
    case COMMAND:
        ret = service_call_method("Timer", "Invoke", scene_variant, stack_peek_at(params, 1));
        break;
    }

    free(scene_variant);
    variant_free(ret);
}

bool    cmd_timer_set_interval(vty_t* vty, variant_stack_t* params)
{
    char scene_name[256] = {0};
    cli_assemble_line(params, 3, scene_name, 256);

    timer_info_t* timer = (timer_info_t*)malloc(sizeof(timer_info_t));
    timer->timeout = variant_get_int(stack_peek_at(params, 1));
    timer->name = strdup(scene_name);

    const char* event_type = variant_get_string(stack_peek_at(params, 2));

    if(strcmp(event_type, "scene") == 0)
    {
        timer->event_type = SCENE;
        timer->event_name = strdup(SCENE_ACTIVATION_EVENT);
    }
    else if(strcmp(event_type, "command") == 0)
    {
        timer->event_type = COMMAND;
        timer->event_name = strdup(COMMAND_ACTIVATION_EVENT);
    }

    timer->singleshot = false;
    stack_push_back(timer_list_static, variant_create_ptr(DT_TIMER, timer, &timer_delete_timer));

    variant_t* scene_variant = variant_create_string(scene_name);
    variant_t* ret = NULL;

    switch(timer->event_type)
    {
    case SCENE:
        ret = service_call_method("Timer", "StartInterval", scene_variant, stack_peek_at(params, 1));
        break;
    case COMMAND:
        ret = service_call_method("Timer", "IntervalInvoke", scene_variant, stack_peek_at(params, 1));
        break;
    }
    free(scene_variant);
    variant_free(ret);
}

bool    cmd_timer_del_timeout(vty_t* vty, variant_stack_t* params)
{
    char scene_name[256] = {0};
    cli_assemble_line(params, 4, scene_name, 256);

    int timeout = variant_get_int(stack_peek_at(params, 2));
    const char* event_type_string = variant_get_string(stack_peek_at(params, 3));
    timer_event_type_t event_type;

    if(strcmp(event_type_string, "scene") == 0)
    {
        event_type = SCENE;
    }
    else if(strcmp(event_type_string, "command") == 0)
    {
        event_type = COMMAND;
    }

    stack_for_each(timer_list_static, timer_variant)
    {
        timer_info_t* timer = (timer_info_t*)variant_get_ptr(timer_variant);
        if(timer->event_type == event_type && 
           strcmp(timer->name, scene_name) == 0 && 
           timer->singleshot == true && 
           timer->timeout == timeout)
        {
            stack_remove(timer_list_static, timer_variant);
            variant_free(timer_variant);
            break;
        }
    }

    variant_t* scene_variant = variant_create_string(scene_name);
    variant_t* ret = service_call_method("Timer", "Stop", scene_variant);
    free(scene_variant);
    variant_free(ret);
}

bool    cmd_timer_del_interval(vty_t* vty, variant_stack_t* params)
{
    char scene_name[256] = {0};
    cli_assemble_line(params, 4, scene_name, 256);
    
    int timeout = variant_get_int(stack_peek_at(params, 2));
    const char* event_type_string = variant_get_string(stack_peek_at(params, 3));
    timer_event_type_t event_type;

    if(strcmp(event_type_string, "scene") == 0)
    {
        event_type = SCENE;
    }
    else if(strcmp(event_type_string, "command") == 0)
    {
        event_type = COMMAND;
    }

    stack_for_each(timer_list_static, timer_variant)
    {
        timer_info_t* timer = (timer_info_t*)variant_get_ptr(timer_variant);
        if(timer->event_type == event_type && 
           strcmp(timer->name, scene_name) == 0 && 
           timer->singleshot == false && 
           timer->timeout == timeout)
        {
            stack_remove(timer_list_static, timer_variant);
            variant_free(timer_variant);
            break;
        }
    }

    variant_t* scene_variant = variant_create_string(scene_name);
    variant_t* ret = service_call_method("Timer", "Stop", scene_variant);
    free(scene_variant);
    variant_free(ret);
}

