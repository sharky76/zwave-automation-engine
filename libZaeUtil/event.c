#include "event.h"
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

variant_stack_t* event_list;
pthread_mutex_t  event_list_mutex;

void    event_destroy(void* arg);

event_t*    event_create(int source_id, variant_t* data)
{
    event_t* new_event = (event_t*)malloc(sizeof(event_t));
    new_event->data = data;
    new_event->source_id = source_id;
    return new_event;
}

void        event_delete(event_t* event)
{
    variant_free(event->data);
    free(event);
}

void        event_post(event_t* event)
{
    //printf("Event post to %p\n", event_list);
    pthread_mutex_lock(&event_list_mutex);
    stack_push_back(event_list, variant_create_ptr(DT_PTR, event, NULL));
    pthread_mutex_unlock(&event_list_mutex);
    raise(SIGUSR1);
}

event_t*    event_receive()
{
    pthread_mutex_lock(&event_list_mutex);
    variant_t* event_variant = stack_pop_front(event_list);
    pthread_mutex_unlock(&event_list_mutex);
    event_t* event = (event_t*)variant_get_ptr(event_variant);
    free(event_variant);
    return event;
}
