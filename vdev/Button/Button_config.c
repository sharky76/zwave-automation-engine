#include "Button_config.h"
#include <resolver.h>
#include <logger.h>

void delete_button_entry(void* arg)
{
    button_entry_t* entry = (button_entry_t*)arg;
    free(entry->name);
    free(entry);
}

button_entry_t*    button_add(int instance, const char* name)
{
    button_entry_t* entry = malloc(sizeof(button_entry_t));
    entry->name = strdup(name);
    entry->instance = instance;
    entry->state = OFF;
    
    stack_push_back(button_list, variant_create_ptr(DT_PTR, entry, &delete_button_entry));
    char name_buf[32] = {0};
    snprintf(name_buf, 31, "%s.%d.%d", entry->name, entry->instance, 0x25);
    resolver_add_entry(VDEV, name_buf, DT_BUTTON, entry->instance, 0x25);

    memset(name_buf, 0, sizeof(name_buf));
    snprintf(name_buf, 31, "%s.%d.%d", entry->name, entry->instance, 0x72);
    resolver_add_entry(VDEV, name_buf, DT_BUTTON, entry->instance, 0x72);
    LOG_ADVANCED(DT_BUTTON, "Added button %s", entry->name);

    return entry;
}

bool  button_del(int instance)
{
    variant_t* victim = NULL;

    stack_for_each(button_list, button_variant)
    {
        button_entry_t* entry = VARIANT_GET_PTR(button_entry_t, button_variant);
        if(entry->instance == instance)
        {
            victim = button_variant;
            char name_buf[32] = {0};
            snprintf(name_buf, 31, "%s.%d.%d", entry->name, entry->instance, 0x25);
            resolver_remove_entry(name_buf);
            memset(name_buf, 0, sizeof(name_buf));
            snprintf(name_buf, 31, "%s.%d.%d", entry->name, entry->instance, 0x72);
            resolver_remove_entry(name_buf);
            LOG_ADVANCED(DT_BUTTON, "Removed button %s", entry->name);
            break;
        }
    }

    if(NULL != victim)
    {
        stack_remove(button_list, victim);
        variant_free(victim);
    }

    return true;
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
