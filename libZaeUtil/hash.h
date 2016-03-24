#pragma once
#include "stack.h"
#include <stdint.h>

/*
This is a variant hash table implementation 
*/
typedef struct hash_node_data_t
{
    uint32_t    key;
    variant_t*  data;
} hash_node_data_t;

//hash_node_t*    hash_table[5003];
typedef struct hash_table_t
{
    int count;
    int hash_size;
    hash_node_data_t** node_array;
} hash_table_t;

typedef struct hash_iterator_t
{
    hash_table_t* hash_table;
    int index;
} hash_iterator_t;


#define variant_hash_for_each_value(hash_table, type, visitor, arg) \
{                                                                       \
    int counter = hash_table->count;                                    \
    for(int i = 0; i < hash_table->hash_size && counter > 0; i++)       \
    {                                                                   \
        if(NULL != hash_table->node_array[i])                           \
        {                                                               \
            visitor((type)variant_get_ptr(hash_table->node_array[i]->data), arg);              \
            --counter;                                                  \
        }                                                               \
    }                                                                   \
}                                                                       

hash_table_t*   variant_hash_init();
void            variant_hash_free(hash_table_t* hash_table);
void            variant_hash_free_void(void* hash_table);

void            variant_hash_insert(hash_table_t* hash_table, uint32_t key, variant_t* item);
variant_t*      variant_hash_get(hash_table_t* hash_table, uint32_t key);
void            variant_hash_remove(hash_table_t* hash_table, uint32_t key);
void            variant_hash_for_each(hash_table_t* hash_table, void (*visitor)(hash_node_data_t*, void*), void* arg);
//void            variant_hash_for_each_value(hash_table_t* hash_table, void (*visitor)(variant_t*, void*), void* arg);

// For debugging
void            variant_hash_print(hash_table_t* hash_table);

hash_iterator_t*    variant_hash_begin(hash_table_t* hash_table);
hash_iterator_t*    variant_hash_end(hash_table_t* hash_table);
bool                variant_hash_iterator_is_end(hash_iterator_t* it);
hash_iterator_t*    variant_hash_iterator_next(hash_iterator_t* it);
variant_t*          variant_hash_iterator_value(hash_iterator_t* it);
