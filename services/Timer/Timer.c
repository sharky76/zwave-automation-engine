#include <service.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <logger.h>
#include "timer_cli.h"
#include "timer_config.h"
#include "event.h"
#include "timer_pump.h"

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

void* timer_alarm_thread_func(void* arg);
void  timer_thread_start();


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

    //timer_thread_start();
}

void    service_cli_create(cli_node_t* parent_node)
{
    timer_cli_init(parent_node);
}

void    on_timer_expired(event_pump_t* pump, int timer_id, void* context)
{
    timer_info_t* timer = (timer_info_t*)context;
    service_post_event(DT_TIMER, timer->event_id, strdup(timer->name));

    if(timer_pump_is_singleshot(pump, timer_id))
    {
        stack_lock(timer_list);
        stack_for_each(timer_list, timer_variant)
        {
            timer_info_t* timer = variant_get_ptr(timer_variant);
            if(timer->timer_id == timer_id)
            {
                event_pump_t* pump = event_dispatcher_get_pump("TIMER_PUMP");
                event_dispatcher_unregister_handler(pump, timer->timer_id);
                stack_remove(timer_list, timer_variant);
                variant_free(timer_variant);
                break;
            }
        }
        stack_unlock(timer_list);
    }
}

bool    add_timer_event(const char* name, int timeout, int event_id, bool singleshot)
{
    timer_info_t* timer = (timer_info_t*)malloc(sizeof(timer_info_t));
    timer->name = strdup(name);
    timer->event_id = event_id;
    event_pump_t* pump = event_dispatcher_get_pump("TIMER_PUMP");
    timer->timer_id = event_dispatcher_register_handler(pump, timeout*1000, singleshot, &on_timer_expired, (void*)timer);
    pump->start(pump, timer->timer_id);

    stack_lock(timer_list);
    stack_push_front(timer_list, variant_create_ptr(DT_TIMER, timer, &timer_delete_timer));
    stack_unlock(timer_list);
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
            add_timer_event(variant_get_string(name_variant), variant_get_int(timeout_variant), SceneActivationEvent, true);
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
    bool retVal = false;
    variant_t* name_variant = va_arg(args, variant_t*);
    if(NULL != variant_get_string(name_variant))
    {
        stack_lock(timer_list);
        stack_for_each(timer_list, timer_variant)
        {
            timer_info_t* timer = variant_get_ptr(timer_variant);
            if(strcmp(timer->name, variant_get_string(name_variant)) == 0)
            {
                event_pump_t* pump = event_dispatcher_get_pump("TIMER_PUMP");
                event_dispatcher_unregister_handler(pump, timer->timer_id);
                stack_remove(timer_list, timer_variant);
                variant_free(timer_variant);
                retVal = true;
            }
        }
        stack_unlock(timer_list);
    }

    return variant_create_bool(retVal);
}

variant_t*  timer_stop_old(service_method_t* method, va_list args)
{
    bool retVal = false;
    variant_t* name_variant = va_arg(args, variant_t*);
    if(NULL != variant_get_string(name_variant))
    {
        stack_lock(timer_list);
        stack_for_each(timer_list, timer_variant)
        {
            timer_info_t* timer = variant_get_ptr(timer_variant);
            if(strcmp(timer->name, variant_get_string(name_variant)) == 0)
            {
                stack_remove(timer_list, timer_variant);
                variant_free(timer_variant);
                retVal = true;
            }
        }
        stack_unlock(timer_list);
    }

    return variant_create_bool(retVal);
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
            add_timer_event(variant_get_string(name_variant), variant_get_int(timeout_variant), SceneActivationEvent, false);
       
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

void timer_delete_timer(void* arg)
{
    timer_info_t* timer = (timer_info_t*)arg;

    free(timer->name);
    free(timer);
}

variant_t*  timer_invoke_command(service_method_t* method, va_list args)
{
    if(timer_enabled)
    {
        variant_t* name_variant = va_arg(args, variant_t*);
        variant_t* timeout_variant = va_arg(args, variant_t*);
    
        if(name_variant->type == DT_STRING && timeout_variant->type == DT_INT32)
        {
            //add_timer_event(variant_get_string(name_variant), variant_get_int(timeout_variant), COMMAND, true);
            add_timer_event(variant_get_string(name_variant), variant_get_int(timeout_variant), CommandActivationEvent, true);
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
            //add_timer_event(variant_get_string(name_variant), variant_get_int(timeout_variant), COMMAND, false);
            add_timer_event(variant_get_string(name_variant), variant_get_int(timeout_variant), CommandActivationEvent, false);
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

        stack_lock(timer_list);
        stack_for_each(timer_list, timer_variant)
        {
            timer_info_t* timer = variant_get_ptr(timer_variant);
            char buf[256] = {0};
            snprintf(buf, 255, "%s", timer->name);
            stack_push_back(result, variant_create_string(strdup(buf)));
        }
        stack_unlock(timer_list);
        return variant_create_list(result);
    }

    return variant_create_bool(false);
}

