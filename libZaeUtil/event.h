#pragma once

/*
Event bus for posting events to engine 
Each event triggers a particular scene 
*/
#include "stack.h"
#include "variant.h"

typedef struct event_t
{
    int source_id;
    variant_t*  data;
} event_t;

typedef struct sensor_event_data_t
{
    uint8_t  node_id;
    uint8_t  instance_id;
    uint8_t  command_id;
    const char*   device_name;
    unsigned long  last_update_time;
} sensor_event_data_t;

typedef struct service_event_data_t
{
    char*   data;
} service_event_data_t;

void        event_manager_init();
void        event_register_handler(void (*event_handler)(event_t*));
event_t*    event_create(int source_id, variant_t* data);
void        event_delete(event_t* event);
void        event_post(event_t* event);
event_t*    event_receive();
