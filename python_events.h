#pragma once

#include <python3.7m/Python.h>
#include <stack.h>

PyObject* PyInit_events(void);
void python_events_init();
const variant_stack_t* python_events_modules_for_device_event();
const variant_stack_t* python_events_modules_for_data_event(int device_id);