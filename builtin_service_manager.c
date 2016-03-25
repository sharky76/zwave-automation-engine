#include "builtin_service_manager.h"
#include <logger.h>
#include <hash.h>
#include <crc32.h>
#include "BuiltinServices/Expression.h"
#include "BuiltinServices/Conditional.h"

USING_LOGGER(BuiltinService)

hash_table_t*   builtin_service_table;

void              builtin_service_manager_init()
{
    LOG_INFO(BuiltinService, "Initializing built-in services");
    builtin_service_table = variant_hash_init();

    expression_service_create(builtin_service_table);
    conditional_service_create(builtin_service_table);
}

service_method_t* builtin_service_manager_get_method(const char* service_class, const char* method_name)
{
    uint32_t key = crc32(0, service_class, strlen(service_class));
    variant_t* service_var = variant_hash_get(builtin_service_table, key);

    if(NULL != service_var)
    {
        builtin_service_t* service = (builtin_service_t*)variant_get_ptr(service_var);

        stack_for_each(service->service_methods, method_variant)
        {
            service_method_t* method = (service_method_t*)variant_get_ptr(method_variant);
            if(strcmp(method->name, method_name) == 0)
            {
                return method;
            }
        }
    }

    return NULL;
}

bool              builtin_service_manager_is_class_exists(const char* service_class)
{
    uint32_t key = crc32(0, service_class, strlen(service_class));
    variant_t* service_var = variant_hash_get(builtin_service_table, key);

    return (NULL != service_var);
}

builtin_service_t* builtin_service_manager_get_class(const char* service_class)
{
    uint32_t key = crc32(0, service_class, strlen(service_class));
    variant_t* retVal = variant_hash_get(builtin_service_table, key);

    return (NULL == retVal)? NULL : (builtin_service_t*)variant_get_ptr(retVal);
}

void              builtin_service_manager_for_each_class(void (*visitor)(builtin_service_t*, void*), void* arg)
{
    variant_hash_for_each_value(builtin_service_table, builtin_service_t*, visitor, arg)
}

void              builtin_service_manager_for_each_method(const char* service_class, void (*visitor)(service_method_t*, void*), void* arg)
{
    builtin_service_t* service = builtin_service_manager_get_class(service_class);

    if(NULL != service)
    {
        stack_for_each(service->service_methods, method_variant)
        {
            service_method_t* method = (service_method_t*)variant_get_ptr(method_variant);
            visitor(method, arg);
        }
    }
}

