#include "timer_pump.h"
#include "variant.h"
#include "math.h"
#include "logger.h"
#include <stdbool.h>
#include <pthread.h>

USING_LOGGER(EventPump);

typedef struct timer_event_data_st
{
    int        timer_id;
    uint64_t   timeout_msec;
    int64_t   current_timeout;
    uint64_t last_poll_time;
    bool  is_started;
    bool  is_singleshot;
    void  (*on_event)(event_pump_t*,int,void*);
    void* context;
    bool  remove_pending_flag;
} timer_event_data_t;

typedef struct timer_pump_data_t
{
    //hash_table_t* registered_timer_table;
    timer_event_data_t timer_table[MAX_TIMERS];
    int next_timer_slot;
    int max_timer_id;
    pthread_mutex_t lock;
} timer_pump_data_t;

void timer_pump_init(event_pump_t* pump)
{
    LOG_ADVANCED(EventPump, "Initializing Timer Pump...");
    pump->name = strdup("TIMER_PUMP");
    pump->poll = &timer_pump_poll;
    pump->register_handler = &timer_pump_add_timeout;
    pump->unregister_handler = &timer_pump_remove_timeout;
    pump->start = &timer_pump_start_timer;
    pump->stop = &timer_pump_stop_timer;
    pump->free = &timer_pump_free;
    pump->priv = (timer_pump_data_t*)malloc(sizeof(timer_pump_data_t));
    memset(pump->priv, 0, sizeof(timer_pump_data_t));
    timer_pump_data_t* data = (timer_pump_data_t*)pump->priv;
    pthread_mutex_init(&data->lock, NULL);
}

void timer_pump_free(event_pump_t* pump)
{
    LOG_ADVANCED(EventPump, "Destroying Timer Pump...");
    free(pump->name);
    free(pump->priv);
}

int timer_pump_add_timeout(event_pump_t* pump, va_list args)
{
    int timer_id = -1;
    int timeout_msec = va_arg(args, int);
    bool is_singleshot = va_arg(args, int);
    void(*on_event)(event_pump_t*,int,void*);
    on_event = va_arg(args, void(*)(event_pump_t*,int,void*));
    void* context = va_arg(args, void*);

    timer_pump_data_t* data = (timer_pump_data_t*)pump->priv;
    pthread_mutex_lock(&data->lock);
    if(data->next_timer_slot < MAX_TIMERS)
    {
        timer_event_data_t* event_data = &data->timer_table[data->next_timer_slot];
        event_data->timer_id = data->max_timer_id++;
        event_data->timeout_msec = (uint64_t)timeout_msec * 1000000;
        event_data->on_event = on_event;
        event_data->last_poll_time = 0;
        event_data->is_started = false;
        event_data->is_singleshot = is_singleshot;
        event_data->context = context;
        data->next_timer_slot++;
        timer_id = event_data->timer_id;
    }
    pthread_mutex_unlock(&data->lock);

    return timer_id;
}

void timer_pump_remove_timeout(event_pump_t* pump, va_list args)
{
    int timer_id = va_arg(args, int);
    timer_pump_data_t* data = (timer_pump_data_t*)pump->priv;
    
    pthread_mutex_lock(&data->lock);
    for(int i = 0; i < MAX_TIMERS; i++)
    {
        timer_event_data_t* event_data = &data->timer_table[i];
        if(event_data->timer_id == timer_id)
        {
            LOG_DEBUG(EventPump, "Removed timer id: %d", timer_id);
            event_data->remove_pending_flag = true;
            break;
        }
    }
    pthread_mutex_unlock(&data->lock);
}

void timer_purge_timers(event_pump_t* pump, int index)
{
    timer_pump_data_t* data = (timer_pump_data_t*)pump->priv;
    pthread_mutex_lock(&data->lock);
    if(data->timer_table[index].remove_pending_flag)
    {
        memcpy(&data->timer_table[index], &data->timer_table[data->next_timer_slot-1], sizeof(timer_event_data_t));
        memset(&data->timer_table[data->next_timer_slot-1], 0, sizeof(timer_event_data_t));
        data->next_timer_slot--;
        LOG_DEBUG(EventPump, "Active timers now: %d", data->next_timer_slot);
    }
    pthread_mutex_unlock(&data->lock);
}

void timer_pump_poll(event_pump_t* pump, struct timespec* ts)
{
    timer_pump_data_t* data = (timer_pump_data_t*)pump->priv;
    uint64_t time_nano = (uint64_t)ts->tv_sec * 1000000000 + ts->tv_nsec;

    for(int i = 0; i < data->next_timer_slot; i++)
    {
        timer_event_data_t* event_data = (timer_event_data_t*)&data->timer_table[i];
        if(event_data->is_started && event_data->last_poll_time < time_nano)
        {   
            uint64_t delta = time_nano - event_data->last_poll_time;
            event_data->current_timeout -= delta;
            event_data->last_poll_time = time_nano;

            if(event_data->current_timeout <= 0)
            {
                if(!event_data->remove_pending_flag)
                {
                    LOG_DEBUG(EventPump, "Timer event: %d", event_data->timer_id);
                    event_data->on_event(pump, event_data->timer_id, event_data->context);
                    if(event_data->is_singleshot)
                    {
                        event_data->is_started = false;
                    }
                    else
                    {
                        event_data->current_timeout = event_data->timeout_msec;
                    }
                }

                timer_purge_timers(pump, i);
            }
        }
    }
}

void timer_pump_start_timer(event_pump_t* pump, ...)
{
    va_list args;
    va_start(args, pump);
    int handler_id = va_arg(args, int);
    va_end(args);

    timer_pump_data_t* data = (timer_pump_data_t*)pump->priv;
    pthread_mutex_lock(&data->lock);
    for(int i = 0; i < data->next_timer_slot; i++)
    {
        timer_event_data_t* timer_event_data = &data->timer_table[i];
        if(timer_event_data->timer_id == handler_id)
        {
            timer_event_data->is_started = true;
            
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            timer_event_data->last_poll_time = (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
            timer_event_data->current_timeout = timer_event_data->timeout_msec;

            LOG_ADVANCED(EventPump, "Started timer: %d", timer_event_data->timer_id);
            break;
        }
    }
    pthread_mutex_unlock(&data->lock);
}

void timer_pump_stop_timer(event_pump_t* pump, ...)
{
    va_list args;
    va_start(args, pump);
    int handler_id = va_arg(args, int);
    va_end(args);

    timer_pump_data_t* data = (timer_pump_data_t*)pump->priv;

    pthread_mutex_lock(&data->lock);
    for(int i = 0; i < data->next_timer_slot; i++)
    {
        timer_event_data_t* timer_event_data = &data->timer_table[i];
        if(timer_event_data->timer_id == handler_id)
        {
            timer_event_data->is_started = false;
            LOG_ADVANCED(EventPump, "Stopped timer: %d", timer_event_data->timer_id);
            break;
        }
    }
    pthread_mutex_unlock(&data->lock);
}

bool timer_pump_is_singleshot(event_pump_t* pump, int timer_id)
{
    bool is_singleshot = false;
    timer_pump_data_t* data = (timer_pump_data_t*)pump->priv;

    pthread_mutex_lock(&data->lock);
    for(int i = 0; i < data->next_timer_slot; i++)
    {
        timer_event_data_t* timer_event_data = &data->timer_table[i];
        if(timer_event_data->timer_id == timer_id)
        {
            is_singleshot = timer_event_data->is_singleshot;
            break;
        }
    }
    pthread_mutex_unlock(&data->lock);

    return is_singleshot;
}
