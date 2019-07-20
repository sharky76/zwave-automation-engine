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

variant_stack_t* timer_config_list;
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
    timer_config_list = stack_create();
}

void timer_delete_timer_config(void* arg)
{
    timer_config_t* timer = (timer_config_t*)arg;

    free(timer->name);
    free(timer);
}

char**  timer_cli_get_config(vty_t* vty)
{
    vty_write(vty, (timer_enabled)? "enable%s" : "no enable%s", VTY_NEWLINE(vty));

    int i = 1;
    stack_for_each(timer_config_list, timer_variant)
    {
        timer_config_t* timer_info = (timer_config_t*)variant_get_ptr(timer_variant);
        vty_write(vty, "%s %d %s %s%s", 
                (timer_info->singleshot)? "timeout" : "interval",
                timer_info->timeout,
                (timer_info->event_id == SceneActivationEvent)? "scene" : "command",
                timer_info->name, VTY_NEWLINE(vty));
    }

    return NULL;
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

    timer_config_t* timer = (timer_config_t*)malloc(sizeof(timer_config_t));
    timer->timeout = variant_get_int(stack_peek_at(params, 1));
    timer->name = strdup(scene_name);

    const char* event_type = variant_get_string(stack_peek_at(params, 2));

    if(strcmp(event_type, "scene") == 0)
    {
        timer->event_id = SceneActivationEvent;
    }
    else if(strcmp(event_type, "command") == 0)
    {
        timer->event_id = CommandActivationEvent;
    }

    timer->singleshot = true;
    stack_push_back(timer_config_list, variant_create_ptr(DT_TIMER, timer, &timer_delete_timer_config));
    
    variant_t* scene_variant = variant_create_string(scene_name);
    variant_t* ret = NULL;

    switch(timer->event_id)
    {
    case SceneActivationEvent:
        ret = service_call_method("Timer", "Start", scene_variant, stack_peek_at(params, 1));
        break;
    case CommandActivationEvent:
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

    timer_config_t* timer = (timer_config_t*)malloc(sizeof(timer_config_t));
    timer->timeout = variant_get_int(stack_peek_at(params, 1));
    timer->name = strdup(scene_name);

    const char* event_type = variant_get_string(stack_peek_at(params, 2));

    if(strcmp(event_type, "scene") == 0)
    {
        timer->event_id = SceneActivationEvent;
    }
    else if(strcmp(event_type, "command") == 0)
    {
        timer->event_id = CommandActivationEvent;
    }

    timer->singleshot = false;
    stack_push_back(timer_config_list, variant_create_ptr(DT_TIMER, timer, &timer_delete_timer));

    variant_t* scene_variant = variant_create_string(scene_name);
    variant_t* ret = NULL;

    switch(timer->event_id)
    {
    case SceneActivationEvent:
        ret = service_call_method("Timer", "StartInterval", scene_variant, stack_peek_at(params, 1));
        break;
    case CommandActivationEvent:
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
    int event_id;

    if(strcmp(event_type_string, "scene") == 0)
    {
        event_id = SceneActivationEvent;
    }
    else if(strcmp(event_type_string, "command") == 0)
    {
        event_id = CommandActivationEvent;
    }

    stack_for_each(timer_config_list, timer_variant)
    {
        timer_config_t* timer = (timer_config_t*)variant_get_ptr(timer_variant);
        if(timer->event_id == event_id && 
           strcmp(timer->name, scene_name) == 0 && 
           timer->singleshot == true && 
           timer->timeout == timeout)
        {
            stack_remove(timer_config_list, timer_variant);
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
    int event_id;

    if(strcmp(event_type_string, "scene") == 0)
    {
        event_id = SceneActivationEvent;
    }
    else if(strcmp(event_type_string, "command") == 0)
    {
        event_id = CommandActivationEvent;
    }

    stack_for_each(timer_config_list, timer_variant)
    {
        timer_config_t* timer = (timer_config_t*)variant_get_ptr(timer_variant);
        if(timer->event_id == event_id && 
           strcmp(timer->name, scene_name) == 0 && 
           timer->singleshot == false && 
           timer->timeout == timeout)
        {
            stack_remove(timer_config_list, timer_variant);
            variant_free(timer_variant);
            break;
        }
    }

    variant_t* scene_variant = variant_create_string(scene_name);
    variant_t* ret = service_call_method("Timer", "Stop", scene_variant);
    free(scene_variant);
    variant_free(ret);
}

