#include "SensorEvent.h"
#include "service_args.h"
#include "logger.h"
#include "stack.h"
#include "../data_callbacks.h"
#include "../command_class.h"
#include "../vdev_manager.h"

extern ZWay zway;
USING_LOGGER(BuiltinService)

variant_t*  event_node_id_impl(struct service_method_t* method, va_list args);
variant_t*  event_command_id_impl(struct service_method_t* method, va_list args);
variant_t*  event_device_name_impl(struct service_method_t* method, va_list args);
variant_t*  event_device_value_impl(struct service_method_t* method, va_list args);

builtin_service_t*  sensor_event_service_create(hash_table_t* service_table)
{
    builtin_service_t*   service = builtin_service_create(service_table, "SensorEvent", "Sensor event data provider (only available to Scene manager)");
    builtin_service_add_method(service, "NodeId", "Return node Id", 0, event_node_id_impl);
    builtin_service_add_method(service, "CommandId", "Return command Id", 0, event_command_id_impl);
    builtin_service_add_method(service, "DeviceName", "Return device name", 0, event_device_name_impl);
    builtin_service_add_method(service, "Value", "Return device value", 1, event_device_value_impl);

    return service;
}

variant_t*  event_node_id_impl(struct service_method_t* method, va_list args)
{
    service_args_stack_t* sensor_event_stack = service_args_stack_get("SensorEvent");
    if(NULL != sensor_event_stack)
    {
        variant_t* event_data_variant = service_args_get_value(sensor_event_stack, "EventData");
    
        if(NULL != event_data_variant)
        {
            sensor_event_data_t* event_data = (sensor_event_data_t*)variant_get_ptr(event_data_variant);
            return variant_create_int32(DT_INT32, event_data->node_id);
        }
        else
        {
            LOG_ERROR(BuiltinService, "Sensor event doesnt exists");
        }
    }
    else
    {
        LOG_ERROR(BuiltinService, "No sensor events exists");
    }

    return variant_create_bool(false);
}

variant_t*  event_command_id_impl(struct service_method_t* method, va_list args)
{
    service_args_stack_t* sensor_event_stack = service_args_stack_get("SensorEvent");
    if(NULL != sensor_event_stack)
    {
        variant_t* event_data_variant = service_args_get_value(sensor_event_stack, "EventData");
    
        if(NULL != event_data_variant)
        {
            sensor_event_data_t* event_data = (sensor_event_data_t*)variant_get_ptr(event_data_variant);
            return variant_create_int32(DT_INT32, event_data->command_id);
        }
        else
        {
            LOG_ERROR(BuiltinService, "Sensor event doesnt exists");
        }
    }
    else
    {
        LOG_ERROR(BuiltinService, "No sensor events exists");
    }

    return variant_create_bool(false);
}

variant_t*  event_device_name_impl(struct service_method_t* method, va_list args)
{
    service_args_stack_t* sensor_event_stack = service_args_stack_get("SensorEvent");
    if(NULL != sensor_event_stack)
    {
        variant_t* event_data_variant = service_args_get_value(sensor_event_stack, "EventData");
    
        if(NULL != event_data_variant)
        {
            sensor_event_data_t* event_data = (sensor_event_data_t*)variant_get_ptr(event_data_variant);
            return variant_create_string(strdup(event_data->device_name));
        }
        else
        {
            LOG_ERROR(BuiltinService, "Sensor event doesnt exists");
        }
    }
    else
    {
        LOG_ERROR(BuiltinService, "No sensor events exists");
    }

    return variant_create_bool(false);
}

variant_t*  event_device_value_impl(struct service_method_t* method, va_list args)
{
    variant_t* path_variant = va_arg(args, variant_t*);
    service_args_stack_t* sensor_event_stack = service_args_stack_get("SensorEvent");
    if(NULL != sensor_event_stack)
    {
        variant_t* event_data_variant = service_args_get_value(sensor_event_stack, "EventData");
    
        if(NULL != event_data_variant)
        {
            command_class_t* cmd_class = NULL;
            sensor_event_data_t* event_data = (sensor_event_data_t*)variant_get_ptr(event_data_variant);
            device_record_t* device_record = resolver_resolve_id(event_data->node_id, event_data->instance_id, event_data->command_id);

            if(NULL != device_record)
            {
                if(device_record->devtype == VDEV)
                {
                    cmd_class = vdev_manager_get_command_class(event_data->node_id);
                }
                else
                {
                    cmd_class = get_command_class_by_id(event_data->command_id);
                }
            }
            
            if(NULL != cmd_class)
            {
                if(NULL == path_variant || *variant_get_string(path_variant) == 0)
                {
                    return command_class_exec(cmd_class, "Get", device_record, path_variant);
                }
                else
                {
                    return command_class_exec(cmd_class, "GetPath", device_record, path_variant);
                }
                
            }
            else
            {
                LOG_ERROR(BuiltinService, "Command class not handled: %d", event_data->command_id);
            }
        }
        else
        {
            LOG_ERROR(BuiltinService, "Sensor event doesnt exists");
        }
    }
    else
    {
        LOG_ERROR(BuiltinService, "No sensor events exists");
    }

    return variant_create_bool(false);
}
