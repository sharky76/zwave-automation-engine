#include "event.h"
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include "logger.h"
#include "hash.h"
#include <string.h>
#include <sys/eventfd.h>

variant_stack_t* event_list;
variant_stack_t* registered_handlers;
//variant_stack_t* registered_types;
//hash_table_t*    registered_handlers;

int     event_fd;
bool    event_manager_running;
pthread_t   event_manager_thread;

typedef struct fd_event_listener_t
{
    int fd;
    void (*event_handler)(int, void*);
    void* context;
} fd_event_listener_t;

variant_stack_t*    fd_event_listener_list;
variant_stack_t*    fd_event_pending_registration_list;
variant_stack_t*    fd_event_pending_unregistration_list;

void     event_destroy(void* arg);
event_t* event_receive();

void    delete_event_handler(void* arg)
{
    event_handler_t* e = (event_handler_t*)arg;
    free(e->event_name);
    free(e);
}

bool    event_handler_match(variant_t* e, void* arg)
{
    event_handler_t* ref = (event_handler_t*)arg;
    event_handler_t* h = VARIANT_GET_PTR(event_handler_t, e);
    return (h->id == ref->id && !strcmp(h->event_name, ref->event_name));
}

DECLARE_LOGGER(Event)

void* event_handle_event(void* arg);

void event_manager_init()
{
    LOG_ADVANCED(Event, "Initializing event manager");
    event_list = stack_create();
    registered_handlers = stack_create();
    fd_event_listener_list = stack_create();
    fd_event_pending_registration_list = stack_create();
    fd_event_pending_unregistration_list = stack_create();
    //registered_types = stack_create();
    
    //pthread_mutex_init(&event_list_mutex, NULL);

    // Init the semaphore
    //sem_init(&event_semaphore, 0, 0);
    event_fd = eventfd(0, EFD_SEMAPHORE);

    // Now create event thread
    event_manager_running = true;
    
    pthread_create(&event_manager_thread, NULL, event_handle_event, NULL);

    LOG_ADVANCED(Event, "Event manager initialized");
}

void        event_manager_shutdown()
{
    LOG_ADVANCED(Event, "Event manager shutdown");
    event_manager_running = false;
    eventfd_write(event_fd, 1);
    pthread_join(event_manager_thread, NULL);
}

void        event_manager_join()
{
    pthread_join(event_manager_thread, NULL);
}


event_t*    event_create(int source_id, const char* event_name, variant_t* data)
{
    if(event_manager_running)
    {
        event_t* new_event = (event_t*)malloc(sizeof(event_t));
        new_event->data = data;
        new_event->source_id = source_id;
        new_event->name = strdup(event_name);
        return new_event;
    }
}

void        event_delete(event_t* event)
{
    variant_free(event->data);
    free(event->name);
    free(event);
}

void        event_post(event_t* event)
{
    if(event_manager_running)
    {
        //printf("Event post to %p\n", event_list);
        //pthread_mutex_lock(&event_list_mutex);
        stack_lock(event_list);
        stack_push_back(event_list, variant_create_ptr(DT_PTR, event, NULL));
        stack_unlock(event_list);
        //pthread_mutex_unlock(&event_list_mutex);
        //sem_post(&event_semaphore);

        // Write event value
        //printf("Post event to event fd %d\n", event_fd);
        //printf("Received event type %s from id %d", event->name, event->source_id);
        eventfd_write(event_fd, 1);
        //raise(SIGUSR1);
    }
}

event_t*    event_receive()
{
    //pthread_mutex_lock(&event_list_mutex);
    stack_lock(event_list);
    variant_t* event_variant = stack_pop_front(event_list);
    stack_unlock(event_list);
    //pthread_mutex_unlock(&event_list_mutex);

    if(NULL != event_variant)
    {
        event_t* event = (event_t*)variant_get_ptr(event_variant);
        free(event_variant);
        return event;
    }
    
    return NULL;
}

