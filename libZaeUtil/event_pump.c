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
typedef struct pending_event_t
{
    int event_id;
    void *event_data;
    void (*event_free)(void*);
    bool processing;
} pending_event_t;

typedef struct event_handler_cb_st
{
    void (*on_event)(event_pump_t*, int, void*, void*);
    void* context;
    bool remove_pending_flag;
} event_handler_cb_t;

typedef struct event_queue_t
{
    int next_slot;
    pending_event_t queue[MAX_QUEUE_SIZE];
    pthread_mutex_t lock;
} event_queue_t;

typedef struct event_pump_data_t
{
    hash_table_t *event_handlers;
    pthread_mutex_t pump_lock;
    event_queue_t current_event_queue;
    event_queue_t pending_event_queue;
    event_queue_t* current_event_queue_ptr;
    event_queue_t* pending_event_queue_ptr;

} event_pump_data_t;

void event_pump_init(event_pump_t *pump)
{
    LOG_ADVANCED(EventPump, "Initializing Event Pump...");
    pump->name = strdup("EVENT_PUMP");
    pump->poll = &event_pump_poll;
    pump->register_handler = &event_pump_register_handler;
    pump->unregister_handler = &event_pump_unregister_handler;
    pump->start = NULL;
    pump->stop = NULL;
    pump->free = &event_pump_free;
    pump->priv = (event_pump_data_t *)calloc(1, sizeof(event_pump_data_t));
    event_pump_data_t *data = (event_pump_data_t*)pump->priv;

    data->event_handlers = variant_hash_init();
    data->current_event_queue_ptr = &data->current_event_queue;
    data->pending_event_queue_ptr = &data->pending_event_queue;
    
    pthread_mutex_init(&data->current_event_queue.lock, NULL);
    pthread_mutex_init(&data->pending_event_queue.lock, NULL);
    pthread_mutex_init(&data->pump_lock, NULL);
}

void event_pump_free(event_pump_t* pump)
{
    LOG_ADVANCED(EventPump, "Destroying Event Pump...");
    free(pump->name);
    event_pump_data_t *data = (event_pump_data_t*)pump->priv;
    variant_hash_free(data->event_handlers);
    free(data);
}

void event_pump_send_event(event_pump_t *pump, int event_id, void *event_data, void (*event_free)(void*))
{
    event_pump_data_t *data = (event_pump_data_t*)pump->priv;

    LOG_DEBUG(EventPump, "Event Pump sending event...");
    pthread_mutex_lock(&data->pump_lock);
    if (data->pending_event_queue_ptr->next_slot < MAX_QUEUE_SIZE)
    {
        data->pending_event_queue_ptr->queue[data->pending_event_queue_ptr->next_slot].event_id = event_id;
        data->pending_event_queue_ptr->queue[data->pending_event_queue_ptr->next_slot].event_data = event_data;
        data->pending_event_queue_ptr->queue[data->pending_event_queue_ptr->next_slot].event_free = event_free;
        data->pending_event_queue_ptr->queue[data->pending_event_queue_ptr->next_slot].processing = false;
        data->pending_event_queue_ptr->next_slot++;
    }
    pthread_mutex_unlock(&data->pump_lock);
}

int event_pump_register_handler(event_pump_t *pump, va_list args)
{
    event_pump_data_t *data = (event_pump_data_t *)pump->priv;

    int event_id = va_arg(args, int);
    void (*on_event)(event_pump_t*, int, void*, void*);
    on_event = va_arg(args, void (*)(event_pump_t*, int, void*, void*));
    void* context = va_arg(args, void*);

    LOG_DEBUG(EventPump, "Registered handler for event id: %d", event_id);
    variant_t *event_handler_variant = variant_hash_get(data->event_handlers, event_id);
    if (NULL == event_handler_variant)
    {
        variant_stack_t* handlers = stack_create();
        event_handler_variant = variant_create_ptr(DT_PTR, handlers, (void(*)(void*))stack_free);
        variant_hash_insert(data->event_handlers, event_id, event_handler_variant);
    }

    variant_stack_t *event_handlers = VARIANT_GET_PTR(variant_stack_t, event_handler_variant);
    event_handler_cb_t *event_handler_cb = (event_handler_cb_t *)malloc(sizeof(event_handler_cb_t));
    event_handler_cb->on_event = on_event;
    event_handler_cb->context = context;
    event_handler_cb->remove_pending_flag = false;

    stack_push_back(event_handlers, variant_create_ptr(DT_PTR, event_handler_cb, variant_delete_default));
    return event_id;
}

