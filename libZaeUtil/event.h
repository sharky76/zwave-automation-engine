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

typedef enum
{
    SceneActivationEvent,
    CommandActivationEvent,
    EventLogEvent,
    VdevDataChangeEvent,
    SensorDataChangeEvent,
    DeviceAddedEvent,
    CommandDataReadyEvent
} e_events;

