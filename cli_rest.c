#include "cli_rest.h"
#include <string.h>
#include <stdlib.h>
#include "resolver.h"
#include <json-c/json.h>
#include "http_server.h"
#include <ZWayLib.h>
#include <ZData.h>
#include <ZPlatform.h>
#include "command_class.h"
#include "event_log.h"

extern ZWay zway;
cli_node_t*     rest_node;

void data_holder_to_json(json_object* root, ZDataHolder data);
void command_class_to_json(command_class_t* cmd_class, void* arg);

bool    cmd_get_sensors(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_node_id(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_command_classes(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_command_class_info(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_command_class_data(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_command_class_data_short(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_command_class_list(vty_t* vty, variant_stack_t* params);

bool    cmd_set_sensor_command_class_data(vty_t* vty, variant_stack_t* params);

bool    cmd_get_events(vty_t* vty, variant_stack_t* params);

cli_command_t   rest_root_list[] = {
    {"GET rest v1 devices",  cmd_get_sensors,  "Get list of sensors"},
    {"GET rest v1 devices INT",  cmd_get_sensor_node_id,  "Get sensor instances info"},
    {"GET rest v1 devices INT instances",  cmd_get_sensor_node_id,  "Get sensor instances info"},
    {"GET rest v1 devices INT instances INT",  cmd_get_sensor_command_classes,  "Get sensor command classes info"},
    {"GET rest v1 devices INT instances INT command-classes",  cmd_get_sensor_command_classes,  "Get sensor command classes info"},
    {"GET rest v1 devices INT instances INT command-classes INT",  cmd_get_sensor_command_class_data,  "Get sensor command classes data"},
    {"GET rest v1 command-classes",  cmd_get_sensor_command_class_list,  "Get list of supported command classes"},
    {"GET rest v1 command-classes INT",  cmd_get_sensor_command_class_info,  "Get sensor command class info"},
    {"GET rest v1 devices INT instances INT command-classes INT",  cmd_get_sensor_command_class_data,  "Get sensor command classes data"},
    {"GET rest v1 devices INT INT INT",  cmd_get_sensor_command_class_data_short,  "Get sensor command classes data"},

    {"PUT rest v1 devices INT instances INT command-classes INT",  cmd_set_sensor_command_class_data,  "Set sensor command classes data"},

    {"GET rest v1 events INT", cmd_get_events,  "Get event log entries"},
    {NULL,                     NULL,                            NULL}
};

void    cli_rest_init(cli_node_t* parent_node)
{
    rest_cli_commands = stack_create();
    cli_install_custom_node(rest_cli_commands, &rest_root_node, NULL, rest_root_list, "", "config");
}

//============================================================================================================================

/* 
 
    List Sensors JSON response 
 
{ 
    sensors: [
        {
            name: "xxx",
            node_id: ID,
            instance_id: ID,
            command_id:  ID,
            type: "ZWAVE",
        },
        {
            ...
        }
    ],
    response: {
        success: true
    }
        
}
*/
void show_resolver_visitor(device_record_t* record, void* arg)
{
    json_object* json_resp = (json_object*)arg;
    json_object* new_sensor = json_object_new_object();
    json_object_object_add(new_sensor, "name", json_object_new_string(record->deviceName));
    json_object_object_add(new_sensor, "node_id", json_object_new_int(record->nodeId));
    json_object_object_add(new_sensor, "instance_id", json_object_new_int(record->instanceId));
    json_object_object_add(new_sensor, "command_id", json_object_new_int(record->commandId));
    json_object_object_add(new_sensor, "type", json_object_new_int(record->devtype));
    json_object_array_add(json_resp, new_sensor);
}

bool    cmd_get_sensors(vty_t* vty, variant_stack_t* params)
{
    http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);
    http_set_content_type((http_vty_priv_t*)vty->priv, CONTENT_TYPE_JSON);
    http_set_cache_control((http_vty_priv_t*)vty->priv, false, 3600);

    json_object* json_resp = json_object_new_object();
    json_object* sensor_array = json_object_new_object();
    json_object_object_add(json_resp, "devices", sensor_array);

    ZWDevicesList node_array;
    node_array = zway_devices_list(zway);

    int i = 0;
    zdata_acquire_lock(ZDataRoot(zway));

    while(node_array[i])
    {
        if(node_array[i] != 1)
        {
            json_object* new_sensor = json_object_new_object();

            ZWBOOL is_failed = TRUE;
            ZDataHolder dh = zway_find_device_data(zway,node_array[i], "isFailed");
            zdata_get_boolean(dh, &is_failed);

            ZWCSTR str_val;
            const char* dev_name = resolver_name_from_node_id(node_array[i]);
            if(NULL == dev_name)
            {
                dh = zway_find_device_data(zway,node_array[i], "deviceTypeString");
                zdata_get_string(dh, &str_val);
            }
            else
            {
                str_val = dev_name;
            }

            json_object_object_add(new_sensor, "node_id", json_object_new_int(node_array[i]));
            json_object_object_add(new_sensor, "name", json_object_new_string(str_val));
            json_object_object_add(new_sensor, "failed", json_object_new_boolean(is_failed == TRUE));

            char buf[4] = {0};
            snprintf(buf, 3, "%d", node_array[i]);
            json_object_object_add(sensor_array, buf, new_sensor);
        }

        i++;
    }
    zdata_release_lock(ZDataRoot(zway));
    zway_devices_list_free(node_array);

    vty_write(vty, json_object_to_json_string(json_resp));
    
    json_object_put(json_resp);
    return 0;
}


/* 
 
    Show sensor info JSON response 
 
{ 
    instances: [
        {
            instance_id: ID,
        },
        {
            ...
        }
    ],
    response: {
        success: true
    }
        
}
*/
bool    cmd_get_sensor_node_id(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 4));

    json_object* json_resp = json_object_new_object();
    json_object* instance_array = json_object_new_array();
    json_object_object_add(json_resp, "instances", instance_array);

    ZWInstancesList instances;
    instances = zway_instances_list(zway, node_id);
    int j = 0;

    if(NULL != instances)
    {
        // Add 0 instance
        json_object_array_add(instance_array, json_object_new_int(0));
    
        // Add all the rest    
        while(instances[j])
        {
            json_object_array_add(instance_array, json_object_new_int(instances[j]));
            j++;
        }

        // Just in case there is only one instance - we want j not to be zero
        j++;
    }

    zway_instances_list_free(instances);

    if(j == 0)
    {
        http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_USER_ERR);
    }
    else
    {
        http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);
    }

    http_set_content_type((http_vty_priv_t*)vty->priv, CONTENT_TYPE_JSON);
    http_set_cache_control((http_vty_priv_t*)vty->priv, true, 3600);

    vty_write(vty, json_object_to_json_string(json_resp));
    
    json_object_put(json_resp);

    return 0;
}

/* 
 
    Show sensor info JSON response 
 
{ 
    command_classes: [
        {
            command_id: ID,
            name: "name",
            device_name: "name"
        },
        {
            ...
        }
    ],
    response: {
        success: true
    }
        
}
*/
bool    cmd_get_sensor_command_classes(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 4));
    ZWBYTE instance_id = variant_get_int(stack_peek_at(params, 6));

    json_object* json_resp = json_object_new_object();
    //json_object* command_class_array = json_object_new_array();
    //json_object_object_add(json_resp, "command_classes", command_class_array);

    json_object* command_class_array = json_object_new_object();
    json_object_object_add(json_resp, "command_classes", command_class_array);


    ZWCommandClassesList commands = zway_command_classes_list(zway, node_id, instance_id);
    int k = 0;

    if(NULL != commands)
    {
        
        while(commands[k])
        {
            const char* device_name = resolver_name_from_id(node_id, instance_id, commands[k]);
            command_class_t* cmd_class = get_command_class_by_id(commands[k]);
    
            //json_object* new_command_class = json_object_new_object();
            if(NULL != cmd_class)
            {
                command_class_to_json(cmd_class, (void*)command_class_array);
            }
   
            k++;
        }
    
        zway_command_classes_list_free(commands);
    }

    if(k == 0)
    {
        http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_USER_ERR);
    }
    else
    {
        http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);
    }
    http_set_content_type((http_vty_priv_t*)vty->priv, CONTENT_TYPE_JSON);
    http_set_cache_control((http_vty_priv_t*)vty->priv, true, 3600);

    vty_write(vty, json_object_to_json_string(json_resp));
    
    json_object_put(json_resp);

    return 0;
}

