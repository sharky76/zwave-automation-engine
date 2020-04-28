#include "Button_config.h"

void delete_button_entry(void* arg)
{
    button_entry_t* entry = (button_entry_t*)arg;
    free(entry->name);
    free(entry);
}

button_entry_t*    button_add(device_record_t* record)
{
    button_entry_t* entry = malloc(sizeof(button_entry_t));
    entry->name = strdup(record->deviceName);
    entry->instance = record->instanceId;
    entry->state = OFF;
    
    stack_push_back(button_list, variant_create_ptr(DT_PTR, entry, &delete_button_entry));
    return entry;
}

button_entry_t*    button_find_instance(int instance)
{
    stack_for_each(button_list, button_variant)
    {
        button_entry_t* entry = VARIANT_GET_PTR(button_entry_t, button_variant);
        if(entry->instance == instance)
        {
            return entry;
        }
    }

    return NULL;
}
