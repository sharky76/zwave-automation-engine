#pragma once

#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Event pumps either push events into event manager
// or can be polled for events
// Eg: Socket event pump when polled executes "poll" call on socket descriptor and return socket descriptor which is ready for read or write
//     Timer event pump when polled increment internal counter and return event if counter reached pre-set value

typedef struct event_pump_t
{
    char* name;
    void (*poll)(struct event_pump_t*, struct timespec* ts);
    int (*register_handler)(struct event_pump_t*, va_list args);
    void (*unregister_handler)(struct event_pump_t*, va_list args);
    void (*start)(struct event_pump_t*, ...);
    void (*stop)(struct event_pump_t*, ...);
    void (*free)(struct event_pump_t*);
    void* priv;
} event_pump_t;

typedef struct event_dispatcher_t event_dispatcher_t;

#define MAX_PUMPS 10

struct event_dispatcher_t
{
    event_pump_t pumps[MAX_PUMPS];
    bool event_dispatcher_running;
    bool event_dispatcher_joined;

    void (*start)(event_dispatcher_t*);
    void (*timed_start)(event_dispatcher_t*, int);
    void (*stop)(event_dispatcher_t*);
    void (*free)(event_dispatcher_t*);
    void (*attach)(event_dispatcher_t*, int);
    void (*detach)(event_dispatcher_t*);
    event_pump_t* (*get_pump)(event_dispatcher_t*, const char*);
};

void event_dispatcher_init();
void event_dispatcher_start();
void event_dispatcher_attach(int timeout_msec);
void event_dispatcher_detach();
void event_dispatcher_timed_start(int timeout_msec);
void event_dispatcher_stop();

int event_dispatcher_register_handler(event_pump_t*, ...);
void event_dispatcher_unregister_handler(event_pump_t*, ...);

event_pump_t* event_dispatcher_get_pump(const char* name);

event_dispatcher_t* event_dispatcher_new();
void event_dispatcher_free(event_dispatcher_t* dispatcher);
bool event_dispatcher_wait(int event_id, int timeout_msec, void** event_data);