bool    cmd_get_sensor_command_class_list(vty_t* vty, variant_stack_t* params)
{
    json_object* json_resp = json_object_new_object();
    //json_object* command_class_array = json_object_new_array();
    //json_object_object_add(json_resp, "command_classes", command_class_array);
    json_object* command_class_array = json_object_new_object();
    json_object_object_add(json_resp, "command_classes", command_class_array);

    command_class_for_each(command_class_to_json, command_class_array);

    http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);
    http_set_content_type((http_vty_priv_t*)vty->priv, CONTENT_TYPE_JSON);
    http_set_cache_control((http_vty_priv_t*)vty->priv, true, 3600);

    vty_write(vty, json_object_to_json_string(json_resp));
    
    json_object_put(json_resp);

    return 0;
}

void command_class_to_json(command_class_t* cmd_class, void* arg)
{
    json_object* json_array = (json_object*)arg;

    json_object* cmd_class_obj = json_object_new_object();

    json_object_object_add(cmd_class_obj, "dec_id", json_object_new_int(cmd_class->command_id));
    json_object_object_add(cmd_class_obj, "name", json_object_new_string(cmd_class->command_name));

    //json_object* supported_methods_array = json_object_new_array();
    json_object* supported_methods_array = json_object_new_object();

    command_method_t* supported_methods = cmd_class->supported_method_list;
    while(supported_methods->name)
    {
        //command_method_t* m = &cmd_class->supported_method_list[i];

        json_object* new_method = json_object_new_object();
        json_object_object_add(new_method, "name", json_object_new_string(supported_methods->name));
        json_object_object_add(new_method, "help", json_object_new_string(supported_methods->help));
        json_object_object_add(new_method, "args", json_object_new_int(supported_methods->nargs));
        //json_object_array_add(supported_methods_array, new_method);
        json_object_object_add(supported_methods_array, supported_methods->name, new_method);


        supported_methods++;
    }

    json_object_object_add(cmd_class_obj, "methods", supported_methods_array);

    char buf[6] = {0};
    snprintf(buf, 5, "0x%x", cmd_class->command_id);
    json_object_object_add(json_array, buf, cmd_class_obj);
}

