#include "data_cache.h"
#include <stack.h>
#include <stdlib.h>
#include <logger.h>
#include "variant_types.h"

static variant_stack_t* data_cache_storage = NULL;

void data_entry_delete(void* arg)
{
    data_entry_t* entry = (data_entry_t*)arg;
    free(entry->key);
    free(entry);
}

void    data_cache_init()
{
    if(NULL == data_cache_storage)
    {
        data_cache_storage = stack_create();
    }
}

void        data_cache_clear()
{
    stack_free(data_cache_storage);
    data_cache_storage = NULL;
    data_cache_init();
}

void        data_cache_add_entry(data_entry_key_t* new_key, ZDataHolder new_data)
{
    LOG_DEBUG("Request adding new data holder to cache for node id: %d and command 0x%x", new_key->node_id, new_key->command_id);
    if(NULL == data_cache_get_data(new_key))
    {
        data_entry_t* new_entry = malloc(sizeof(data_entry_t));
        new_entry->key = new_key;
        new_entry->data = new_data;
    
        stack_push_back(data_cache_storage, variant_create_ptr(DT_DATA_ENTRY_TYPE, new_entry, &data_entry_delete));
        LOG_ADVANCED("New data holder added to cache for node id %d and command 0x%x", new_key->node_id, new_key->command_id);
    }
    else
    {
        LOG_DEBUG("Data holder already exists");
    }
}

void        data_cache_del_entry(data_entry_key_t* key)
{
    stack_for_each(data_cache_storage, data_entry_variant)
    {
        data_entry_t* entry = variant_get_ptr(data_entry_variant);
        
        if(memcmp(entry->key, key, sizeof(data_entry_key_t)) == 0)
        {
            stack_remove(data_cache_storage, data_entry_variant);
            variant_free(data_entry_variant);
            return;
        }
    }
}

ZDataHolder data_cache_get_data(data_entry_key_t* key)
{
    stack_for_each(data_cache_storage, data_entry_variant)
    {
        data_entry_t* entry = variant_get_ptr(data_entry_variant);
        
        if(memcmp(entry->key, key, sizeof(data_entry_key_t)) == 0)
        {
            return entry->data;
        }
    }

    return NULL;
}

bool    data_cache_get_nodes(ZWBYTE* node_ids, int* count)
{
    int nodes_size = *count;
    ZWBYTE* node_start = node_ids;
    *count = 0;

    stack_for_each(data_cache_storage, data_entry_variant)
    {
        data_entry_t* entry = variant_get_ptr(data_entry_variant);
        bool nodeExists = false;
        for(int i = 0; i < *count && !nodeExists; i++)
        {
            nodeExists = (node_start[i] == entry->key->node_id);
        }

        if(!nodeExists)
        {
            *node_ids = entry->key->node_id;
            node_ids++;
            (*count)++;
    
            if(*count == nodes_size)
            {
                return false;
            }
        }
    }

    return true;
}

bool    data_cache_get_instances(ZWBYTE node_id, ZWBYTE* instances, int* count)
{
    int instances_size = *count;
    ZWBYTE* inst_start = instances;
    *count = 0;
    stack_for_each(data_cache_storage, data_entry_variant)
    {
        data_entry_t* entry = variant_get_ptr(data_entry_variant);
        if(entry->key->node_id == node_id)
        {
            bool nodeExists = false;
            for(int i = 0; i < *count && !nodeExists; i++)
            {
                nodeExists = (inst_start[i] == entry->key->instance_id);
            }

            if(!nodeExists)
            {
                *instances = entry->key->instance_id;
                instances++;
                (*count)++;
    
                if(*count == instances_size)
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool    data_cache_get_commands(ZWBYTE node_id, ZWBYTE instance_id, ZWBYTE* commands, int* count)
{
    int commands_size = *count;
    ZWBYTE* cmd_start = commands;
    *count = 0;
    stack_for_each(data_cache_storage, data_entry_variant)
    {
        data_entry_t* entry = variant_get_ptr(data_entry_variant);
        if(entry->key->instance_id == instance_id &&
           entry->key->node_id == node_id)
        {
            bool nodeExists = false;
            for(int i = 0; i < *count && !nodeExists; i++)
            {
                nodeExists = (cmd_start[i] == entry->key->command_id);
            }

            if(!nodeExists)
            {
                *commands = entry->key->command_id;
                commands++;
                (*count)++;
    
                if(*count == commands_size)
                {
                    return false;
                }
            }
        }
    }

    return true;
}

