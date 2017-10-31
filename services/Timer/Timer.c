#include <service.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <logger.h>
#include "timer_cli.h"
#include "timer_config.h"
#include "event.h"

// Config variables
bool timer_enabled;
variant_stack_t* timer_list;
int DT_TIMER;

// Service methods
variant_t*  timer_start(service_method_t* method, va_list args);
variant_t*  timer_stop(service_method_t* method, va_list args);
variant_t*  timer_start_interval(service_method_t* method, va_list args);
variant_t*  timer_invoke_command(service_method_t* method, va_list args);
variant_t*  timer_interval_invoke_command(service_method_t* method, va_list args);
variant_t*  timer_show_timers(service_method_t* method, va_list args);

void alarm_expire_handler(int sig);

void service_create(service_t** service, int service_id)
{
    SERVICE_INIT(Timer, "Provides timer services with Start and Stop methods");
    SERVICE_ADD_METHOD(Start, timer_start, 2, "Timer name (string), Timeout in seconds (int)"); // Timer.Start(<Name>, <Secs>)
    SERVICE_ADD_METHOD(Stop, timer_stop, 1, "Timer name (string)");   // Timer.Stop(<Name>)
    SERVICE_ADD_METHOD(StartInterval, timer_start_interval, 2, "Timer name (string), Interval period in seconds (int)"); // Timer.StartInterval(<Name>, <Secs>)
    SERVICE_ADD_METHOD(StopInterval, timer_stop, 1, "Timer name (string)");   // Timer.StopInterval(<Name>)
    SERVICE_ADD_METHOD(Invoke, timer_invoke_command, 2, "Command name with args (string), Timeout in seconds (int)");
    SERVICE_ADD_METHOD(IntervalInvoke, timer_interval_invoke_command, 2, "Command name with args (string), Interval timeout in seconds (int)");
    SERVICE_ADD_METHOD(Show, timer_show_timers, 0, "Display a list of pending timers");

    timer_list = stack_create();
    //self = *service;
    DT_TIMER = service_id;
    (*service)->get_config_callback = timer_cli_get_config;

    // Default config values
    timer_enabled = true;

    // Setup SIGALARM handler
    struct sigaction sa;
    sa.sa_handler = &alarm_expire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &sa, 0) == -1) 
    {
        perror(0);
    }
    else
    {
        alarm(1);
    }
}

void    service_cli_create(cli_node_t* parent_node)
{
    timer_cli_init(parent_node);
}

bool    add_timer_event(const char* name, int timeout, timer_event_type_t event_type, bool singleshot)
{
    timer_info_t* timer = (timer_info_t*)malloc(sizeof(timer_info_t));

    timer->name = strdup(name);

    switch(event_type)
    {
    case SCENE:
        timer->event_name = strdup(SCENE_ACTIVATION_EVENT);
        break;
    case COMMAND:
        timer->event_name = strdup(COMMAND_ACTIVATION_EVENT);
        break;
    }

    timer->event_type = event_type;
    timer->timeout = timer->ticks_left = timeout;
    timer->singleshot = singleshot;

    stack_push_front(timer_list, variant_create_ptr(DT_TIMER, timer, &timer_delete_timer));
}

variant_t*  timer_start(service_method_t* method, va_list args)
{
    if(timer_enabled)
    {
        //timer_info_t* timer = (timer_info_t*)malloc(sizeof(timer_info_t));
    
        variant_t* name_variant = va_arg(args, variant_t*);
        variant_t* timeout_variant = va_arg(args, variant_t*);
    
        if(name_variant->type == DT_STRING && timeout_variant->type == DT_INT32)
        {
            /*timer->name = strdup(variant_get_string(name_variant));
            timer->event_name = strdup(SCENE_ACTIVATION_EVENT);
            timer->timeout = timer->ticks_left = variant_get_int(timeout_variant);
            timer->singleshot = true;
        
            stack_push_front(timer_list, variant_create_ptr(DT_TIMER, timer, &timer_delete_timer));*/
            add_timer_event(variant_get_string(name_variant), variant_get_int(timeout_variant), SCENE, true);
            return variant_create_bool(true);
        }
        else
        {
            LOG_ERROR(DT_TIMER, "Failed to create timer");
            //free(timer);
            return variant_create_bool(false);
        }
    }
    else
    {
        return variant_create_bool(false);
    }
}