/* 
 
    Show command class info JSON response 
 
{ 
        {
            command_id: ID,
            name: "name",
            device_name: "name",
 
        },
    response: {
        success: true
    }
        
}
*/
bool    cmd_get_sensor_command_class_info(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE command_id = variant_get_int(stack_peek_at(params, 4));

    json_object* json_resp = json_object_new_object();
    json_object* command_class = json_object_new_object();
    json_object_object_add(json_resp, "command_class", command_class);

    command_class_t* cmd_class = get_command_class_by_id(command_id);
    if(NULL == cmd_class)
    {
        http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_USER_ERR);
    }
    else
    {
        http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);
        command_class_to_json(cmd_class, (void*)command_class);
    }
    
    http_set_content_type((http_vty_priv_t*)vty->priv, CONTENT_TYPE_JSON);
    http_set_cache_control((http_vty_priv_t*)vty->priv, true, 3600);

    vty_write(vty, json_object_to_json_string(json_resp));
    
    json_object_put(json_resp);

    return 0;
}

bool    get_sensor_command_class_data(vty_t* vty, ZWBYTE node_id, ZWBYTE instance_id, ZWBYTE command_id)
{
    json_object* json_resp = json_object_new_object();
    const char* resolver_name = resolver_name_from_id(node_id, instance_id, command_id);
    if(NULL != resolver_name)
    {
        json_object_object_add(json_resp, "name", json_object_new_string(resolver_name));
    
        zdata_acquire_lock(ZDataRoot(zway));
        ZDataHolder dh = zway_find_device_instance_cc_data(zway, node_id, instance_id, command_id, ".");
        
        if(NULL == dh)
        {
            http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_USER_ERR);
        }
        else
        {
            json_object* device_data = json_object_new_object();
            data_holder_to_json(device_data, dh);
            json_object_object_add(json_resp, "device_data", device_data);
            http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);
        }
        zdata_release_lock(ZDataRoot(zway));
    }
    else
    {
        http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_USER_ERR);
    }

    http_set_content_type((http_vty_priv_t*)vty->priv, CONTENT_TYPE_JSON);
    http_set_cache_control((http_vty_priv_t*)vty->priv, true, 3600);

    vty_write(vty, json_object_to_json_string(json_resp));
    
    json_object_put(json_resp);

    return 0;
}
bool    cmd_get_sensor_command_class_data(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 4));
    ZWBYTE instance_id = variant_get_int(stack_peek_at(params, 6));
    ZWBYTE command_id = variant_get_int(stack_peek_at(params, 8));

    return get_sensor_command_class_data(vty, node_id,instance_id,command_id);
}

bool    cmd_get_sensor_command_class_data_short(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 4));
    ZWBYTE instance_id = variant_get_int(stack_peek_at(params, 5));
    ZWBYTE command_id = variant_get_int(stack_peek_at(params, 6));

    return get_sensor_command_class_data(vty, node_id,instance_id,command_id);
}

