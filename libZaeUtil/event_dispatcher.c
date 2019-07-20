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
    dispatcher->event_dispatcher_joined = false;

    socket_pump_init(&dispatcher->pumps[pump_idx++]);
    timer_pump_init(&dispatcher->pumps[pump_idx++]);
    event_pump_init(&dispatcher->pumps[pump_idx++]);
}

void _event_dispatcher_free(event_dispatcher_t* dispatcher)
{
    LOG_INFO(EventPump, "Destroying Event Dispatcher...");
    dispatcher->event_dispatcher_running = false;

    int pump_idx = 0;
    for(int i = 0; i < MAX_PUMPS; i++)
    {
        if(NULL != dispatcher->pumps[i].free)
        {
            dispatcher->pumps[i].free(&dispatcher->pumps[i]);
        }
    }

    free(dispatcher);
}

void _event_dispatcher_run(event_dispatcher_t* dispatcher, struct timespec* ts)
{
    for(int i = 0; i < MAX_PUMPS; i++)
    {
        if(NULL != dispatcher->pumps[i].poll)
        {
            dispatcher->pumps[i].poll(&dispatcher->pumps[i], ts);
        }
        else
        {
            break;
        }
    }
}

void _event_dispatcher_start(event_dispatcher_t* dispatcher)
{
    LOG_INFO(EventPump, "Event Dispatcher started");
    struct timespec ts;
    dispatcher->event_dispatcher_running = true;

    while(dispatcher->event_dispatcher_running)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        _event_dispatcher_run(dispatcher, &ts);
    }

    LOG_INFO(EventPump, "Event Dispatcher stopped");
}

void _event_dispatcher_wait_handler(event_pump_t* pump, int timer_id, void* context)
{
    bool* timer_fired = (bool*)context;
    *timer_fired = true;
}

void _event_dispatcher_attach(event_dispatcher_t* dispatcher, int timeout_msec)
{
    event_pump_t* pump = event_dispatcher_get_pump("TIMER_PUMP");
    bool timer_fired = false;
    dispatcher->event_dispatcher_joined = true;
    int timer_id = event_dispatcher_register_handler(pump, timeout_msec, true, _event_dispatcher_wait_handler, &timer_fired);
    pump->start(pump, timer_id);
    struct timespec ts;
    while(!timer_fired && dispatcher->event_dispatcher_joined) 
    {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        _event_dispatcher_run(dispatcher, &ts);
    }

    dispatcher->event_dispatcher_joined = false;
    pump->stop(pump, timer_id);
    event_dispatcher_unregister_handler(pump, timer_id);
}

void _event_dispatcher_detach(event_dispatcher_t* dispatcher)
{
    dispatcher->event_dispatcher_joined = false;
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
        _event_dispatcher_run(dispatcher, &ts);

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

void event_dispatcher_attach(int timeout_msec)
{
    _event_dispatcher_attach(&default_dispatcher, timeout_msec);
}

void event_dispatcher_detach()
{
    _event_dispatcher_detach(&default_dispatcher);
}

void event_dispatcher_stop()
{
    _event_dispatcher_stop(&default_dispatcher);
}

event_pump_t* event_dispatcher_get_pump(const char* name)
{
    _event_dispatcher_get_pump(&default_dispatcher, name);
}

int event_dispatcher_register_handler(event_pump_t* pump, ...)
{
    va_list args;
    va_start(args, pump);
    int id = pump->register_handler(pump, args);
    va_end(args);
    return id;
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

    dispatcher->start = &_event_dispatcher_start;
    dispatcher->timed_start = &_event_dispatcher_timed_start;
    dispatcher->stop = &_event_dispatcher_stop;
    dispatcher->get_pump = &_event_dispatcher_get_pump;
    dispatcher->free = &_event_dispatcher_free;
    dispatcher->attach = &_event_dispatcher_attach;
    dispatcher->detach = &_event_dispatcher_detach;

    return dispatcher;
}

typedef struct event_wait_context_t
{
    void* event_data;
    bool is_found;
} event_wait_context_t;

void event_id_wait_handler(event_pump_t* pump, int event_id, void* event_data, void* ctx)
{
    event_wait_context_t* context = (event_wait_context_t*)ctx;
    context->is_found = true;
    context->event_data = event_data;
    event_dispatcher_detach();
}

bool event_dispatcher_wait(int event_id, int timeout_msec)
{
    event_wait_context_t ctx = {.is_found = false};
    event_dispatcher_register_handler(event_dispatcher_get_pump("EVENT_PUMP"), event_id, &event_id_wait_handler, (void*)&ctx);
    event_dispatcher_attach(timeout_msec);
    event_dispatcher_unregister_handler(event_dispatcher_get_pump("EVENT_PUMP"), event_id, &event_id_wait_handler, (void*)&ctx);

    return ctx.is_found;
}