void        event_register_handler(int handler_id, const char* event_name, void (*event_handler)(event_t*, void*), void* context)
{
    LOG_DEBUG(Event, "Register handler %d for event %s with callback %p\n", handler_id, event_name, event_handler);
    event_handler_t* handler = malloc(sizeof(event_handler_t));
    handler->id = handler_id;
    handler->event_name = strdup(event_name);
    handler->event_handler = event_handler;
    handler->context = context;

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

void event_unregister_handler(int handler_id, const char* event_name)
{
    stack_for_each(registered_handlers, handler_var)
    {
        event_handler_t* event_handler = VARIANT_GET_PTR(event_handler_t, handler_var);
        if(event_handler->id == handler_id && strcmp(event_handler->event_name, event_name) == 0)
        {
            stack_remove(registered_handlers, handler_var);
            variant_free(handler_var);
            break;
        }
    }
}

void        event_register_fd(int fd, void (*event_handler)(int, void*), void* context)
{
    fd_event_listener_t* event_listener = malloc(sizeof(fd_event_listener_t));
    event_listener->fd = fd;
    event_listener->event_handler = event_handler;
    event_listener->context = context;

    if(stack_trylock(fd_event_listener_list))
    {
        stack_push_back(fd_event_listener_list, variant_create_ptr(DT_PTR, event_listener, &variant_delete_default));
        stack_unlock(fd_event_listener_list);
    }
    else
    {
        stack_lock(fd_event_pending_registration_list);
        stack_push_back(fd_event_pending_registration_list, variant_create_ptr(DT_PTR, event_listener, &variant_delete_default));
        stack_unlock(fd_event_pending_registration_list);
    }

    // Break the wait to reinitialize descriptor list
    eventfd_write(event_fd, 1);

    LOG_ADVANCED(Event, "Registered listener for FD %d", fd);
}

void        event_unregister_fd(int fd)
{
    if(stack_trylock(fd_event_listener_list))
    {
        stack_for_each(fd_event_listener_list, event_listener_variant)
        {
            fd_event_listener_t* event_listener = VARIANT_GET_PTR(fd_event_listener_t, event_listener_variant);
            if(event_listener->fd == fd)
            {
                stack_remove(fd_event_listener_list, event_listener_variant);
                variant_free(event_listener_variant);
                break;
            }
        }
        stack_unlock(fd_event_listener_list);
    }
    else
    {
        stack_lock(fd_event_pending_unregistration_list);
        stack_push_back(fd_event_pending_unregistration_list, variant_create_int32(DT_INT32, fd));
        stack_unlock(fd_event_pending_unregistration_list);
    }

    // Break the wait to reinitialize descriptor list
    eventfd_write(event_fd, 1);

    LOG_ADVANCED(Event, "Unregistered listener for FD %d", fd);
}

/** 
 * 
 * 
 */
void process_pending_registrations()
{
    stack_lock(fd_event_pending_registration_list);
    stack_for_each(fd_event_pending_registration_list, pending_reg_variant)
    {
        stack_lock(fd_event_listener_list);
        stack_push_back(fd_event_listener_list, pending_reg_variant);
        stack_unlock(fd_event_listener_list);
    }

    while(fd_event_pending_registration_list->count > 0)
    {
        stack_pop_front(fd_event_pending_registration_list);
    }

    stack_unlock(fd_event_pending_registration_list);
}
/** 
 * 
 * 
 */
void process_pending_unregistrations()
{
    stack_lock(fd_event_pending_unregistration_list);
    stack_for_each(fd_event_pending_unregistration_list, pending_unreg_variant)
    {
        event_unregister_fd(variant_get_int(pending_unreg_variant));
    }
    stack_empty(fd_event_pending_unregistration_list);
    stack_unlock(fd_event_pending_unregistration_list);
}

void* event_handle_event(void* arg)
{
    while(event_manager_running)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(event_fd, &fds);

        stack_lock(fd_event_listener_list);
        stack_for_each(fd_event_listener_list, event_listener_variant)
        {
            fd_event_listener_t* event_listener = VARIANT_GET_PTR(fd_event_listener_t, event_listener_variant);
            FD_SET(event_listener->fd, &fds);
        }
        stack_unlock(fd_event_listener_list);

        int ret = select(FD_SETSIZE, &fds, NULL, NULL, NULL);

        if(ret <= 0)
        {
            continue;
        }

        //sem_wait(&event_semaphore);
        if(FD_ISSET(event_fd, &fds))
        {
            eventfd_t event_count;
            eventfd_read(event_fd, &event_count);

            event_t* event = event_receive();

            if(NULL != event)
            {
                LOG_ADVANCED(Event, "Received event type %s from id %d", event->name, event->source_id);
        
                stack_for_each(registered_handlers, handler_var)
                {
                    event_handler_t* event_handler = VARIANT_GET_PTR(event_handler_t, handler_var);
                    if(strcmp(event_handler->event_name, event->name) == 0)
                    {
                        //printf("Found registered event type with id %d and callback %p\n", event_handler->id, event_handler->event_handler);
                        LOG_DEBUG(Event, "Found registered event type with id %d", event_handler->id);
                        void (*event_handler_cb)(event_t*,void*);
            
                        event_handler_cb = event_handler->event_handler;
                        event_handler_cb(event, event_handler->context);
                    }
                }
                event_delete(event);
            }
        }
        
        // Must be inside brackets
        {

            stack_lock(fd_event_listener_list);
            stack_for_each(fd_event_listener_list, event_listener_variant)
            {
                fd_event_listener_t* event_listener = VARIANT_GET_PTR(fd_event_listener_t, event_listener_variant);
                if(FD_ISSET(event_listener->fd, &fds))
                {
                    LOG_ADVANCED(Event, "Received event for FD %d", event_listener->fd);
                    event_listener->event_handler(event_listener->fd, event_listener->context);
                }
            }
            stack_unlock(fd_event_listener_list);
        }

        // Process pending FD registrations...
        process_pending_registrations();

        // Process pending FD unregistrations...
        process_pending_unregistrations();
    }
}

