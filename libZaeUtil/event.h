#pragma once

/*
Event bus for posting events to engine 
Each event triggers a particular scene 
*/
#include "stack.h"
#include "variant.h"
#include <stdint.h>

typedef struct event_t
{
    int     source_id;
    char*   name;
    variant_t*  data;
} event_t;

typedef struct event_type_data_t
{
    int id;
    char* event_name;
} event_type_data_t;

typedef struct event_handler_t
{
    int id;
    char* event_name;
    void (*event_handler)(event_t*,void*);
    void* context;
} event_handler_t;

#define SCENE_ACTIVATION_EVENT   "SceneActivationEvent"
#define COMMAND_ACTIVATION_EVENT "CommandActivationEvent"
#define TIMER_TICK_EVENT         "TimerTickEvent"

void        event_manager_init();
void        event_manager_shutdown();
void        event_manager_join();

// Each module who wish to have events forwarded to it must register its handler method
// Only events registered with the handler will be forwarded to it
// Multiple handlers can registed to handle same event
void        event_register_handler(int handler_id, const char* event_name, void (*event_handler)(event_t*, void*), void* context);
void        event_unregister_handler(int handler_id, const char* event_name);

// Listen to file descriptor and call event_handler if data is available
void        event_register_fd(int fd, void (*event_handler)(int, void*), void* context);
void        event_unregister_fd(int fd);

event_t*    event_create(int source_id, const char* event_name, variant_t* data);
void        event_delete(event_t* event);
void        event_post(event_t* event);
int         event_wait(int source_id, const char* event_name, uint32_t timeout);

