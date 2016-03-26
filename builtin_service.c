#include "builtin_service.h"
#include <logger.h>
#include <crc32.h>

DECLARE_LOGGER(BuiltinService)

static hash_table_t*   service_stack_table;

builtin_service_t*   builtin_service_create(hash_table_t* service_table, const char* name, const char* description)
{
    builtin_service_t* new_service = calloc(1, sizeof(builtin_service_t));
    new_service->service_name = strdup(name);
    new_service->description = strdup(description);
    new_service->service_id = variant_get_next_user_type();
    new_service->service_methods = stack_create();

    uint32_t key = crc32(0, new_service->service_name, strlen(new_service->service_name));
    variant_hash_insert(service_table, key, variant_create_ptr(DT_PTR, new_service, NULL));

    return new_service;
}

void                 builtin_service_add_method(builtin_service_t* service, const char* name, const char* help, int args, variant_t*  (*eval_callback)(struct service_method_t*, va_list))
{
    service_method_t* new_method = calloc(1, sizeof(service_method_t));
    new_method->name = strdup(name);
    new_method->help = strdup(help);
    new_method->args = args;
    new_method->eval_callback = eval_callback;

    stack_push_back(service->service_methods, variant_create_ptr(DT_PTR, new_method, NULL));
}

void service_stack_delete(void* arg)
{
    service_stack_t* s = (service_stack_t*)arg;
    free(s->service_name);
    stack_free(s->stack);
    variant_hash_free(s->data_storage);
    free(s);
}

void                 builtin_service_stack_create(const char* service_name)
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

    service_stack_t* s = malloc(sizeof(service_stack_t));
    s->service_name = strdup(service_name);
    s->stack = stack_create();
    s->data_storage = variant_hash_init();
    variant_hash_insert(service_stack_table, key, variant_create_ptr(DT_PTR, s, service_stack_delete));
}

void                 builtin_service_stack_add(const char* service_name, uint32_t value_key, variant_t* value)
{
    uint32_t key = crc32(0, service_name, strlen(service_name));
    variant_t* stack_var = variant_hash_get(service_stack_table, key);

    if(NULL != stack_var)
    {
        service_stack_t* s = (service_stack_t*)variant_get_ptr(stack_var);
        variant_hash_insert(s->data_storage, value_key, value);
    }
}

service_stack_t*     builtin_service_stack_get(const char* service_name)
{
    uint32_t key = crc32(0, service_name, strlen(service_name));
    variant_t* stack_var = variant_hash_get(service_stack_table, key);

    if(NULL != stack_var)
    {
        return (service_stack_t*)variant_get_ptr(stack_var);
    }

    return NULL;
}

void                 builtin_service_stack_clear()
{
    if(NULL != service_stack_table)
    {
        variant_hash_free(service_stack_table);
        service_stack_table = NULL;
    }
}

