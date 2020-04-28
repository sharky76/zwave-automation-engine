#pragma once
#include <stack.h>
#include <resolver.h>

int  DT_BUTTON;

typedef enum ButtonState
{
    OFF, ON
} ButtonState;

typedef struct button_entry_t
{
    char* name;
    int instance;
    ButtonState state;
} button_entry_t;

variant_stack_t* button_list;

button_entry_t*    button_add(device_record_t* record);
button_entry_t*    button_find_instance(int instance);
