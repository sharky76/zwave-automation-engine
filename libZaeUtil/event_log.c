#include "event_log.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "logger.h"
#include "event.h"

static uint32_t monotonic_event_id;

typedef struct event_log_handle_t
{
    variant_stack_t* event_log_list;
    int limit;
} event_log_handle_t;

event_log_handle_t  event_log_handle;

DECLARE_LOGGER(EventLog)

void event_log_entry_to_string(variant_t* entry_var, char** string)
{
    event_log_entry_t* e = VARIANT_GET_PTR(event_log_entry_t, entry_var);

    *string = calloc(256, sizeof(char));
    struct tm* ptime = localtime(&e->timestamp);
    char time_str[15] = {0};
    strftime(time_str, sizeof(time_str), "%H:%M:%S ", ptime);

    snprintf(*string, 255, "%s %d.%d.%d %s", time_str, e->node_id, e->instance_id, e->command_id, e->event_data);
}

void delete_event_log_entry(void* arg)
{
    event_log_entry_t* e = (event_log_entry_t*)arg;
    free(e->event_data);
    free(e);
}

void    event_log_init()
{
    event_log_handle.event_log_list = stack_create();
    event_log_handle.limit = 100;
    variant_register_converter_string(DT_EVENT_LOG_ENTRY, event_log_entry_to_string);

    LOG_INFO(EventLog, "Event log initialized");
}

void    event_log_add_event(event_log_entry_t* event_entry)
{
    while(event_log_handle.event_log_list->count > event_log_handle.limit)
    {
        variant_t* e = stack_pop_back(event_log_handle.event_log_list);
        variant_free(e);
    }

    event_entry->event_id = monotonic_event_id++;
    event_entry->timestamp = time(NULL);
    variant_t* entry_variant = variant_create_ptr(DT_EVENT_LOG_ENTRY, event_entry, &delete_event_log_entry);
    stack_push_front(event_log_handle.event_log_list, entry_variant);

    // Post event when new log entry is added
    event_t* event = event_create(EventLog, EVENT_LOG_ADDED_EVENT, variant_clone(entry_variant));
    event_post(event);
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

void    event_log_for_each(void (*callback)(event_log_entry_t*, void*), void* arg)
{
    stack_for_each(event_log_handle.event_log_list, event_variant)
    {
        event_log_entry_t* e = VARIANT_GET_PTR(event_log_entry_t, event_variant);
        callback(e, arg);
    }
}

variant_stack_t* event_log_get_tail(int lines)
{
    variant_stack_t* result_list = stack_create();

    stack_for_each_range(event_log_handle.event_log_list, 0, lines, event_entry_var)
    {
        stack_push_back(result_list, variant_clone(event_entry_var));
    }

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
