#include "hash.h"
#include "logger.h"
#include <stdlib.h>

void delete_node_data(void* arg)
{
    hash_node_data_t* node_data = (hash_node_data_t*)arg;
    variant_free(node_data->data);
    free(node_data);
}

variant_t*   create_node_data(uint32_t key, variant_t* data)
{
    hash_node_data_t* new_data = calloc(1, sizeof(hash_node_data_t));
    new_data->data = data;
    new_data->key = key;

    return variant_create_ptr(DT_PTR, new_data, delete_node_data);
}

hash_table_t*   variant_hash_init()
{
    hash_table_t* hash_table = calloc(1, sizeof(hash_table_t));
    //hash_table->hash_items = stack_create();
    hash_table->node_array = calloc(HASH_SIZE, sizeof(hash_node_t*));
    hash_table->count = 0;

    for(int i = 0; i < HASH_SIZE; i++)
    {
        hash_table->node_array[i] = calloc(1, sizeof(hash_node_t));
        hash_table->node_array[i]->data_list = stack_create();
    }

    return hash_table;
}

void            variant_hash_free(hash_table_t* hash_table)
{
    for(int i = 0; i < HASH_SIZE; i++)
    {
        stack_free(hash_table->node_array[i]->data_list);
        free(hash_table->node_array[i]);
    }

    free(hash_table->node_array);
    //stack_free(hash_table->hash_items);
    free(hash_table);
}

void            variant_hash_insert(hash_table_t* hash_table, uint32_t key, variant_t* item)
{
    int hash_index = key % HASH_SIZE;
    variant_t* hash_item = create_node_data(key, item);

    stack_for_each(hash_table->node_array[hash_index]->data_list, node_data_variant)
    {
        hash_node_data_t* node_data = (hash_node_data_t*)variant_get_ptr(node_data_variant);
        
        if(node_data->key == key)
        {
            // Same key already exists, replace the item
            delete_node_data(node_data);
            node_data_variant->storage.ptr_data = variant_get_ptr(hash_item);
            free(hash_item);
            return;
        }
    }

    // No such item was found, insert new 
    stack_push_back(hash_table->node_array[hash_index]->data_list, hash_item);
    hash_table->count++;
}

variant_t*      variant_hash_get(hash_table_t* hash_table, uint32_t key)
{
    int hash_index = key % HASH_SIZE;
    stack_for_each(hash_table->node_array[hash_index]->data_list, node_data_variant)
    {
        hash_node_data_t* node_data = (hash_node_data_t*)variant_get_ptr(node_data_variant);
        
        if(node_data->key == key)
        {
            return node_data->data;
        }
    }

    return NULL;
}

void            variant_hash_remove(hash_table_t* hash_table, uint32_t key)
{
    int hash_index = key % HASH_SIZE;
    stack_for_each(hash_table->node_array[hash_index]->data_list, node_data_variant)
    {
        hash_node_data_t* node_data = (hash_node_data_t*)variant_get_ptr(node_data_variant);
        
        if(node_data->key == key)
        {
            stack_remove(hash_table->node_array[hash_index]->data_list, node_data_variant);
            variant_free(node_data_variant);
            hash_table->count--;
            return;
        }
    }
}

void            variant_hash_for_each(hash_table_t* hash_table, void (*visitor)(hash_node_data_t*, void*), void* arg)
{
    int counter = hash_table->count;
    for(int i = 0; i < HASH_SIZE && counter > 0; i++)
    {
        if(hash_table->node_array[i]->data_list->count > 0)
        {
            stack_for_each(hash_table->node_array[i]->data_list, hash_node_variant)
            {
                visitor((hash_node_data_t*)variant_get_ptr(hash_node_variant), arg);
                counter--;
            }
        }
    }
}


