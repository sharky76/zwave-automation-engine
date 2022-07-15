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

typedef struct event_entry_t
{
    int event_id; // autoincrement
    int node_id;
    int instance_id;
    int command_id;
    int device_type;
    char* event_data; 
    time_t   timestamp;
} event_entry_t;

typedef enum
{
    SceneActivationEvent,
    CommandActivationEvent,
    //EventLogEvent,
    VdevDataChangeEvent,
    SensorDataChangeEvent,
    DeviceAddedEvent,
    CommandDataReadyEvent,
    EventLogNewEvent
} e_events;

event_entry_t* new_event_entry();
void event_send(event_entry_t* event, void* context);