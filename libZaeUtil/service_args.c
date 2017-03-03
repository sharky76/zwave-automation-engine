#include "service_args.h"
#include "crc32.h"

static hash_table_t*   service_stack_table;

void service_stack_delete(void* arg)
{
    service_args_stack_t* s = (service_args_stack_t*)arg;
    free(s->service_name);
    stack_free(s->stack);
    variant_hash_free(s->data_storage);
    free(s);
}

void                    service_args_stack_create(const char* service_name)
{
    uint32_t key = crc32(0, service_name, strlen(service_name));

    if(NULL == service_stack_table)
    {
        service_stack_table = variant_hash_init();
    }
    
    if(NULL != variant_hash_get(service_stack_table, key))
    {
        return;
    }

    service_args_stack_t* s = malloc(sizeof(service_args_stack_t));
    s->service_name = strdup(service_name);
    s->stack = stack_create();
    s->data_storage = variant_hash_init();
    variant_hash_insert(service_stack_table, key, variant_create_ptr(DT_PTR, s, service_stack_delete));
}

void                    service_args_stack_add(const char* service_name, uint32_t value_key, variant_t* value)
{
    uint32_t key = crc32(0, service_name, strlen(service_name));
    variant_t* stack_var = variant_hash_get(service_stack_table, key);

    if(NULL != stack_var)
    {
        service_args_stack_t* s = (service_args_stack_t*)variant_get_ptr(stack_var);
        variant_hash_insert(s->data_storage, value_key, value);
    }
}

void                    service_args_stack_remove(const char* service_name, uint32_t value_key)
{
    uint32_t key = crc32(0, service_name, strlen(service_name));
    variant_t* stack_var = variant_hash_get(service_stack_table, key);

    if(NULL != stack_var)
    {
        service_args_stack_t* s = (service_args_stack_t*)variant_get_ptr(stack_var);
        variant_hash_remove(s->data_storage, value_key);
    }
}

service_args_stack_t*   service_args_stack_get(const char* service_name)
{
    if(NULL != service_stack_table)
    {
        uint32_t key = crc32(0, service_name, strlen(service_name));
        variant_t* stack_var = variant_hash_get(service_stack_table, key);
    
        if(NULL != stack_var)
        {
            return (service_args_stack_t*)variant_get_ptr(stack_var);
        }
    }

    return NULL;
}

void                    service_args_stack_clear()
{
    if(NULL != service_stack_table)
    {
        variant_hash_free(service_stack_table);
        service_stack_table = NULL;
    }
}

