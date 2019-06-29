#include "event_pump.h"
#include "stack.h"
#include "hash.h"
#include <assert.h>

/* 
    Each event consist of ID (name) and data
    A client can submit events to the pump by calling event_pump_send_event(event_id, data)
    This event is stored in thread safe queue and a registered handler will be called upon the 
    next call to poll() by a dispatcher

    
 */
typedef struct event_st
{
    int event_id;
    void* event_data;
} event_t;

typedef struct event_handler_cb_st
{
    void(*on_event)(event_t*);
} event_handler_cb_t;

typedef struct event_handler_st
{
    int event_id;
    variant_stack_t* handlers; // List of event_handler_cb_t
} event_handler_t;

void variant_delete_event_handler(void* arg)
{
    event_handler_t* e = (event_handler_t*)arg;
    variant_free(e->handlers);
    free(e);
}

typedef struct event_pump_data_t
{
    variant_stack_t* event_queue;
    hash_table_t*    event_handlers;

} event_pump_data;

void event_pump_init(event_pump_t* pump)
{
    pump->name = strdup("EVENT_PUMP");
    pump->poll = &event_pump_poll;
    pump->register_handler = &event_pump_register_handler;
    pump->unregister_handler = &event_pump_unregister_handler;
    pump->start = NULL;
    pump->stop = NULL;
    pump->priv = (event_pump_data*)malloc(sizeof(event_pump_data));
    event_pump_data* data = (event_pump_data*)pump->priv;

    data->event_queue = stack_create();
    data->event_handlers = variant_hash_init();
}

void event_pump_send_event(event_pump_t* pump, int event_id, void* event_data)
{
    event_pump_data* data = (event_pump_data*)pump->priv;

    stack_lock(data->event_queue);
    event_t* new_event = (event_t*)malloc(sizeof(event_t));
    new_event->event_id = event_id;
    new_event->event_data = event_data;
    stack_push_back(data->event_queue, variant_create_ptr(DT_PTR, new_event, variant_delete_default));
    stack_unlock(data->event_queue);
}

void event_pump_register_handler(event_pump_t* pump, ...)
{
    event_pump_data* data = (event_pump_data*)pump->priv;

    va_list args;
    va_start(args, pump);
    int event_id = va_arg(args, int);
    void(*on_event)(event_t*);
    on_event = va_arg(args, void(*)(event_t*));

    variant_t* event_handler_variant = variant_hash_get(data->event_handlers, event_id);
    if(NULL == event_handler_variant)
    {
        event_handler_t* event_handlers = (event_handler_t*)malloc(sizeof(event_handler_t));
        event_handlers->event_id = event_id;
        event_handlers->handlers = stack_create();
        event_handler_variant = variant_create_ptr(DT_PTR, event_handlers, variant_delete_event_handler);
        variant_hash_insert(data->event_handlers, event_id, event_handler_variant);
    }

    event_handler_t* event_handlers = VARIANT_GET_PTR(event_handler_t, event_handler_variant);
    event_handler_cb_t* event_handler_cb = (event_handler_cb_t*)malloc(sizeof(event_handler_cb_t));
    event_handler_cb->on_event = on_event;

    stack_push_back(event_handlers->handlers, variant_create_ptr(DT_PTR, event_handler_cb, variant_delete_default));
}

void event_pump_unregister_handler(event_pump_t* pump, ...)
{
    event_pump_data* data = (event_pump_data*)pump->priv;

    va_list args;
    va_start(args, pump);
    int event_id = va_arg(args, int);
    void(*on_event)(event_t*);
    on_event = va_arg(args, void(*)(event_t*));

    variant_t* event_handler_variant = variant_hash_get(data->event_handlers, event_id);
    if(NULL != event_handler_variant)
    {
        event_handler_t* event_handlers = VARIANT_GET_PTR(event_handler_t, event_handler_variant);
        stack_for_each(event_handlers->handlers, event_handler_variant)
        {
            event_handler_cb_t* event_handler_cb = VARIANT_GET_PTR(event_handler_cb_t, event_handler_variant);
            if(event_handler_cb->on_event == on_event)
            {
                stack_remove(event_handlers->handlers, event_handler_variant);
                break;
            }
        }
    }
}

void event_pump_poll(event_pump_t* pump, struct timespec* ts)
{
    event_pump_data* data = (event_pump_data*)pump->priv;

    stack_lock(data->event_queue);
    stack_for_each(data->event_queue, event_variant)
    {
        event_t* e = VARIANT_GET_PTR(event_t, event_variant);
        variant_t* event_handler_variant = variant_hash_get(data->event_handlers, e->event_id);
        assert(NULL != event_handler_variant);

        if(NULL != event_handler_variant)
        {
            event_handler_t* event_handlers = VARIANT_GET_PTR(event_handler_t, event_handler_variant);
            stack_for_each(event_handlers->handlers, event_handler_variant)
            {
                event_handler_cb_t* event_handler_cb = VARIANT_GET_PTR(event_handler_cb_t, event_handler_variant);
                event_handler_cb->on_event(e);
            }
        }
    }
    stack_unlock(data->event_queue);
}