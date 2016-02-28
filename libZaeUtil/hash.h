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

typedef struct hash_node_t
{
    variant_stack_t*    data_list;
} hash_node_t;

//hash_node_t*    hash_table[5003];
typedef struct hash_table_t
{
    int count;
    hash_node_t** node_array;
    //variant_stack_t*    hash_items;
} hash_table_t;

#define HASH_SIZE 5003

hash_table_t*   variant_hash_init();
void            variant_hash_free(hash_table_t* hash_table);
void            variant_hash_insert(hash_table_t* hash_table, uint32_t key, variant_t* item);
variant_t*      variant_hash_get(hash_table_t* hash_table, uint32_t key);
void            variant_hash_remove(hash_table_t* hash_table, uint32_t key);
void            variant_hash_for_each(hash_table_t* hash_table, void (*visitor)(hash_node_data_t*, void*), void* arg);

