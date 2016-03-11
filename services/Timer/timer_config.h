#pragma once
#include <stdbool.h>
#include <service.h>

typedef struct timer_info_t
{
    char*       name;
    bool        singleshot;
    uint32_t    timeout;
    uint32_t    ticks_left;
} timer_info_t;

extern bool timer_enabled;
extern variant_stack_t* timer_list;
extern int DT_TIMER;
//service_t* self;

void timer_delete(void* arg);
