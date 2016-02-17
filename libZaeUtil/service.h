#pragma once

/*
 
API for adding service plugins  
 
*/

#include "variant.h"
#include "stack.h"
#include "cli.h"
#include "event.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

typedef struct service_method_t
{
    char*   name;
    char*   help;
    int     args;
    variant_t*  (*eval_callback)(struct service_method_t*, va_list);
} service_method_t;

typedef struct service_t
{
    int         service_id;
    char*       service_name;
    char*       description;
    char**      (*get_config_callback)(void);
    void        (*on_event)(event_t*);
    variant_stack_t*    service_methods;
} service_t;

#define SERVICE_INIT(_name, _desc)  \
*service = (service_t*)calloc(1, sizeof(service_t)); \
(*service)->service_name = strdup(#_name);  \
(*service)->description = strdup(_desc);    \
(*service)->service_id = service_id;        \
(*service)->service_methods = stack_create();

#define SERVICE_ADD_METHOD(_method_name, _method, _nargs, _help) \
service_method_t* _method_name = (service_method_t*)malloc(sizeof(service_method_t));  \
    _method_name->name = strdup(#_method_name); \
    _method_name->args = _nargs;   \
    _method_name->help = strdup(_help); \
    _method_name->eval_callback = &_method;    \
    stack_push_back((*service)->service_methods, variant_create_ptr(service_id, _method_name, NULL)); \

#define SERVICE_ADD_EVENT_HANDLER(_handler)   \
    (*service)->on_event = _handler

void    service_create(service_t** service, int service_id);
void    service_cli_create(cli_node_t* parent_node);
void    service_post_event(int source_id, const char* data);

variant_t*  service_call_method(service_t* service, const char* method_name, ...);

