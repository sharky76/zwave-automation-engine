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
#include "event.h"
#include "zway_json.h"
#include "vdev_manager.h"
#include "sensor_manager.h"
#include "vty_io.h"

extern ZWay zway;
cli_node_t*     rest_node;

void data_leaf_to_json(json_object* root, ZDataHolder data);
void data_holder_to_json(json_object* root, ZDataHolder data);
void command_class_to_json(command_class_t* cmd_class, void* arg);

bool    cmd_get_sensors(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_node_id(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_command_classes(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_command_class_info(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_command_class_data(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_command_class_data_short(vty_t* vty, variant_stack_t* params);
bool    cmd_get_sensor_command_class_list(vty_t* vty, variant_stack_t* params);
bool    cmd_get_multisensor_command_class_data(vty_t* vty, variant_stack_t* params);
bool    cmd_get_multisensor_command_class_data_short(vty_t* vty, variant_stack_t* params);
bool    cmd_set_sensor_command_class_data(vty_t* vty, variant_stack_t* params);


bool    cmd_get_events(vty_t* vty, variant_stack_t* params);
bool    cmd_subscribe_sse(vty_t* vty, variant_stack_t* params);

void    sse_event_handler(event_t* event, void* context);

cli_command_t   rest_root_list[] = {
    {"GET rest v1 devices",  cmd_get_sensors,  "Get list of sensors"},
    {"GET rest v1 devices INT",  cmd_get_sensor_node_id,  "Get sensor instances info"},
    {"GET rest v1 devices INT instances",  cmd_get_sensor_node_id,  "Get sensor instances info"},
    {"GET rest v1 devices INT instances INT",  cmd_get_sensor_command_classes,  "Get sensor command classes info"},
    {"GET rest v1 devices INT instances INT command-classes",  cmd_get_sensor_command_classes,  "Get sensor command classes info"},
    {"GET rest v1 devices INT instances INT command-classes INT",  cmd_get_sensor_command_class_data,  "Get sensor command classes data"},
    {"GET rest v1 devices INT instances INT command-classes INT data WORD",  cmd_get_multisensor_command_class_data,  "Get sensor in a multi sensor command classes data"},

    {"GET rest v1 command-classes",  cmd_get_sensor_command_class_list,  "Get list of supported command classes"},
    {"GET rest v1 command-classes INT",  cmd_get_sensor_command_class_info,  "Get sensor command class info"},
    {"GET rest v1 devices INT INT",  cmd_get_sensor_command_classes,  "Get sensor command classes data"},
    {"GET rest v1 devices INT INT INT",  cmd_get_sensor_command_class_data_short,  "Get sensor command classes data"},
    {"GET rest v1 devices INT INT INT WORD",  cmd_get_multisensor_command_class_data_short,  "Get sensor command classes data"},

    {"POST rest v1 devices INT INT INT",  cmd_set_sensor_command_class_data,  "Set sensor command classes data"},

    {"GET rest v1 events", cmd_get_events,  "Get event log entries"},
    {"GET rest v1 events INT", cmd_get_events,  "Get event log entries"},
    {"GET rest v1 sse", cmd_subscribe_sse,  "Subscribe to Server side event bus"},

    {NULL,                     NULL,                            NULL}
};

static int CLI_REST_ID;
static variant_stack_t*    sse_event_listener_list;

void    cli_rest_init(cli_node_t* parent_node)
{
    CLI_REST_ID = variant_get_next_user_type();
    sse_event_listener_list = stack_create();
    rest_cli_commands = stack_create();
    cli_install_custom_node(rest_cli_commands, &rest_root_node, NULL, rest_root_list, "", "config");
    event_register_handler(CLI_REST_ID, EVENT_LOG_ADDED_EVENT, sse_event_handler, (void*)sse_event_listener_list);
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

void vdev_enumerate(device_record_t* record, void* arg)
{
    // Virtual devices have invalid instance ID
    if(record->devtype == VDEV && record->instanceId == INVALID_INSTANCE)
    {
        json_object* sensor_array = (json_object*)arg;
        json_object* new_sensor = json_object_new_object();
    
        json_object_object_add(new_sensor, "type", json_object_new_string("VDEV"));
        json_object_object_add(new_sensor, "node_id", json_object_new_int(record->nodeId));
        json_object_object_add(new_sensor, "name", json_object_new_string(record->deviceName));
        json_object_object_add(new_sensor, "isFailed", json_object_new_boolean(FALSE));
    
        json_object_object_add(new_sensor, "descriptor", sensor_manager_serialize(record->nodeId));

        // Command class enumeration...
        vdev_t* vdev = vdev_manager_get_vdev(record->deviceName);
        json_object* cmd_classes_array = json_object_new_array();
        
        stack_for_each(vdev->supported_method_list, vdev_command_variant)
        {
            vdev_command_t* vdev_command = VARIANT_GET_PTR(vdev_command_t, vdev_command_variant);
            if(vdev_command->command_id > 0)
            {
                json_object* cmd_class = json_object_new_object();
                json_object_object_add(cmd_class, "id", json_object_new_int(vdev_command->command_id));

                json_object* cmd_value = json_object_new_object();
                command_class_t* vdev_cmd_class = vdev_manager_get_command_class(record->nodeId);
                
                if(NULL != vdev_cmd_class)
                {
                    // Get device record
                    device_record_t* device_record = resolver_resolve_id(record->nodeId, 0, vdev_command->command_id);
    
    
                    if(NULL == device_record)
                    {
                        // We dont have resolver entry for this command class. Lets query the root device
                        device_record = record;
                    }
    
                    if(NULL != device_record)
                    {
                        json_object_object_add(cmd_class, "instance", json_object_new_int(device_record->instanceId));

                        variant_t* value = command_class_exec(vdev_cmd_class, vdev_command->name, device_record);
                        char* string_value;
                        variant_to_string(value, &string_value);
        
                        if(NULL == vdev_command->data_holder)
                        {
                            // The string value is json string to be appended directly under "dh" key
                            json_object_put(cmd_value);
                            cmd_value = json_tokener_parse(string_value);
                        }
                        else if(strchr(vdev_command->data_holder, '.') != NULL)
                        {
                            json_object* parameter_array = json_object_new_array();
                            char* data_holder = strdup(vdev_command->data_holder);
                            char* saveptr;
                            char* tok = strtok_r(data_holder, ".", &saveptr);
                            json_object* dh_item = json_object_new_object();
        
                            if(NULL != tok)
                            {
                                
                                json_object_object_add(dh_item, "data_holder", json_object_new_string(tok));
                            }
        
                            tok = strtok_r(NULL, ".", &saveptr);
        
                            if(NULL != tok)
                            {
                                json_object_object_add(dh_item, tok, json_object_new_string(string_value));
                            }
                            
                            json_object_array_add(parameter_array, dh_item);
                            json_object_object_add(cmd_value, "parameters", parameter_array);
                            free(data_holder);
                        }
                        else
                        {
            
                            json_object_object_add(cmd_value, vdev_command->data_holder, json_object_new_string(string_value));
                        }
                        
                        free(string_value);
                        variant_free(value);
    
                        json_object_object_add(cmd_class, "dh", cmd_value);
                    }
                }

                json_object_array_add(cmd_classes_array, cmd_class);
            }
        }

        json_object_object_add(new_sensor, "command_classes", cmd_classes_array);
        //char buf[4] = {0};
        //snprintf(buf, 3, "%d", record->nodeId);
        json_object_array_add(sensor_array, new_sensor);
    }
}

bool    cmd_get_sensors(vty_t* vty, variant_stack_t* params)
{
    http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);
    http_set_content_type((http_vty_priv_t*)vty->priv, CONTENT_TYPE_JSON);
    http_set_cache_control((http_vty_priv_t*)vty->priv, false, 3600);

    json_object* json_resp = json_object_new_object();
    json_object* sensor_array = /*json_object_new_object();*/ json_object_new_array();
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
            dh = zway_find_device_data(zway,node_array[i], "deviceTypeString");
            zdata_get_string(dh, &str_val);

            json_object_object_add(new_sensor, "type", json_object_new_string("ZWAVE"));

            json_object_object_add(new_sensor, "node_id", json_object_new_int(node_array[i]));
            json_object_object_add(new_sensor, "name", json_object_new_string(dev_name));
            json_object_object_add(new_sensor, "deviceTypeString", json_object_new_string(str_val));
            json_object_object_add(new_sensor, "isFailed", json_object_new_boolean(is_failed == TRUE));

            // char* sensor_role = sensor_manager_get_role(node_array[i]);
            // if(NULL != sensor_role)
            // {
            //     json_object_object_add(new_sensor, "role", json_object_new_string(sensor_role));
            // }

            json_object_object_add(new_sensor, "descriptor", sensor_manager_serialize(node_array[i]));

            json_object* command_class_array = json_object_new_array();
            json_object_object_add(new_sensor, "command_classes", command_class_array);

            
            int j = 0;

            ZWInstancesList instances = zway_instances_list(zway, node_array[i]);

            if(NULL != instances)
            {
                while(instances[j])
                {
                    int k = 0;
                    ZWCommandClassesList commands = zway_command_classes_list(zway, node_array[i], instances[j]);

                    if(NULL != commands)
                    {
                        while(commands[k])
                        {
                            json_object* command_class_dh_obj = json_object_new_object();
                            json_object_object_add(command_class_dh_obj, "id", json_object_new_int(commands[k]));
                            json_object_object_add(command_class_dh_obj, "instance", json_object_new_int(instances[j]));

                            ZDataHolder cmd_class_dh = zway_find_device_instance_cc_data(zway, node_array[i], instances[j], commands[k], ".");
                            json_object* device_data = json_object_new_object();
                            zway_json_data_holder_to_json(device_data, cmd_class_dh);
                            json_object_object_add(command_class_dh_obj, "dh", device_data);
                            json_object_array_add(command_class_array, command_class_dh_obj);
                            k++;
                        }
                    }
            
                    zway_command_classes_list_free(commands);
                    j++;
                }

                zway_instances_list_free(instances);
            }

            ZWCommandClassesList commands = zway_command_classes_list(zway, node_array[i], 0);

            if(NULL != commands)
            {
                int k = 0;
                while(commands[k])
                {
                    json_object* command_class_dh_obj = json_object_new_object();
                    json_object_object_add(command_class_dh_obj, "id", json_object_new_int(commands[k]));
                    json_object_object_add(command_class_dh_obj, "instance", json_object_new_int(0));

                    ZDataHolder cmd_class_dh = zway_find_device_instance_cc_data(zway, node_array[i], 0, commands[k], ".");
                    json_object* device_data = json_object_new_object();
                    zway_json_data_holder_to_json(device_data, cmd_class_dh);
                    json_object_object_add(command_class_dh_obj, "dh", device_data);
                    json_object_array_add(command_class_array, command_class_dh_obj);
                    k++;
                }
            }
    
            zway_command_classes_list_free(commands);

            //char buf[4] = {0};
            //snprintf(buf, 3, "%d", node_array[i]);
            //json_object_object_add(sensor_array, buf, new_sensor);
            json_object_array_add(sensor_array, new_sensor);
        }

        i++;
    }
    zdata_release_lock(ZDataRoot(zway));
    zway_devices_list_free(node_array);


    // Now add virtual devices
    resolver_for_each(vdev_enumerate, sensor_array);

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

    const char* dev_name = resolver_name_from_node_id(node_id);
    device_record_t* record = resolver_get_device_record(dev_name);

    int j = 0;
    json_object* json_resp = json_object_new_object();

    if(record->devtype == ZWAVE)
    {
        json_object* instance_array = json_object_new_array();
        json_object_object_add(json_resp, "instances", instance_array);
    
        ZWInstancesList instances;
        instances = zway_instances_list(zway, node_id);
    
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
    }
    else if(record->devtype == VDEV)
    {
        // Virtual devices have only one instance
        json_object* instance_array = json_object_new_array();
        json_object_object_add(json_resp, "instances", instance_array);
        json_object_array_add(instance_array, json_object_new_int(0));
        j++;
    }

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
    ZWBYTE instance_id = variant_get_int(stack_peek_at(params, params->count-1));

    json_object* json_resp = json_object_new_object();
    //json_object* command_class_array = json_object_new_array();
    //json_object_object_add(json_resp, "command_classes", command_class_array);

    json_object* command_class_array = json_object_new_object();
    json_object_object_add(json_resp, "command_classes", command_class_array);

    const char* dev_name = resolver_name_from_node_id(node_id);
    device_record_t* record = resolver_get_device_record(dev_name);
    int k = 0;

    if(NULL == record || record->devtype == ZWAVE)
    {
        ZWCommandClassesList commands = zway_command_classes_list(zway, node_id, instance_id);
    
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
    }
    else if(record->devtype == VDEV)
    {
        command_class_t* cmd_class = vdev_manager_get_command_class(node_id);
        if(NULL != cmd_class)
        {
            command_class_to_json(cmd_class, (void*)command_class_array);
        }

        k++;
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

        if(NULL != supported_methods->help)
        {
            json_object_object_add(new_method, "help", json_object_new_string(supported_methods->help));
        }

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

bool    get_sensor_command_class_data(vty_t* vty, ZWBYTE node_id, ZWBYTE instance_id, ZWBYTE command_id, const char* path)
{
    json_object* json_resp = json_object_new_object();

    device_record_t* record = resolver_resolve_id(node_id, instance_id, command_id);

    if(NULL == record)
    {
        // Maybe its virtual device, lets get its root
        const char* name = resolver_name_from_node_id(node_id);
        if(NULL != name)
        {
            record = resolver_get_device_record(name);
        }
    }

    if(NULL != record)
    {
        //const char* resolver_name = resolver_name_from_id(node_id, instance_id, command_id);
        if(record->devtype == ZWAVE)
        {
            json_object_object_add(json_resp, "name", json_object_new_string(record->deviceName));
        
            zdata_acquire_lock(ZDataRoot(zway));
            ZDataHolder dh = zway_find_device_instance_cc_data(zway, node_id, instance_id, command_id, path);
            
            if(NULL == dh)
            {
                http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_USER_ERR);
            }
            else
            {
                json_object* device_data = json_object_new_object();
                zway_json_data_holder_to_json(device_data, dh);
                json_object_object_add(json_resp, "device_data", device_data);
                http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);
            }
            zdata_release_lock(ZDataRoot(zway));
        }
        else
        {
            // This is VDEV response...
            // { "name": "3.0.SensorBinary", "device_data": { "sensorTypeString": "General purpose", "level": false, "parameters": [ ] } }

            json_object_object_add(json_resp, "name", json_object_new_string(record->deviceName));
            
            // Find root VDEV
            const char* root_vdev_name = resolver_name_from_node_id(node_id);

            if(NULL != root_vdev_name)
            {
                vdev_t* vdev = vdev_manager_get_vdev(root_vdev_name);
    
                stack_for_each(vdev->supported_method_list, vdev_command_variant)
                {
                    vdev_command_t* vdev_command = VARIANT_GET_PTR(vdev_command_t, vdev_command_variant);
                    if(vdev_command->command_id == command_id)
                    {
                        json_object* cmd_value = json_object_new_object();
                        command_class_t* vdev_cmd_class = vdev_manager_get_command_class(record->nodeId);
                        if(NULL != vdev_cmd_class)
                        {
                            variant_t* value = command_class_exec(vdev_cmd_class, vdev_command->name, record);
                            char* string_value;
                            variant_to_string(value, &string_value);
        
                            if(NULL == vdev_command->data_holder)
                            {
                                // The string value is json string to be appended directly under "dh" key
                                json_object_put(cmd_value);
                                cmd_value = json_tokener_parse(string_value);
                            }
                            else if(strchr(vdev_command->data_holder, '.') != NULL)
                            {
                                char* data_holder = strdup(vdev_command->data_holder);
                                char* saveptr;
                                char* tok = strtok_r(data_holder, ".", &saveptr);
                                json_object* dh_item = json_object_new_object();
            
                                if(NULL != tok)
                                {
                                    
                                    json_object_object_add(dh_item, "data_holder", json_object_new_string(tok));
                                }
            
                                tok = strtok_r(NULL, ".", &saveptr);
            
                                if(NULL != tok)
                                {
                                    json_object_object_add(dh_item, tok, json_object_new_string(string_value));
                                }
                                
                                if(NULL == path)
                                {
                                    json_object* parameter_array = json_object_new_array();
                                    json_object_array_add(parameter_array, dh_item);
                                    json_object_object_add(cmd_value, "parameters", parameter_array);
                                }
                                else
                                {
                                    json_object_object_add(cmd_value, tok, json_object_new_string(string_value));
                                    json_object_put(dh_item);
                                }
                                free(data_holder);
                            }
                            else
                            {
                
                                json_object_object_add(cmd_value, vdev_command->data_holder, json_object_new_string(string_value));
                            }
                        
    
                            //json_object_object_add(cmd_value, vdev_command->data_holder, json_object_new_string(string_value));
                            free(string_value);
                            variant_free(value);
                        }

                        json_object_object_add(json_resp, "device_data", cmd_value);
                        http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);
                        break;
                    }
                }
            }
            else
            {
                http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_SERVER_ERR);
            }
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
bool    cmd_get_sensor_command_class_data(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 4));
    ZWBYTE instance_id = variant_get_int(stack_peek_at(params, 6));
    ZWBYTE command_id = variant_get_int(stack_peek_at(params, 8));

    return get_sensor_command_class_data(vty, node_id,instance_id,command_id, ".");
}

bool    cmd_get_multisensor_command_class_data(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 4));
    ZWBYTE instance_id = variant_get_int(stack_peek_at(params, 6));
    ZWBYTE command_id = variant_get_int(stack_peek_at(params, 8));

    char* data_string;
    variant_t* data_variant = stack_peek_at(params, 10);
    variant_to_string(data_variant, &data_string);

    bool retVal = get_sensor_command_class_data(vty, node_id,instance_id,command_id, data_string);
    free(data_string);
    return retVal;
}