variant_t*  timer_stop(service_method_t* method, va_list args)
{
    variant_t* name_variant = va_arg(args, variant_t*);
    
    stack_for_each(timer_list, timer_variant)
    {
        timer_info_t* timer = variant_get_ptr(timer_variant);
        if(strcmp(timer->name, variant_get_string(name_variant)) == 0)
        {
            stack_remove(timer_list, timer_variant);
            variant_free(timer_variant);
            return variant_create_bool(true);
        }
    }

    return variant_create_bool(false);
}

variant_t*  timer_start_interval(service_method_t* method, va_list args)
{
    if(timer_enabled)
    {
        //timer_info_t* timer = (timer_info_t*)malloc(sizeof(timer_info_t));
    
        variant_t* name_variant = va_arg(args, variant_t*);
        variant_t* timeout_variant = va_arg(args, variant_t*);
    
        if(name_variant->type == DT_STRING && timeout_variant->type == DT_INT32)
        {
            /*timer->name = strdup(variant_get_string(name_variant));
            timer->event_name = strdup(SCENE_ACTIVATION_EVENT);
            timer->timeout = timer->ticks_left = variant_get_int(timeout_variant);
            timer->singleshot = false;
        
            stack_push_front(timer_list, variant_create_ptr(DT_TIMER, timer, &timer_delete_timer));*/
            add_timer_event(variant_get_string(name_variant), variant_get_int(timeout_variant), SCENE, false);
        
            return variant_create_bool(true);
        }
        else
        {
            LOG_ERROR(DT_TIMER, "Failed to create interval");
            //free(timer);
            return variant_create_bool(false);
        }
    }
    else
    {
        return variant_create_bool(false);
    }
}

void alarm_expire_handler(int sig)
{
    if(timer_enabled)
    {
        stack_for_each(timer_list, timer_variant)
        {
            if(NULL != timer_variant)
            {
                timer_info_t* timer = variant_get_ptr(timer_variant);
                if(--timer->ticks_left == 0)
                {
                    service_post_event(DT_TIMER, timer->event_name, variant_create_string(strdup(timer->name)));
        
                    if(timer->singleshot)
                    {
                        stack_remove(timer_list, timer_variant);
                        variant_free(timer_variant);
                    }
                    else
                    {
                        timer->ticks_left = timer->timeout;
                    }
                }
            }
        }
    
        service_post_event(DT_TIMER, TIMER_TICK_EVENT, NULL);
    }

    alarm(1);
}

void timer_delete_timer(void* arg)
{
    timer_info_t* timer = (timer_info_t*)arg;

    free(timer->name);
    free(timer->event_name);
    free(timer);
}

variant_t* call_timer_start(service_method_t* method, ...)
{
    va_list args;
    va_start(args, method);
    variant_t* result = timer_start(method, args);
    va_end(args);

    return result;
}

variant_t*  timer_invoke_command(service_method_t* method, va_list args)
{
    if(timer_enabled)
    {
        variant_t* name_variant = va_arg(args, variant_t*);
        variant_t* timeout_variant = va_arg(args, variant_t*);
    
        if(name_variant->type == DT_STRING && timeout_variant->type == DT_INT32)
        {
            add_timer_event(variant_get_string(name_variant), variant_get_int(timeout_variant), COMMAND, true);
            return variant_create_bool(true);
        }
        else
        {
            LOG_ERROR(DT_TIMER, "Failed to create interval");
            return variant_create_bool(false);
        }
    }
    else
    {
        return variant_create_bool(false);
    }
}

variant_t*  timer_interval_invoke_command(service_method_t* method, va_list args)
{
    if(timer_enabled)
    {
        variant_t* name_variant = va_arg(args, variant_t*);
        variant_t* timeout_variant = va_arg(args, variant_t*);
    
        if(name_variant->type == DT_STRING && timeout_variant->type == DT_INT32)
        {
            add_timer_event(variant_get_string(name_variant), variant_get_int(timeout_variant), COMMAND, false);
            return variant_create_bool(true);
        }
        else
        {
            LOG_ERROR(DT_TIMER, "Failed to create interval");
            return variant_create_bool(false);
        }
    }
    else
    {
        return variant_create_bool(false);
    }
}

variant_t*  timer_show_timers(service_method_t* method, va_list args)
{
    if(timer_enabled)
    {
        variant_stack_t* result = stack_create();

        stack_for_each(timer_list, timer_variant)
        {
            timer_info_t* timer = variant_get_ptr(timer_variant);
            char buf[256] = {0};
            snprintf(buf, 255, "%d %s: %s", timer->ticks_left, timer->event_name, timer->name);
            stack_push_back(result, variant_create_string(strdup(buf)));
        }

        return variant_create_list(result);
    }

    return variant_create_bool(false);
}

