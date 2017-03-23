#include "event.h"
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include "logger.h"
#include "hash.h"
#include <string.h>

variant_stack_t* event_list;
variant_stack_t* registered_handlers;
//variant_stack_t* registered_types;
//hash_table_t*    registered_handlers;

pthread_mutex_t  event_list_mutex;
sem_t   event_semaphore;

void    event_destroy(void* arg);

void    delete_event_handler(void* arg)
{
    event_handler_t* e = (event_handler_t*)arg;
    free(e->name);
    free(e);
}

bool    event_handler_match(variant_t* e, void* arg)
{
    event_handler_t* ref = (event_handler_t*)arg;
    event_handler_t* h = VARIANT_GET_PTR(event_handler_t, e);
    return (h->id == ref->id && !strcmp(h->name, ref->name));
}

DECLARE_LOGGER(Event)

void* event_handle_event(void* arg);

void event_manager_init()
{
    LOG_ADVANCED(Event, "Initializing event manager");
    event_list = stack_create();
    registered_handlers = stack_create();
    //registered_types = stack_create();

    // Init the semaphore
    sem_init(&event_semaphore, 0, 0);

    // Now create event thread
    pthread_t   event_thread;
    pthread_create(&event_thread, NULL, event_handle_event, NULL);
    pthread_detach(event_thread);

    LOG_ADVANCED(Event, "Event manager initialized");
}

event_t*    event_create(int source_id, const char* name, variant_t* data)
{
    event_t* new_event = (event_t*)malloc(sizeof(event_t));
    new_event->data = data;
    new_event->source_id = source_id;
    new_event->name = strdup(name);
    return new_event;
}

void        event_delete(event_t* event)
{
    variant_free(event->data);
    free(event->name);
    free(event);
}

void        event_post(event_t* event)
{
    //printf("Event post to %p\n", event_list);
    pthread_mutex_lock(&event_list_mutex);
    stack_push_back(event_list, variant_create_ptr(DT_PTR, event, NULL));
    pthread_mutex_unlock(&event_list_mutex);
    sem_post(&event_semaphore);
    //raise(SIGUSR1);
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

void        event_register_handler(int handler_id, const char* event_name, void (*event_handler)(event_t*))
{
    /*if(!variant_hash_get(registered_handlers, handler_id))
    {
        variant_hash_insert(registered_handlers, handler_id, variant_create_ptr(DT_PTR, event_handler, NULL));
    }
    else
    {
        LOG_ERROR(Event, "Handler for id %d already registered", source_id);
    }*/

    LOG_DEBUG(Event, "Register handler %d for event %s with callback %p\n", handler_id, event_name, event_handler);
    event_handler_t* handler = malloc(sizeof(event_handler_t));
    handler->id = handler_id;
    handler->name = strdup(event_name);
    handler->event_handler = event_handler;

    if(!stack_is_exists(registered_handlers, &event_handler_match, handler))
    {
        stack_push_back(registered_handlers, variant_create_ptr(DT_PTR, handler, &delete_event_handler));
    }
    else
    {
        //printf("Handler id %d for event %s already registered\n", handler_id, name);
        LOG_ERROR(Event, "Handler id %d for event %s already registered", handler_id, event_name);
    }
}

void* event_handle_event(void* arg)
{
    while(true)
    {
        sem_wait(&event_semaphore);
        event_t* event = event_receive();
        LOG_ADVANCED(Event, "Received event type %s from id %d", event->name, event->source_id);

        stack_for_each(registered_handlers, handler_var)
        {
            event_handler_t* event_handler = VARIANT_GET_PTR(event_handler_t, handler_var);
            if(strcmp(event_handler->name, event->name) == 0)
            {
                //printf("Found registered event type with id %d and callback %p\n", event_handler->id, event_handler->event_handler);
                LOG_DEBUG(Event, "Found registered event type with id %d", event_handler->id);
                void (*event_handler_cb)(event_t*);
    
                event_handler_cb = event_handler->event_handler;
                event_handler_cb(event);
            }
        }
        event_delete(event);
    }
}