bool    cmd_get_sensor_command_class_data_short(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 4));
    ZWBYTE instance_id = variant_get_int(stack_peek_at(params, 5));
    ZWBYTE command_id = variant_get_int(stack_peek_at(params, 6));

    return get_sensor_command_class_data(vty, node_id,instance_id,command_id, ".");
}

bool    cmd_get_multisensor_command_class_data_short(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 4));
    ZWBYTE instance_id = variant_get_int(stack_peek_at(params, 5));
    ZWBYTE command_id = variant_get_int(stack_peek_at(params, 6));

    char* data_string;
    variant_t* data_variant = stack_peek_at(params, 7);
    variant_to_string(data_variant, &data_string);

    bool retVal = get_sensor_command_class_data(vty, node_id,instance_id,command_id, data_string);
    free(data_string);
    return retVal;
}

/*
    JSON request data:
 
    {
      "command"  : "Set",
      "arguments": [true, 12, 23]
    }
*/
bool    cmd_set_sensor_command_class_data(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 4));
    ZWBYTE instance_id = variant_get_int(stack_peek_at(params, 5));
    ZWBYTE command_id = variant_get_int(stack_peek_at(params, 6));

    const char* device_name = resolver_name_from_id(node_id, instance_id, command_id);
    const char* json_data = ((http_vty_priv_t*)vty->priv)->post_data;

    if(NULL == device_name || NULL == json_data)
    {
        http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_USER_ERR);
    }
    else
    {
        http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);
        struct json_object* response_obj = json_tokener_parse(json_data);

        command_class_t* command_class = get_command_class_by_id(command_id);
        if(NULL != command_class)
        {
            //LOG_DEBUG(CLI, "Matching command-class %s found for sensor %s", command_class->command_name, event_data->device_name);
            const char* device_name = resolver_name_from_id(node_id, instance_id, command_id);

            if(NULL != device_name)
            {
                device_record_t* device_record = resolver_get_device_record(device_name);

                if(NULL != device_record)
                {
                    struct json_object* cmd_object;
                    if(json_object_object_get_ex(response_obj, "command", &cmd_object) == TRUE)
                    {
                        const char* command = json_object_get_string(cmd_object);

                        // We support maximum 4 arguments...
                        variant_t* arg_list[4] = {0};
                        
                        struct json_object* arg_array;
                        if(json_object_object_get_ex(response_obj, "arguments", &arg_array) == TRUE)
                        {
                            int arg_count = json_object_array_length(arg_array);

                            for(int i = 0; i < arg_count; i++)
                            {
                                json_object* entry = json_object_array_get_idx(arg_array, i);
                                switch(json_object_get_type(entry))
                                {
                                case json_type_boolean:
                                    arg_list[i] = variant_create_bool(json_object_get_boolean(entry));
                                    break;
                                case json_type_int:
                                    arg_list[i] = variant_create_int32(DT_INT32, json_object_get_int(entry));
                                    break;
                                case json_type_string:
                                    arg_list[i] = variant_create_string(json_object_get_string(entry));
                                    break;
                                default:
                                    break;
                                }
                            }

                            switch(arg_count)
                            {
                            case 1:
                                command_class_exec(command_class, command, device_record, arg_list[0]);
                                break;
                            case 2:
                                command_class_exec(command_class, command, device_record, arg_list[0], arg_list[1]);
                                break;
                            case 3:
                                command_class_exec(command_class, command, device_record, arg_list[0], arg_list[1], arg_list[2]);
                                break;
                            case 4:
                                command_class_exec(command_class, command, device_record, arg_list[0], arg_list[1], arg_list[2], arg_list[3]);
                                break;
                            default:
                                break;
                            }
                        }
                    }
                }
            }
        }

    }

}

