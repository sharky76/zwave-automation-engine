#pragma once

/*
Event bus for posting events to engine 
Each event triggers a particular scene 
*/
#include "stack.h"
#include "variant.h"

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
    char* name;
    void (*event_handler)(event_t*);
} event_handler_t;

typedef struct sensor_event_data_t
{
    uint8_t  node_id;
    uint8_t  instance_id;
    uint8_t  command_id;
    const char*   device_name;
    unsigned long  last_update_time;
} sensor_event_data_t;

#define SCENE_ACTIVATION_EVENT   "SceneActivationEvent"
#define COMMAND_ACTIVATION_EVENT "CommandActivationEvent"
#define TIMER_TICK_EVENT         "TimerTickEvent"

void        event_manager_init();

// Each module who wish to have events forwarded to it must register its handler method
// Only events registered with the handler will be forwarded to it
// Multiple handlers can registed to handle same event
void        event_register_handler(int handler_id, const char* name, void (*event_handler)(event_t*));

event_t*    event_create(int source_id, const char* name, variant_t* data);
void        event_delete(event_t* event);
void        event_post(event_t* event);
event_t*    event_receive();