void event_pump_unregister_handler(event_pump_t *pump, va_list args)
{
    event_pump_data_t *data = (event_pump_data_t *)pump->priv;

    int event_id = va_arg(args, int);
    void (*on_event)(event_pump_t*, int, void*, void*);
    on_event = va_arg(args, void (*)(event_pump_t*, int, void*, void*));
    void* context = va_arg(args, void*);

    variant_t *event_handler_variant = variant_hash_get(data->event_handlers, event_id);
    if (NULL != event_handler_variant)
    {
        variant_stack_t *event_handlers = VARIANT_GET_PTR(variant_stack_t, event_handler_variant);
        stack_for_each(event_handlers, event_handler_context)
        {
            event_handler_cb_t *event_handler_cb = VARIANT_GET_PTR(event_handler_cb_t, event_handler_context);
            if (event_handler_cb->on_event == on_event && event_handler_cb->context == context)
            {
                LOG_DEBUG(EventPump, "Unregistered handler for event id: %d", event_id);
                event_handler_cb->remove_pending_flag = true;
                break;
            }
        }
    }
}

void event_pump_purge_handlers(event_pump_t *pump, int event_id)
{
    event_pump_data_t *data = (event_pump_data_t *)pump->priv;

    variant_t *event_handler_variant = variant_hash_get(data->event_handlers, event_id);
    variant_t* values_to_remove[100] = {0};
    int remove_index = 0;


    if (NULL != event_handler_variant)
    {
        variant_stack_t *event_handlers = VARIANT_GET_PTR(variant_stack_t, event_handler_variant);
        stack_for_each(event_handlers, event_handler_context)
        {
            event_handler_cb_t *event_handler_cb = VARIANT_GET_PTR(event_handler_cb_t, event_handler_context);
            if (event_handler_cb->remove_pending_flag && remove_index < 100)
            {
                values_to_remove[remove_index++] = event_handler_context;
            }
        }

        for(int i = 0; i < remove_index; i++)
        {
            stack_remove(event_handlers, values_to_remove[i]);
        }
    }
}

void event_pump_poll(event_pump_t *pump, struct timespec *ts)
{
    event_pump_data_t *data = (event_pump_data_t *)pump->priv;

    if(0 != data->current_event_queue_ptr->next_slot) 
    {
        LOG_DEBUG(EventPump, "Event Pump processing %d events", data->current_event_queue_ptr->next_slot);
        for (int i = 0; i < data->current_event_queue_ptr->next_slot; i++)
        {
            pending_event_t *e = &data->current_event_queue_ptr->queue[i];

            if(!e->processing)
            {
                e->processing = true;
                variant_t *event_handler_variant = variant_hash_get(data->event_handlers, e->event_id);

                if (NULL != event_handler_variant)
                {
                    variant_stack_t *event_handlers = VARIANT_GET_PTR(variant_stack_t, event_handler_variant);
                    stack_for_each(event_handlers, event_handler_variant)
                    {
                        event_handler_cb_t *event_handler_cb = VARIANT_GET_PTR(event_handler_cb_t, event_handler_variant);
                        if(!event_handler_cb->remove_pending_flag)
                        {
                            event_handler_cb->on_event(pump, e->event_id, e->event_data, event_handler_cb->context);
                        }
                    }

                    if(NULL != e->event_free)
                    {
                        LOG_DEBUG(EventPump, "Calling registered free callback for event");
                        e->event_free(e->event_data);
                    }
                    
                    event_pump_purge_handlers(pump, e->event_id);
                }
            }
        }

        data->current_event_queue_ptr->next_slot = 0;
    }

    // Switch current with pending:
    pthread_mutex_lock(&data->pump_lock);
    
    event_queue_t* tmp_ptr = data->current_event_queue_ptr;
    data->current_event_queue_ptr = data->pending_event_queue_ptr;
    data->pending_event_queue_ptr = tmp_ptr;

    pthread_mutex_unlock(&data->pump_lock);
}
