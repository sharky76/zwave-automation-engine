#include "Resolver.h"
#include "../resolver.h"
#include "service_args.h"
#include "logger.h"
#include "stack.h"

USING_LOGGER(BuiltinService)

variant_t*  name_from_id_impl(struct service_method_t* method, va_list args);
variant_t*  get_list_impl(struct service_method_t* method, va_list args);
variant_t*  get_node_list_impl(struct service_method_t* method, va_list args);
variant_t*  get_node_list_for_command_id_impl(struct service_method_t* method, va_list args);

builtin_service_t*  resolver_service_create(hash_table_t* service_table)
{
    builtin_service_t*   service = builtin_service_create(service_table, "Resolver", "Name resolving methods");
    builtin_service_add_method(service, "NameFromId", "Return device name", 1, name_from_id_impl);
    builtin_service_add_method(service, "GetListOfCommandIds", "Return list of devices's command id (arg: command class) ", 1, get_list_impl);
    builtin_service_add_method(service, "GetListOfNodes", "Return list of devices's node id", 0, get_node_list_impl);
    builtin_service_add_method(service, "GetListOfNodesForCommandId", "Return list of devices's node id having command id (arg: cmd id) ", 1, get_node_list_for_command_id_impl);

    return service;
}

variant_t*  name_from_id_impl(struct service_method_t* method, va_list args)
{
    variant_t* node_id_variant = va_arg(args, variant_t*);
    const char* node_name = resolver_name_from_node_id(variant_get_int(node_id_variant));

    if(NULL != node_name)
    {
        return variant_create_string(strdup(node_name));
    }
    else
    {
        return variant_create_bool(false);
    }
}

typedef struct cmd_class_data_t
{
    int value;
    variant_stack_t* cmd_class_list;
} cmd_class_data_t;

void device_class_id_visitor(device_record_t* record, void* arg)
{
    cmd_class_data_t* data = (cmd_class_data_t*)arg;
    if(record->commandId == data->value)
    {
        stack_push_back(data->cmd_class_list, variant_create_string(strdup(record->deviceName)));
    }
}

variant_t*  get_list_impl(struct service_method_t* method, va_list args)
{
    variant_t* cmd_class_variant = va_arg(args, variant_t*);
    variant_stack_t* cmd_class_list = stack_create();

    cmd_class_data_t data = {
        .value = variant_get_int(cmd_class_variant),
        .cmd_class_list = cmd_class_list
    };

    resolver_for_each(device_class_id_visitor, (void*)&data);

   return variant_create_list(cmd_class_list);
}

void device_node_id_visitor(device_record_t* record, void* arg)
{
    cmd_class_data_t* data = (cmd_class_data_t*)arg;
    if(record->commandId == 0xff)
    {
        stack_push_back(data->cmd_class_list, variant_create_int32(DT_INT32, record->nodeId));
    }
}

variant_t*  get_node_list_impl(struct service_method_t* method, va_list args)
{
    variant_stack_t* cmd_class_list = stack_create();

    cmd_class_data_t data = {
        .cmd_class_list = cmd_class_list
    };

    resolver_for_each(device_node_id_visitor, (void*)&data);

   return variant_create_list(cmd_class_list);
}

void device_node_id_for_class_visitor(device_record_t* record, void* arg)
{
    cmd_class_data_t* data = (cmd_class_data_t*)arg;
    if(record->commandId == data->value)
    {
        stack_push_back(data->cmd_class_list, variant_create_int32(DT_INT32, record->nodeId));
    }
}

variant_t*  get_node_list_for_command_id_impl(struct service_method_t* method, va_list args)
{
    variant_t* cmd_class_variant = va_arg(args, variant_t*);
    variant_stack_t* cmd_class_list = stack_create();

    cmd_class_data_t data = {
        .cmd_class_list = cmd_class_list,
        .value = variant_get_int(cmd_class_variant)
    };

    resolver_for_each(device_node_id_for_class_visitor, (void*)&data);

   return variant_create_list(cmd_class_list);
}
