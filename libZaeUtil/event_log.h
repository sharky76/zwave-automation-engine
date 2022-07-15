#pragma once

#include "stack.h"
#include "event.h"
#include <time.h>

#define EVENT_LOG_ADDED_EVENT "EventLogAddedEvent"

void    event_log_init();
void    event_log_add_event(event_entry_t* event_entry);
void    event_log_set_limit(int limit);
void    event_log_for_each(void (*callback)(event_entry_t*, void*), void* arg);
variant_stack_t* event_log_get_tail(int lines);
variant_stack_t* event_log_get_list();
int     event_log_get_size();