/*
       "NAME" : value,
 
*/
void data_holder_to_json(json_object* root, ZDataHolder data)
{
    ZDataIterator child = zdata_first_child(data);
    while (child != NULL)
    {
        const char* name = zdata_get_name(child->data);

        ZWDataType type;
        zdata_get_type(child->data, &type);
    
        ZWBOOL bool_val;
        int int_val;
        float float_val;
        ZWCSTR str_val;
        const ZWBYTE *binary;
        const int *int_arr;
        const float *float_arr;
        const ZWCSTR *str_arr;
        size_t len, i;
    
        switch (type) 
        {
        case Empty:
            json_object_object_add(root, name, json_object_new_string("Empty"));
                break;
            case Boolean:
                zdata_get_boolean(child->data, &bool_val);
                json_object_object_add(root, name, json_object_new_boolean(bool_val));
                break;
            case Integer:
                zdata_get_integer(child->data, &int_val);
                json_object_object_add(root, name, json_object_new_int(int_val));
                break;
            case Float:
                zdata_get_float(child->data, &float_val);
                json_object_object_add(root, name, json_object_new_double(float_val));
                break;
        case String:
                zdata_get_string(child->data, &str_val);
                json_object_object_add(root, name, json_object_new_string(str_val));
                break;
            case Binary:
                zdata_get_binary(child->data, &binary, &len);
                json_object* bin_array = json_object_new_array();
                for(int i = 0; i < len; i++)
                {
                    json_object_array_add(bin_array, json_object_new_int(binary[i]));
                }
    
                json_object_object_add(root, name, bin_array);
                break;
            case ArrayOfInteger:
                zdata_get_integer_array(data, &int_arr, &len);
                json_object* int_array = json_object_new_array();
                for(int i = 0; i < len; i++)
                {
                    json_object_array_add(int_array, json_object_new_int(int_arr[i]));
                }
                json_object_object_add(root, name, int_array);
                break;
            case ArrayOfFloat:
                zdata_get_float_array(data, &float_arr, &len);
                json_object* float_array = json_object_new_array();
                for(int i = 0; i < len; i++)
                {
                    json_object_array_add(float_array, json_object_new_double(float_arr[i]));
                }
                json_object_object_add(root, name, float_array);
                break;
            case ArrayOfString:
                zdata_get_string_array(data, &str_arr, &len);
                json_object* string_array = json_object_new_array();
                for(int i = 0; i < len; i++)
                {
                    json_object_array_add(string_array, json_object_new_string(str_arr[i]));
                }
                json_object_object_add(root, name, string_array);
                break;
            default:
                break;
        }
    
        ZDataIterator nested_child = zdata_first_child(child->data);
        if (nested_child != NULL)
        {
            // Nested child: create nested JSON structure
            const char* name = zdata_get_name(child->data);
            json_object* nested_data = json_object_new_object();
            data_holder_to_json(nested_data, child->data);
            json_object_object_add(root, name, nested_data);
        }

        child = zdata_next_child(child);
    }
}

bool    cmd_set_sensor_command_class_data(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 4));
    ZWBYTE instance_id = variant_get_int(stack_peek_at(params, 6));
    ZWBYTE command_id = variant_get_int(stack_peek_at(params, 8));

    const char* device_name = resolver_name_from_id(node_id, instance_id, command_id);
    const char* json_data = ((http_vty_priv_t*)vty->priv)->post_data;

    if(NULL == device_name || NULL == json_data)
    {
        http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_USER_ERR);
    }
    else
    {
        http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);
    }

    struct json_object* response_obj = json_tokener_parse(json_data);
}

bool    cmd_get_events(vty_t* vty, variant_stack_t* params)
{
    int num_lines = variant_get_int(stack_peek_at(params, 4));

    if(num_lines <= 0)
    {
        num_lines = 1;
    }

    variant_stack_t* event_list = event_log_get_tail(num_lines-1);

    json_object* json_resp = json_object_new_object();
    http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);

    stack_for_each(event_list, event_entry_variant)
    {
        event_log_entry_t* event_entry = VARIANT_GET_PTR(event_log_entry_t, event_entry_variant);
        const char* resolver_name = resolver_name_from_id(event_entry->node_id, event_entry->instance_id, event_entry->command_id);
        if(NULL != resolver_name)
        {
            json_object* event_entry_data = json_object_new_object();
            json_object_object_add(event_entry_data, "name", json_object_new_string(resolver_name));
            json_object_object_add(event_entry_data, "node_id", json_object_new_int(event_entry->node_id));
            json_object_object_add(event_entry_data, "instance_id", json_object_new_int(event_entry->instance_id));
            json_object_object_add(event_entry_data, "command_id", json_object_new_int(event_entry->command_id));
            json_object_object_add(event_entry_data, "event", json_object_new_string(event_entry->event_data));

            struct tm* ptime = localtime(&event_entry->timestamp);
            char time_str[15] = {0};
            strftime(time_str, sizeof(time_str), "%H:%M:%S", ptime);
            json_object_object_add(json_resp, time_str, event_entry_data);
        }
        else
        {
            http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_USER_ERR);
            break;
        }
    }

    http_set_content_type((http_vty_priv_t*)vty->priv, CONTENT_TYPE_JSON);
    http_set_cache_control((http_vty_priv_t*)vty->priv, false, 0);

    vty_write(vty, json_object_to_json_string(json_resp));
    
    json_object_put(json_resp);

    return 0;
}

