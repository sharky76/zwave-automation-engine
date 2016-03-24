#pragma once
/**
 *  
 * Built-in service definition 
 *  
 * */ 
#include <service.h>
#include <hash.h>

typedef struct builtin_service_t
{
    int         service_id;
    char*       service_name;
    char*       description;
    variant_stack_t*    service_methods;
} builtin_service_t;

typedef struct service_stack_t
{
    char* service_name;
    variant_stack_t*    stack;
} service_stack_t;

static hash_table_t*   service_stack_table;

builtin_service_t*   builtin_service_create(hash_table_t* service_table, const char* name, const char* description);
void                 builtin_service_add_method(builtin_service_t* service, const char* name, const char* help, int args, variant_t*  (*eval_callback)(struct service_method_t*, va_list));
void                 builtin_service_stack_create(const char* service_name);
void                 builtin_service_stack_add(const char* service_name, variant_t* data);
service_stack_t*     builtin_service_stack_get(const char* service_name);
void                 builtin_service_stack_clear(const char* service_name);

