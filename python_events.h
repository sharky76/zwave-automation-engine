#pragma once

#include <python3.7m/Python.h>
#include <stack.h>

typedef struct timer_event_entry_t
{
    int module_id;
    int timeout_msec;
    char* callback;
    int timer_id;
} timer_event_entry_t;

PyObject* PyInit_events(void);
void python_events_init();
const variant_stack_t* python_events_modules_for_device_event();
variant_stack_t* python_events_modules_for_timer_event();
const variant_stack_t* python_events_modules_for_data_event(int device_id);