typedef struct event_wait_context_t
{
    pthread_cond_t* wait_cond;
    const char*     event_name;
    variant_t*      event_data;
} event_wait_context_t;

void    event_wait_handler(event_t* event, void* arg)
{
    event_wait_context_t* event_ctx = (event_wait_context_t*)arg;
    if(strcmp(event_ctx->event_name, event->name) == 0)
    {
        LOG_DEBUG(Event, "Wait for %s completed", event->name);
        event_ctx->event_data = variant_clone(event->data);
        pthread_cond_signal(event_ctx->wait_cond);
    }
}

variant_t*    event_wait(int source_id, const char* event_name, uint32_t timeout)
{
    pthread_mutex_t wait_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t  wait_cond = PTHREAD_COND_INITIALIZER;
    int retVal;

    event_wait_context_t    event_wait_ctx = {
        .wait_cond = &wait_cond,
        .event_name = event_name,
        .event_data = NULL
    };

    event_register_handler(source_id, event_name, event_wait_handler, &event_wait_ctx);
    pthread_mutex_lock(&wait_lock);
    if(timeout != 0)
    {
        time_t duration = time(NULL);
        struct timespec ts = {
            .tv_sec = duration + timeout / 1000,
            .tv_nsec = (timeout % 1000) * 1000000
        };
    
        LOG_DEBUG(Event, "Waiting for: %d sec %d nsec", ts.tv_sec, ts.tv_nsec);
        retVal = pthread_cond_timedwait(&wait_cond, &wait_lock, &ts);
    }
    else
    {
        retVal = pthread_cond_wait(&wait_cond, &wait_lock);
    }
    pthread_mutex_unlock(&wait_lock);

    event_unregister_handler(source_id, event_name);
    return event_wait_ctx.event_data;
}
