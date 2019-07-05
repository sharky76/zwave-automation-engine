#include "event_pump.h"
#include "stack.h"
#include "hash.h"
#include <assert.h>
#include "logger.h"
#include <pthread.h>
#include <sys/eventfd.h>

USING_LOGGER(EventPump);
/* 
    Each event consist of ID (name) and data
    A client can submit events to the pump by calling event_pump_send_event(event_id, data)
    This event is stored in thread safe queue and a registered handler will be called upon the 
    next call to poll() by a dispatcher

    
 */
typedef struct event_st
{
    int event_id;
    void *event_data;
} event_t;

typedef struct event_handler_cb_st
{
    void (*on_event)(event_t *);
} event_handler_cb_t;

typedef struct event_handler_st
{
    int event_id;
    variant_stack_t *handlers; // List of event_handler_cb_t
} event_handler_t;

void variant_delete_event_handler(void *arg)
{
    event_handler_t *e = (event_handler_t *)arg;
    stack_free(e->handlers);
    free(e);
}

typedef struct event_pump_data_t
{
    hash_table_t *event_handlers;
    pthread_mutex_t queue_lock;
    int next_queue_slot;
    int next_peniding_queue_slot;
    event_t event_queue[MAX_QUEUE_SIZE];
    event_t pending_event_queue[MAX_QUEUE_SIZE];
    event_t* current_event_queue_ptr;
    event_t* pending_event_queue_ptr;
} event_pump_data;

void event_pump_init(event_pump_t *pump)
{
    LOG_ADVANCED(EventPump, "Initializing Event Pump...");
    pump->name = strdup("EVENT_PUMP");
    pump->poll = &event_pump_poll;
    pump->register_handler = &event_pump_register_handler;
    pump->unregister_handler = &event_pump_unregister_handler;
    pump->start = NULL;
    pump->stop = NULL;
    pump->priv = (event_pump_data *)malloc(sizeof(event_pump_data));
    event_pump_data *data = (event_pump_data *)pump->priv;

    memset(data->event_queue, 0, sizeof(data->event_queue));
    data->next_queue_slot = 0;
    data->next_peniding_queue_slot = 0;
    data->event_handlers = variant_hash_init();
    
    data->current_event_queue_ptr = data->event_queue;
    data->pending_event_queue_ptr = data->pending_event_queue;
    
    pthread_mutex_init(&data->queue_lock, NULL);
}

void event_pump_send_event(event_pump_t *pump, int event_id, void *event_data)
{
    event_pump_data *data = (event_pump_data *)pump->priv;

    LOG_DEBUG(EventPump, "Event Pump sending event...");
    if(pthread_mutex_trylock(&data->queue_lock) == 0)
    {
        if (data->next_queue_slot < MAX_QUEUE_SIZE)
        {
            data->current_event_queue_ptr[data->next_queue_slot].event_id = event_id;
            data->current_event_queue_ptr[data->next_queue_slot].event_data = event_data;
            data->next_queue_slot++;
        }
        pthread_mutex_unlock(&data->queue_lock);
    }
    else
    {
        if (data->next_peniding_queue_slot < MAX_QUEUE_SIZE)
        {
            data->pending_event_queue_ptr[data->next_peniding_queue_slot].event_id = event_id;
            data->pending_event_queue_ptr[data->next_peniding_queue_slot].event_data = event_data;
            data->next_peniding_queue_slot++;
        }
    }
}

void event_pump_register_handler(event_pump_t *pump, va_list args)
{
    event_pump_data *data = (event_pump_data *)pump->priv;

    int event_id = va_arg(args, int);
    void (*on_event)(event_t *);
    on_event = va_arg(args, void (*)(event_t *));

    variant_t *event_handler_variant = variant_hash_get(data->event_handlers, event_id);
    if (NULL == event_handler_variant)
    {
        event_handler_t *event_handlers = (event_handler_t *)malloc(sizeof(event_handler_t));
        event_handlers->event_id = event_id;
        event_handlers->handlers = stack_create();
        event_handler_variant = variant_create_ptr(DT_PTR, event_handlers, variant_delete_event_handler);
        variant_hash_insert(data->event_handlers, event_id, event_handler_variant);
    }

    event_handler_t *event_handlers = VARIANT_GET_PTR(event_handler_t, event_handler_variant);
    event_handler_cb_t *event_handler_cb = (event_handler_cb_t *)malloc(sizeof(event_handler_cb_t));
    event_handler_cb->on_event = on_event;

    stack_push_back(event_handlers->handlers, variant_create_ptr(DT_PTR, event_handler_cb, variant_delete_default));
}

void event_pump_unregister_handler(event_pump_t *pump, va_list args)
{
    event_pump_data *data = (event_pump_data *)pump->priv;

    int event_id = va_arg(args, int);
    void (*on_event)(event_t *);
    on_event = va_arg(args, void (*)(event_t *));

    variant_t *event_handler_variant = variant_hash_get(data->event_handlers, event_id);
    if (NULL != event_handler_variant)
    {
        event_handler_t *event_handlers = VARIANT_GET_PTR(event_handler_t, event_handler_variant);
        stack_for_each(event_handlers->handlers, event_handler_variant)
        {
            event_handler_cb_t *event_handler_cb = VARIANT_GET_PTR(event_handler_cb_t, event_handler_variant);
            if (event_handler_cb->on_event == on_event)
            {
                stack_remove(event_handlers->handlers, event_handler_variant);
                break;
            }
        }
    }
}

void event_pump_poll(event_pump_t *pump, struct timespec *ts)
{
    event_pump_data *data = (event_pump_data *)pump->priv;

    if(0 == data->next_queue_slot) return;
    
    pthread_mutex_lock(&data->queue_lock);

    //LOG_DEBUG(EventPump, "Event Pump processing events: %d", data->next_queue_slot);
    for (int i = 0; i < data->next_queue_slot; i++)
    {
        event_t *e = &data->current_event_queue_ptr[i];
        variant_t *event_handler_variant = variant_hash_get(data->event_handlers, e->event_id);
        assert(NULL != event_handler_variant);

        if (NULL != event_handler_variant)
        {
            event_handler_t *event_handlers = VARIANT_GET_PTR(event_handler_t, event_handler_variant);
            stack_for_each(event_handlers->handlers, event_handler_variant)
            {
                event_handler_cb_t *event_handler_cb = VARIANT_GET_PTR(event_handler_cb_t, event_handler_variant);
                event_handler_cb->on_event(e);
            }
        }
    }

    data->next_queue_slot = 0;

    // Switch current with pending:
    int tmp_next_slot = data->next_queue_slot;
    data->current_event_queue_ptr = data->pending_event_queue;
    data->pending_event_queue_ptr = data->event_queue;

    data->next_queue_slot = data->next_peniding_queue_slot;
    data->next_peniding_queue_slot = tmp_next_slot;

    pthread_mutex_unlock(&data->queue_lock);
}