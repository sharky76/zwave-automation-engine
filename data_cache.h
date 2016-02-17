#pragma once 
/*
This is the data cache. It stores the Data Holder objects for each discovered 
node, instance and command class 
 
The data hierarchy is node, then instance and then command class 
*/
#include <ZPlatform.h>
#include <ZData.h>
#include <stdbool.h>

/*
typedef struct data_cache_t
{
    ZWBYTE  node_id;
    variant_stack_t*    instance_list;
} data_cache_t;

typedef struct instance_cache_t
{
    ZWBYTE  instance_id;
    variant_stack_t*    command_class_list;
} instance_cache_t;

typedef struct command_class_cache_t
{
    ZWBYTE  command_class_id;
    ZDataHolder data;
} command_class_cache_t;
*/

typedef struct data_entry_key_t
{
    ZWBYTE  node_id;
    ZWBYTE  instance_id;
    ZWBYTE  command_id;
} data_entry_key_t;

typedef struct  data_entry_t
{
    data_entry_key_t* key;
    ZDataHolder       data;
} data_entry_t;

void        data_cache_init();
void        data_cache_clear();
void        data_cache_add_entry(data_entry_key_t* new_key, ZDataHolder new_data);
void        data_cache_del_entry(data_entry_key_t* key);
ZDataHolder data_cache_get_data(data_entry_key_t* key);

bool    data_cache_get_nodes(ZWBYTE* node_ids, int* count);
bool    data_cache_get_instances(ZWBYTE node_id, ZWBYTE* instances, int* count);
bool    data_cache_get_commands(ZWBYTE node_id, ZWBYTE instance_id, ZWBYTE* commands, int* count);

