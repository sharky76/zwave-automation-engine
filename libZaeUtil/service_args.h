#include "service.h"
#include "stack.h"
#include "hash.h"
#include "variant.h"

typedef struct service_args_stack_t
{
    char* service_name;
    variant_stack_t*    stack;
    hash_table_t*       data_storage;
} service_args_stack_t;

void                    service_args_stack_create(const char* service_name);
void                    service_args_stack_add(const char* service_name, uint32_t value_key, variant_t* value);
void                    service_args_stack_remove(const char* service_name, uint32_t value_key);
service_args_stack_t*   service_args_stack_get(const char* service_name);
void                    service_args_stack_clear();
variant_t*              service_args_get_value(service_args_stack_t* stack, const char* key);
void                    service_args_for_each(service_args_stack_t* stack, void (*visitor)(uint32_t, variant_t*, void*), void* arg);


