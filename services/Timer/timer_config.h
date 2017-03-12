#pragma once
#include <stdbool.h>
#include <service.h>

typedef enum timer_event_type_t
{
    SCENE,
    COMMAND
} timer_event_type_t;

typedef struct timer_info_t
{
    char*       name;
    char*       event_name;
    bool        singleshot;
    timer_event_type_t event_type;
    uint32_t    timeout;
    uint32_t    ticks_left;
} timer_info_t;

extern bool timer_enabled;
extern variant_stack_t* timer_list;
extern int DT_TIMER;
//service_t* self;

void timer_delete_timer(void* arg);
