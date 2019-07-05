#include "event_dispatcher.h"
#include <time.h>
#include "logger.h"
#include "socket_pump.h"
#include "timer_pump.h"
#include "event_pump.h"
#include <sys/eventfd.h>

DECLARE_LOGGER(EventPump);

event_dispatcher_t default_dispatcher = {0};

void _event_dispatcher_init(event_dispatcher_t* dispatcher)
{
    LOG_INFO(EventPump, "Initializing Event Dispatcher...");
    memset(dispatcher, 0, sizeof(event_dispatcher_t));

    int pump_idx = 0;
    dispatcher->event_dispatcher_running = false;

    socket_pump_init(&dispatcher->pumps[pump_idx++]);
    timer_pump_init(&dispatcher->pumps[pump_idx++]);
    event_pump_init(&dispatcher->pumps[pump_idx++]);
}

void _event_dispatcher_start(event_dispatcher_t* dispatcher)
{
    LOG_INFO(EventPump, "Event Dispatcher started");
    struct timespec ts;
    dispatcher->event_dispatcher_running = true;

    while(dispatcher->event_dispatcher_running)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for(int i = 0; i < MAX_PUMPS; i++)
        {
            if(NULL != dispatcher->pumps[i].poll)
            {
                dispatcher->pumps[i].poll(&dispatcher->pumps[i], &ts);
            }
            else
            {
                break;
            }
        }
    }

    LOG_INFO(EventPump, "Event Dispatcher stopped");
}

void _event_dispatcher_timed_start(event_dispatcher_t* dispatcher, int timeout_msec)
{
    LOG_INFO(EventPump, "Event Dispatcher started");
    struct timespec ts;
    dispatcher->event_dispatcher_running = true;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    uint64_t current_time;
    uint64_t start_time = (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
    
    while(dispatcher->event_dispatcher_running)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for(int i = 0; i < MAX_PUMPS; i++)
        {
            if(NULL != dispatcher->pumps[i].poll)
            {
                dispatcher->pumps[i].poll(&dispatcher->pumps[i], &ts);
            }
            else
            {
                break;
            }
        }

        current_time = (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
        dispatcher->event_dispatcher_running = ((start_time + (uint64_t)timeout_msec*1000000) > current_time);
    }

    LOG_INFO(EventPump, "Event Dispatcher stopped");
}

void _event_dispatcher_stop(event_dispatcher_t* dispatcher)
{
    dispatcher->event_dispatcher_running = false;
}

event_pump_t* _event_dispatcher_get_pump(event_dispatcher_t* dispatcher, const char* name)
{
    for(int i = 0; i < MAX_PUMPS; i++)
    {
        if(strcmp(dispatcher->pumps[i].name, name) == 0)
        {
            return &dispatcher->pumps[i];
        }
    }

    return NULL;
}

void event_dispatcher_init()
{
    _event_dispatcher_init(&default_dispatcher);
}

void event_dispatcher_start()
{
    _event_dispatcher_start(&default_dispatcher);
}

void event_dispatcher_timed_start(int timeout_msec)
{
    _event_dispatcher_timed_start(&default_dispatcher, timeout_msec);
}

void event_dispatcher_stop()
{
    _event_dispatcher_stop(&default_dispatcher);
}

event_pump_t* event_dispatcher_get_pump(const char* name)
{
    _event_dispatcher_get_pump(&default_dispatcher, name);
}

void event_dispatcher_register_handler(event_pump_t* pump, ...)
{
    va_list args;
    va_start(args, pump);
    pump->register_handler(pump, args);
    va_end(args);
}

void event_dispatcher_unregister_handler(event_pump_t* pump, ...)
{
    va_list args;
    va_start(args, pump);
    pump->unregister_handler(pump, args);
    va_end(args);
}

event_dispatcher_t* event_dispatcher_new()
{
    event_dispatcher_t* dispatcher = (event_dispatcher_t*)malloc(sizeof(event_dispatcher_t));
    _event_dispatcher_init(dispatcher);

    dispatcher->event_dispatcher_start = &_event_dispatcher_start;
    dispatcher->event_dispatcher_timed_start = &_event_dispatcher_timed_start;
    dispatcher->event_dispatcher_stop = &_event_dispatcher_stop;
    dispatcher->event_dispatcher_get_pump = &_event_dispatcher_get_pump;

    return dispatcher;
}