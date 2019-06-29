#include "event_dispatcher.h"
#include "socket_pump.h"
#include "timer_pump.h"
#include "event_pump.h"
#include <time.h>

#define MAX_PUMPS 10

event_pump_t pumps[MAX_PUMPS] = {0};
bool event_dispatcher_running = false;

void event_dispatcher_init()
{
    int pump_idx = 0;
    event_dispatcher_running = false;

    socket_pump_init(&pumps[pump_idx++]);
    timer_pump_init(&pumps[pump_idx++]);
    event_pump_init(&pumps[pump_idx++]);
}

void event_dispatcher_start()
{
    struct timespec ts;
    event_dispatcher_running = true;

    while(event_dispatcher_running)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for(int i = 0; i < MAX_PUMPS; i++)
        {
            if(NULL != pumps[i].poll)
            {
                pumps[i].poll(&pumps[i], &ts);
            }
            else
            {
                break;
            }
        }
    }
}

void event_dispatcher_stop()
{
    event_dispatcher_running = false;
}

event_pump_t* event_dispatcher_get_pump(const char* name)
{
    for(int i = 0; i < MAX_PUMPS; i++)
    {
        if(strcmp(pumps[i].name, name) == 0)
        {
            return &pumps[i];
        }
    }

    return NULL;
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
