#pragma once

/**
 * API for adding virtual devices
 */

#include "variant.h"
#include "cli.h"
#include "service.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef struct device_record_t device_record_t;

typedef struct vdev_event_data_t
{
    uint8_t  vdev_id;
    uint8_t  command_id;
    char*    data;
} vdev_event_data_t;

typedef struct vdev_command_t
{
    int   command_id;
    char* name;
    int   nargs;
    char* help;
    variant_t*   (*command_impl)(va_list);
    void* command_class_ptr;
} vdev_command_t;

typedef struct vdev_t
{
    int  vdev_id;
    const char* name;
    void  (*start)();
    char**      (*get_config_callback)(void);
    variant_stack_t*  supported_method_list;
    variant_stack_t*  event_subscriptions;
} vdev_t;

#define VDEV_INIT(_name, _start)  \
*vdev = (vdev_t*)calloc(1, sizeof(vdev_t));  \
(*vdev)->vdev_id = vdev_id;        \
(*vdev)->name = strdup(_name);      \
(*vdev)->start = _start;    \
(*vdev)->supported_method_list = stack_create();    \
(*vdev)->event_subscriptions = stack_create();

#define VDEV_ADD_COMMAND(_id, _name, _nargs, _callback, _help)  \
{   \
vdev_command_t* cmd = (vdev_command_t*)calloc(1, sizeof(vdev_command_t));  \
cmd->command_id = _id;    \
cmd->name = strdup(_name);  \
cmd->nargs = _nargs;  \
cmd->help = strdup(_help);    \
cmd->command_impl = _callback;    \
stack_push_back((*vdev)->supported_method_list, variant_create_ptr(DT_PTR, cmd, NULL)); \
}

#define VDEV_ADD_CONFIG_PROVIDER(_config)    \
(*vdev)->get_config_callback = _config;

#define VDEV_SUBSCRIBE_TO_EVENT_SOURCE(_source_, _handler_) \
    {   \
        event_subscription_t* es = (event_subscription_t*)malloc(sizeof(event_subscription_t)); \
        es->source = strdup(_source_);  \
        es->on_event = _handler_;   \
        stack_push_back((*vdev)->event_subscriptions, variant_create_ptr(DT_PTR, es, variant_delete_none)); \
    }

void    vdev_create(vdev_t** vdev, int vdev_id);
void    vdev_cli_create(cli_node_t* parent_node);
void    vdev_post_event(int vdev_id, int command_id, const char* data);
