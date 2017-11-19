#include "service_args.h"
#include "crc32.h"

static hash_table_t*   service_stack_table;

void service_stack_delete(void* arg)
{
    service_args_stack_t* s = (service_args_stack_t*)arg;
    free(s->service_name);
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
    s->data_storage = variant_hash_init();
    variant_hash_insert(service_stack_table, key, variant_create_ptr(DT_PTR, s, service_stack_delete));
}

void                    service_args_stack_destroy(const char* service_name)
{
    uint32_t key = crc32(0, service_name, strlen(service_name));

    if(NULL == service_stack_table)
    {
        return;
    }

    variant_t* service_args_variant = variant_hash_get(service_stack_table, key);
    if(NULL == service_args_variant)
    {
        return;
    }

    variant_hash_remove(service_stack_table, key);
}


void                    service_args_stack_add(const char* service_name, const char* key, variant_t* value)
{
    uint32_t value_key = crc32(0, key, strlen(key));
    service_args_stack_add_with_key(service_name, value_key, value);
}

void                    service_args_stack_add_with_key(const char* service_name, uint32_t key, variant_t* value)
{
    uint32_t stack_key = crc32(0, service_name, strlen(service_name));

    variant_t* stack_var = variant_hash_get(service_stack_table, stack_key);

    if(NULL != stack_var)
    {
        service_args_stack_t* s = (service_args_stack_t*)variant_get_ptr(stack_var);
        variant_hash_insert(s->data_storage, key, value);
    }

}

void                    service_args_stack_remove(const char* service_name, const char* key)
{
    uint32_t stack_key = crc32(0, service_name, strlen(service_name));
    uint32_t value_key = crc32(0, key, strlen(key));

    variant_t* stack_var = variant_hash_get(service_stack_table, stack_key);

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

variant_t*              service_args_get_value(service_args_stack_t* stack, const char* key)
{
    hash_table_t*   token_table = (hash_table_t*)stack->data_storage;
    uint32_t hash_key = crc32(0, key, strlen(key));
    variant_t* value_var = variant_hash_get(token_table, hash_key);

    return value_var;
}

void                    service_args_for_each(service_args_stack_t* stack, void (*visitor)(uint32_t, variant_t*, void*), void* arg)
{
    hash_table_t*   token_table = (hash_table_t*)stack->data_storage;
    hash_iterator_t* it = variant_hash_begin(token_table);
    while(!variant_hash_iterator_is_end(variant_hash_iterator_next(it)))
    {
        variant_t* value = variant_hash_iterator_value(it);
        uint32_t   key = variant_hash_iterator_key(it);
        visitor(key, value, arg);
    }
    free(it);
}

