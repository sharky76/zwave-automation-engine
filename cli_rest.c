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

extern ZWay zway;
cli_node_t*     rest_node;



bool    cmd_get_sensors(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_node_id(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_command_classes(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_command_class_info(vty_t* vty, variant_stack_t* params);

cli_command_t   rest_root_list[] = {
    {"GET rest v1 sensors",  cmd_get_sensors,  "Get list of sensors"},
    {"GET rest v1 sensors INT",  cmd_get_sensor_node_id,  "Get sensor instances info"},
    {"GET rest v1 sensors INT instances",  cmd_get_sensor_node_id,  "Get sensor instances info"},
    {"GET rest v1 sensors INT instances INT",  cmd_get_sensor_command_classes,  "Get sensor command classes info"},
    {"GET rest v1 sensors INT instances INT command-classes",  cmd_get_sensor_command_classes,  "Get sensor command classes info"},
    {"GET rest v1 sensors INT instances INT command-classes INT",  cmd_get_sensor_command_class_info,  "Get sensor command class info"},

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
    http_set_cache_control((http_vty_priv_t*)vty->priv, true, 3600);

    json_object* json_resp = json_object_new_object();
    json_object* sensor_array = json_object_new_array();
    json_object_object_add(json_resp, "sensors", sensor_array);

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

            dh = zway_find_device_data(zway,node_array[i], "deviceTypeString");
            ZWCSTR str_val;
            zdata_get_string(dh, &str_val);


            json_object_object_add(new_sensor, "node_id", json_object_new_int(node_array[i]));
            json_object_object_add(new_sensor, "name", json_object_new_string(str_val));
            json_object_object_add(new_sensor, "is_failed", json_object_new_boolean(is_failed == TRUE));

            json_object_array_add(sensor_array, new_sensor);
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
        json_object* new_instance = json_object_new_object();
        json_object_object_add(new_instance, "instance_id", json_object_new_int(0));
        json_object_array_add(instance_array, new_instance);
    
        // Add all the rest    
        while(instances[j])
        {
            json_object* new_instance = json_object_new_object();
            json_object_object_add(new_instance, "instance_id", json_object_new_int(instances[j]));
            json_object_array_add(instance_array, new_instance);
    
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
    json_object* command_class_array = json_object_new_array();
    json_object_object_add(json_resp, "command_classes", command_class_array);

    ZWCommandClassesList commands = zway_command_classes_list(zway, node_id, instance_id);
    int k = 0;

    if(NULL != commands)
    {
        
        while(commands[k])
        {
            const char* device_name = resolver_name_from_id(node_id, instance_id, commands[k]);
            command_class_t* cmd_class = get_command_class_by_id(commands[k]);
    
            json_object* new_command_class = json_object_new_object();
            json_object_object_add(new_command_class, "command_id", json_object_new_int(commands[k]));
    
            if(NULL != cmd_class)
            {
                json_object_object_add(new_command_class, "name", json_object_new_string(cmd_class->command_name));
            }
            else
            {
                json_object_object_add(new_command_class, "name", json_object_new_string("Unknown"));
            }
    
            json_object_object_add(new_command_class, "device_name", json_object_new_string(device_name));
            json_object_array_add(command_class_array, new_command_class);
    
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
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 4));
    ZWBYTE instance_id = variant_get_int(stack_peek_at(params, 6));
    ZWBYTE command_id = variant_get_int(stack_peek_at(params, 8));

    json_object* json_resp = json_object_new_object();
    json_object* command_class = json_object_new_object();
    json_object_object_add(json_resp, "command_class", command_class);

    const char* device_name = resolver_name_from_id(node_id, instance_id, command_id);

    if(NULL != device_name)
    {
        command_class_t* cmd_class = get_command_class_by_id(command_id);
        if(NULL == cmd_class)
        {
            http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_USER_ERR);
        }
        else
        {
            http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);
            json_object_object_add(command_class, "command_id", json_object_new_int(cmd_class->command_id));
            json_object_object_add(command_class, "name", json_object_new_string(cmd_class->command_name));
            json_object_object_add(command_class, "device_name", json_object_new_string(device_name));

            json_object* supported_methods_array = json_object_new_array();

            command_method_t* supported_methods = cmd_class->supported_method_list;
            while(supported_methods->name)
            {
                //command_method_t* m = &cmd_class->supported_method_list[i];

                json_object* new_method = json_object_new_object();
                json_object_object_add(new_method, "name", json_object_new_string(supported_methods->name));
                json_object_object_add(new_method, "help", json_object_new_string(supported_methods->help));
                json_object_object_add(new_method, "args", json_object_new_int(supported_methods->nargs));
                json_object_array_add(supported_methods_array, new_method);


                supported_methods++;
            }

            json_object_object_add(command_class, "methods", supported_methods_array);
        }
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

