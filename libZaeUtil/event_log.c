#include "event_log.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "logger.h"
#include "event.h"
#include "event_dispatcher.h"
#include "event_pump.h"
#include "resolver.h"

typedef struct event_log_handle_t
{
    variant_stack_t* event_log_list;
    int limit;
} event_log_handle_t;

event_log_handle_t  event_log_handle;

DECLARE_LOGGER(EventLog)

void event_entry_to_string(variant_t* entry_var, char** string)
{
    *string = calloc(2048, sizeof(char));
    event_entry_t* e = VARIANT_GET_PTR(event_entry_t, entry_var);
    struct tm* ptime = localtime(&e->timestamp);
    char time_str[15] = {0};
    strftime(time_str, sizeof(time_str), "%H:%M:%S", ptime);
    snprintf(*string, 2047, "%s [%s] %s (%d.%d.%d) %s",
                    time_str,
                    resolver_name_from_node_id(e->node_id),
                    resolver_name_from_id(e->node_id, e->instance_id, e->command_id), 
                    e->node_id, e->instance_id, e->command_id, e->event_data);
}

/*void delete_event_log_entry(void* arg)
{
    event_entry_t* e = (event_entry_t*)arg;
    free(e->event_data);
    free(e);
}*/

void event_log_on_new_event(event_pump_t* pump, int event_id, void* data, void* context)
{
    event_entry_t* event_entry = (event_entry_t*)data;
    LOG_DEBUG(EventLog, "Adding new event from pump");
    event_log_add_event(event_entry);
}

void    event_log_init()
{
    event_log_handle.event_log_list = stack_create();
    event_log_handle.limit = 10000;
    variant_register_converter_string(DT_EVENT_LOG_ENTRY, event_entry_to_string);

    event_pump_t* pump = event_dispatcher_get_pump("EVENT_PUMP");
    event_dispatcher_register_handler(pump, EventLogNewEvent, event_log_on_new_event, NULL);

    LOG_INFO(EventLog, "Event log initialized");
}



void    event_log_add_event(event_entry_t* event_entry)
{
    while(event_log_handle.event_log_list->count > event_log_handle.limit)
    {
        variant_t* e = stack_pop_back(event_log_handle.event_log_list);
        variant_free(e);
    }

    variant_t* entry_variant = variant_create_ptr(DT_EVENT_LOG_ENTRY, event_entry, &variant_delete_none);
    char* event_buf;
    if(variant_to_string(entry_variant, &event_buf))
    {
        LOG_INFO(EventLog, event_buf);
        stack_push_front(event_log_handle.event_log_list, variant_create_string(entry_variant));
    }
    variant_free(entry_variant);
}

void    event_log_set_limit(int limit)
{
    event_log_handle.limit = limit;
    while(event_log_handle.event_log_list->count > event_log_handle.limit)
    {
        variant_t* e = stack_pop_back(event_log_handle.event_log_list);
        variant_free(e);
    }
}

void    event_log_for_each(void (*callback)(event_entry_t*, void*), void* arg)
{
    stack_for_each(event_log_handle.event_log_list, event_variant)
    {
        event_entry_t* e = VARIANT_GET_PTR(event_entry_t, event_variant);
        callback(e, arg);
    }
}

variant_stack_t* event_log_get_tail(int lines)
{
    variant_stack_t* result_list = stack_create();
    stack_iterator_t* it = stack_iterator_begin(event_log_get_list());

    while(lines-- > 0 && !stack_iterator_is_end(it))
    {
        variant_t* event_entry_var = stack_iterator_data(it);
        stack_push_front(result_list, variant_clone(event_entry_var)); // Create ascending list by time

        it = stack_iterator_next(it);
    }
    stack_iterator_free(it);

    return result_list;
}

variant_stack_t* event_log_get_list()
{
    return event_log_handle.event_log_list;
}

int     event_log_get_size()
{
    return event_log_handle.event_log_list->count;
}
