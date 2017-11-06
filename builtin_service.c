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

