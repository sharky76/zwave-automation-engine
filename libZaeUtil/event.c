#include "event.h"
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include "logger.h"

variant_stack_t* event_list;
variant_stack_t* registered_handlers;

pthread_mutex_t  event_list_mutex;
sem_t   event_semaphore;

void    event_destroy(void* arg);

DECLARE_LOGGER(Event)

void* event_handle_event(void* arg);

void event_manager_init()
{
    LOG_ADVANCED(Event, "Initializing event manager");
    event_list = stack_create();
    registered_handlers = stack_create();

    // Init the semaphore
    sem_init(&event_semaphore, 0, 0);

    // Now create event thread
    pthread_t   event_thread;
    pthread_create(&event_thread, NULL, event_handle_event, NULL);
    pthread_detach(event_thread);

    LOG_ADVANCED(Event, "Event manager initialized");
}

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

void        event_register_handler(void (*event_handler)(event_t*))
{
    stack_push_back(registered_handlers, variant_create_ptr(DT_PTR, event_handler, NULL));
}

void* event_handle_event(void* arg)
{
    while(true)
    {
        sem_wait(&event_semaphore);
        LOG_DEBUG(Event, "Event received");
        event_t* event = event_receive();
        LOG_ADVANCED(Event, "Received event id %d", event->source_id);

        stack_for_each(registered_handlers, handler_var)
        {
            void (*event_handler)(event_t*);

            event_handler = (void(*)(event_t*))variant_get_ptr(handler_var);
            event_handler(event);
        }
        event_delete(event);
    }
}
