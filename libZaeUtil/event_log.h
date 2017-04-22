#pragma once

#include "stack.h"
#include <time.h>

typedef struct event_log_entry_t
{
    int node_id;
    int instance_id;
    int command_id;
    char* event_data; 
    time_t   timestamp;
} event_log_entry_t;

#define EVENT_LOG_ADDED_EVENT "EventLogAddedEvent"

void    event_log_init();
void    event_log_add_event(event_log_entry_t* event_entry);
void    event_log_set_limit(int limit);
void    event_log_for_each(void (*callback)(event_log_entry_t*, void*), void* arg);
variant_stack_t* event_log_get_tail(int lines);
variant_stack_t* event_log_get_list();


