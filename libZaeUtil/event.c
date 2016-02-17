#include "event.h"
#include <stdlib.h>
#include <signal.h>

variant_stack_t* event_list;

void    event_destroy(void* arg);

event_t*    event_create(int source_id, variant_t* source)
{
    event_t* new_event = (event_t*)malloc(sizeof(event_t));
    new_event->source = source;
    new_event->source_id = source_id;
    return new_event;
}

void        event_delete(event_t* event)
{
    variant_free(event->source);
    free(event);
}

void        event_post(event_t* event)
{
    stack_push_back(event_list, variant_create_ptr(DT_EVENT, event, NULL));
    raise(SIGUSR1);
}

event_t*    event_receive()
{
    variant_t* event_variant = stack_pop_front(event_list);
    event_t* event = (event_t*)variant_get_ptr(event_variant);
    free(event_variant);
    return event;
}