bool    cmd_get_events(vty_t* vty, variant_stack_t* params)
{
    int num_lines;

    if(params->count == 4)
    {
        num_lines = event_log_get_size();
    }
    else
    {
        num_lines = variant_get_int(stack_peek_at(params, 4));
    
        if(num_lines <= 0)
        {
            num_lines = 1;
        }
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

/*
    This event handler is called when new event log entry is added
*/
void    sse_event_handler(event_t* event, void* context)
{
    variant_stack_t* event_listeners = (variant_stack_t*)context;

    variant_stack_t* vty_to_remove = stack_create();

    stack_lock(event_listeners);
    stack_for_each(event_listeners, vty_variant)
    {
        vty_t* vty = VARIANT_GET_PTR(vty_t, vty_variant);
        event_log_entry_t* e = VARIANT_GET_PTR(event_log_entry_t, event->data);
        
        http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_NONE);

        // Add data...
        vty_write(vty, "id: %d\nevent: %s\ndata: {\"type\": \"%s\",\"node_id\": \"%d\",\"instance_id\":\"%d\",\"command_id\":\"%d\",\"data\":%s}\n\n", 
                  e->event_id, 
                  EVENT_LOG_ADDED_EVENT, 
                  (e->device_type == ZWAVE)? "ZWAVE" : "VDEV",
                  e->node_id,
                  e->instance_id,
                  e->command_id,
                  e->event_data);

        // Send data
        if(!vty_flush(vty))
        {
            stack_push_back(vty_to_remove, vty_variant);
            
        }
    }

    stack_for_each(vty_to_remove, dead_vty_variant)
    {
        stack_remove(event_listeners, dead_vty_variant);
        vty_t* vty = VARIANT_GET_PTR(vty_t, dead_vty_variant);
        vty_set_in_use(vty, false);
        vty_free(vty);
    }

    stack_free(vty_to_remove);

    stack_unlock(event_listeners);

}

bool    cmd_subscribe_sse(vty_t* vty, variant_stack_t* params)
{
    http_set_content_type((http_vty_priv_t*)vty->priv, "text/event-stream");
    http_set_cache_control((http_vty_priv_t*)vty->priv, false, 0);
    http_set_header((http_vty_priv_t*)vty->priv, "Connection", "keep-alive");

    char* accept_type;
    if(http_request_find_header_value((http_vty_priv_t*)vty->priv, "Accept", &accept_type))
    {
        if(strstr(accept_type, "text/event-stream") != NULL)
        {
            http_set_response((http_vty_priv_t*)vty->priv, HTTP_RESP_OK);

            char* last_event_id;
            if(http_request_find_header_value((http_vty_priv_t*)vty->priv, "last-event-id", &last_event_id))
            {
                // Send all pending events from last-event-id till latest event
        
                free(last_event_id);
            }
            else
            {
                // Send response with empty ID - this will reset
                // client id counter too.
                //vty_write(vty, "\n");
                vty_flush(vty);
            }

            vty_set_in_use(vty, true); // Mark this VTY as "in use" so it will not be deleted
            stack_lock(sse_event_listener_list);
            stack_push_back(sse_event_listener_list, variant_create_ptr(DT_PTR, vty, variant_delete_none));
            stack_unlock(sse_event_listener_list);
        }

        free(accept_type);
    }
    return true;
}

void cli_rest_handle_connect_event(int fd, void* context)
{
    int session_sock = accept(fd, NULL, NULL);
    vty_io_data_t* vty_data = malloc(sizeof(vty_io_data_t));
    vty_data->desc.socket = session_sock;

    vty_t* vty_sock = vty_io_create(VTY_HTTP, vty_data);
    vty_set_echo(vty_sock, false);

    event_register_fd(session_sock, &cli_rest_handle_data_event, (void*)vty_sock);
}

void    cli_rest_handle_data_event(int socket, void* context)
{
    vty_t* vty_sock = (vty_t*)context;
    char* str = vty_read(vty_sock);

    if(NULL != str)
    {
        cli_command_exec_custom_node(rest_root_node, vty_sock, str);
    }

    event_unregister_fd(socket);
    vty_free(vty_sock);
}
