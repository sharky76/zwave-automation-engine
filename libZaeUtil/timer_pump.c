#include "timer_pump.h"
#include "variant.h"
#include "math.h"

typedef struct timer_pump_data_t
{
    hash_table_t* registered_timer_table;
} timer_pump_data;


typedef struct timer_event_data_t
{
    int   handler_id;
    uint64_t   timeout_msec;
    uint64_t   current_timeout;
    uint64_t last_poll_time;
    bool  is_started;
    bool  is_singleshot;
    void  (*on_event)(void);
} timer_event_data;


void timer_pump_init(event_pump_t* pump)
{
    pump->name = strdup("TIMER_PUMP");
    pump->poll = &timer_pump_poll;
    pump->register_handler = &timer_pump_add_timeout;
    pump->unregister_handler = &timer_pump_remove_timeout;
    pump->start = &timer_pump_start_timer;
    pump->stop = &timer_pump_stop_timer;
    pump->priv = (timer_pump_data*)malloc(sizeof(timer_pump_data));
    timer_pump_data* data = (timer_pump_data*)pump->priv;

    data->registered_timer_table = variant_hash_init();
}

void timer_pump_add_timeout(event_pump_t* pump, ...)
{
    va_list args;
    va_start(args, pump);
    int handler_id = va_arg(args, int);
    int timeout_msec = va_arg(args, int);
    bool is_singleshot = va_arg(args, bool);
    void(*on_event)(void);
    on_event = va_arg(args, void(*)(void));
    va_end(args);

    timer_pump_data* data = (timer_pump_data*)pump->priv;

    timer_event_data* event_data = (timer_event_data*)malloc(sizeof(timer_event_data));
    event_data->handler_id = handler_id;
    event_data->timeout_msec = event_data->current_timeout = timeout_msec * 1000000;
    event_data->on_event = on_event;
    event_data->last_poll_time = 0;
    event_data->is_started = false;
    event_data->is_singleshot = is_singleshot;

    variant_hash_insert(data->registered_timer_table, handler_id, variant_create_ptr(DT_PTR, event_data, variant_delete_default));
}

void timer_pump_remove_timeout(event_pump_t* pump, ...)
{
    va_list args;
    va_start(args, pump);
    int handler_id = va_arg(args, int);
    va_end(args);

    timer_pump_data* data = (timer_pump_data*)pump->priv;
    variant_hash_remove(data->registered_timer_table, handler_id);
}

void on_timer_event_visitor(timer_event_data* event_data, void* arg)
{
    struct timespec* ts  = (struct timespec*)arg;
    uint64_t time_nano = ts->tv_sec * 1000000000 + ts->tv_nsec;

    if(event_data->is_started)
    {
        int delta = time_nano - event_data->last_poll_time;
        event_data->current_timeout -= delta;
        event_data->last_poll_time = time_nano;

        if(event_data->current_timeout <= 0)
        {
            event_data->on_event();
            if(event_data->is_singleshot)
            {
                event_data->is_started = false;
            }
            else
            {
                event_data->current_timeout = event_data->timeout_msec;
            }
        }
    }
}

void timer_pump_poll(event_pump_t* pump, struct timespec* ts)
{
    timer_pump_data* data = (timer_pump_data*)pump->priv;

    variant_hash_for_each_value(data->registered_timer_table, timer_event_data*, on_timer_event_visitor, (void*)ts);
}

void timer_pump_start_timer(event_pump_t* pump, ...)
{
    va_list args;
    va_start(args, pump);
    int handler_id = va_arg(args, int);
    va_end(args);

    timer_pump_data* data = (timer_pump_data*)pump->priv;

    variant_t* timer_data_variant = variant_hash_get(data->registered_timer_table, handler_id);
    timer_event_data* timer_event_data = VARIANT_GET_PTR(timer_event_data, timer_data_variant);
    timer_event_data->is_started = true;
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    timer_event_data->last_poll_time = ts.tv_sec * 1000000000 + ts.tv_nsec;

}
void timer_pump_stop_timer(event_pump_t* pump, ...)
{
    va_list args;
    va_start(args, pump);
    int handler_id = va_arg(args, int);
    va_end(args);

    timer_pump_data* data = (timer_pump_data*)pump->priv;

    variant_t* timer_data_variant = variant_hash_get(data->registered_timer_table, handler_id);
    timer_event_data* timer_event_data = VARIANT_GET_PTR(timer_event_data, timer_data_variant);
    timer_event_data->is_started = false;
}

bool timer_pump_is_timer_started(event_pump_t* pump, int handler_id)
{
    timer_pump_data* data = (timer_pump_data*)pump->priv;

    variant_t* timer_data_variant = variant_hash_get(data->registered_timer_table, handler_id);
    timer_event_data* timer_event_data = VARIANT_GET_PTR(timer_event_data, timer_data_variant);
    return timer_event_data->is_started;
}