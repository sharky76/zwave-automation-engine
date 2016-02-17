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
    variant_t*  source;
} event_t;

event_t*    event_create(int source_id, variant_t* source);
void        event_delete(event_t* event);
void        event_post(event_t* event);
event_t*    event_receive();
