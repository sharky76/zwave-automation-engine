#include "cli_sensor.h"
#include <stack.h>
#include <ZWayLib.h>
#include <ZData.h>
#include <ZPlatform.h>
#include "data_cache.h"
#include "resolver.h"

extern ZWay zway;

bool    cmd_sensor_info(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_info_noname(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_show_nodes(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_guessed_info(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_show_node_info(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_force_interview(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_command_interview(vty_t* vty, variant_stack_t* params);

// Forward declaration of utility methods
void cli_print_data_holder(vty_t* vty, ZDataHolder dh);

cli_command_t   sensor_root_list[] = {
    {"show sensor",                             cmd_sensor_show_nodes,     "List sensors"},
    {"show sensor brief",                       cmd_sensor_show_node_info, "Show sensor nodes information"},
    {"show sensor name WORD",              cmd_sensor_info,           "Display sensor data"},
    {"show sensor node-id INT",            cmd_sensor_info_noname,           "Display sensor data"},
    {"show sensor node-id INT instance INT command-class INT",          cmd_sensor_info_noname,           "Display sensor data"},
    {"show sensor guessed-info node-id INT",    cmd_sensor_guessed_info,   "Display guessed sensor information"},
    {"sensor interview node-id INT",            cmd_sensor_force_interview, "Force interview for a sensor"},
    {"sensor interview node-id INT instance INT command-class INT", cmd_sensor_command_interview, "Run a command interview"},
    {NULL,                   NULL,                          NULL}
};
void    cli_sensor_init(cli_node_t* parent_node)
{
    cli_append_to_node(parent_node, sensor_root_list);
}

bool    cmd_sensor_info(vty_t* vty, variant_stack_t* params)
{
    const char* device_name = variant_get_string(stack_peek_at(params, 3));

    device_record_t* record = resolver_get_device_record(device_name);

    if(NULL != record)
    {
        vty_write(vty, "%-65s%-25s\n", "Path", "Value");
        //ZWBYTE   node_id = resolver_nodeId_from_name(vty->resolver, device_name);
        ZWBYTE node_id = record->nodeId;
        zdata_acquire_lock(ZDataRoot(zway));
        ZDataHolder dh = zway_find_device_data(zway,node_id,".");
        cli_print_data_holder(vty, dh);
    
        dh = zway_find_device_instance_data(zway,node_id,record->instanceId, ".");
        cli_print_data_holder(vty, dh);
    
        dh = zway_find_device_instance_cc_data(zway,node_id,record->instanceId, record->commandId, ".");
        cli_print_data_holder(vty, dh);
    
        zdata_release_lock(ZDataRoot(zway));
    }
    else
    {
        vty_error(vty, "No such name\n");
    }
}

// show sensor data node-id INT instance INT command-class INT
bool    cmd_sensor_info_noname(vty_t* vty, variant_stack_t* params)
{
    vty_write(vty, "%-65s%-25s\n", "Path", "Value");
    //ZWBYTE   node_id = resolver_nodeId_from_name(vty->resolver, device_name);
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 3));
    zdata_acquire_lock(ZDataRoot(zway));
    ZDataHolder dh = zway_find_device_data(zway, node_id, ".");
    cli_print_data_holder(vty, dh);

    if(params->count > 4)
    {
        dh = zway_find_device_instance_data(zway,node_id, variant_get_int(stack_peek_at(params, 5)), ".");
        cli_print_data_holder(vty, dh);
    
        dh = zway_find_device_instance_cc_data(zway,node_id,variant_get_int(stack_peek_at(params, 5)), variant_get_int(stack_peek_at(params, 7)), ".");
        cli_print_data_holder(vty, dh);
    }

    zdata_release_lock(ZDataRoot(zway));
}

bool    cmd_sensor_show_nodes(vty_t* vty, variant_stack_t* params)
{
    ZWDevicesList node_array;
    node_array = zway_devices_list(zway);

    vty_write(vty, "%-10s%-20s%-20s%-20s%-20s%-10s\n", "Node ID", "Resolved Name", "Instance", "Command Class", "Interview Done", "Is Failed");
    int i = 0;
    zdata_acquire_lock(ZDataRoot(zway));
    while(node_array[i])
    {
        if(node_array[i] != 1)
        {
            ZWBOOL is_failed = TRUE;
            ZDataHolder dh = zway_find_device_data(zway, node_array[i], "isFailed");
            zdata_get_boolean(dh, &is_failed);

            ZWInstancesList instances;
            instances = zway_instances_list(zway, node_array[i]);
            int j = 0;
            while(instances[j])
            {
                ZWCommandClassesList commands = zway_command_classes_list(zway, node_array[i], instances[j]);
                int k = 0;
                while(commands[k])
                {
                    vty_write(vty, "%-10d", node_array[i]);
                    const char* device_name = resolver_name_from_id(node_array[i], instances[j], commands[k]);
                    if(NULL != device_name)
                    {
                        device_record_t*  record = resolver_get_device_record(device_name);
                        vty_write(vty, "%-20s%-20d%-20d (0x%x)%-11s%-20s%-10s\n", record->deviceName, record->instanceId, record->commandId, record->commandId, "",
                                  (zway_device_is_interview_done(zway,node_array[i]) == TRUE)? "True" : "False",
                                  (is_failed == TRUE)? "True" : "False");
                    }
                    else
                    {
                        vty_write(vty, "%-20s%-20d%d (0x%x)%-11s%-20s%-10s\n", "Unresolved", instances[j], commands[k], commands[k], "",
                                  (zway_device_is_interview_done(zway,node_array[i]) == TRUE)? "True" : "False",
                                  (is_failed == TRUE)? "True" : "False");
                    }
    
                    k++;
                }

                zway_command_classes_list_free(commands);
    
                j++;
            }

            zway_instances_list_free(instances);
    
            ZWCommandClassesList commands = zway_command_classes_list(zway, node_array[i], 0);
            int k = 0;
            while(commands[k])
            {
                vty_write(vty, "%-10d", node_array[i]);
                const char* device_name = resolver_name_from_id(node_array[i], 0, commands[k]);
                if(NULL != device_name)
                {
                    device_record_t*  record = resolver_get_device_record(device_name);
                    vty_write(vty, "%-20s%-20d%d (0x%x)%-11s%-20s%-10s\n", record->deviceName, record->instanceId, record->commandId, record->commandId, "",
                              (zway_device_is_interview_done(zway,node_array[i]) == TRUE)? "True" : "False",
                              (is_failed == TRUE)? "True" : "False");
                }
                else
                {
                    vty_write(vty, "%-20s%-20d%d (0x%x)%-11s%-20s%-10s\n", "Unresolved", 0, commands[k], commands[k], "",
                              (zway_device_is_interview_done(zway,node_array[i]) == TRUE)? "True" : "False",
                              (is_failed == TRUE)? "True" : "False");
                }
    
                k++;
            }

            zway_command_classes_list_free(commands);
        }

        i++;
    }

    zdata_release_lock(ZDataRoot(zway));
    zway_devices_list_free(node_array);
}

bool    cmd_sensor_guessed_info(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 4));
    ZGuessedProduct* guessedProduct = zway_device_guess(zway, node_id);
    ZGuessedProduct* root = guessedProduct;

    if(NULL != root)
    {
        struct _ZGuessedProduct * pProduct = *guessedProduct;
        int savedScore = 4;
        vty_write(vty, "%-10s%-45s%-45s\n", "Score", "Device", "Vendor");
        while(NULL != pProduct)
        {
            if(pProduct->score >= savedScore)
            {
                //LOG_DEBUG("Guessed info: score %d, device: %s, vendor: %s", pProduct->score, pProduct->product, pProduct->vendor);
                vty_write(vty, "%-10d%-45s%-45s\n", pProduct->score, pProduct->product, pProduct->vendor);
            }
            guessedProduct++;
            pProduct = *guessedProduct;
        }
        
        zway_device_guess_free(root);
    }
}

bool    cmd_sensor_show_node_info(vty_t* vty, variant_stack_t* params)
{
    ZWDevicesList node_array;
    node_array = zway_devices_list(zway);

    vty_write(vty, "%-10s%-25s%-20s%-20s\n", "Node ID", "Device Type", "Interview Done", "Is Failed");
    int i = 0;
    zdata_acquire_lock(ZDataRoot(zway));

    while(node_array[i])
    {
        if(node_array[i] != 1)
        {
            ZWBOOL is_failed = TRUE;
            ZDataHolder dh = zway_find_device_data(zway,node_array[i], "isFailed");
            zdata_get_boolean(dh, &is_failed);

            dh = zway_find_device_data(zway,node_array[i], "deviceTypeString");
            ZWCSTR str_val;
            zdata_get_string(dh, &str_val);

            vty_write(vty, "%-10d%-25s%-20s%-20s\n", 
                      node_array[i], 
                      str_val,
                      (zway_device_is_interview_done(zway,node_array[i]) == TRUE)? "True" : "False",
                       (is_failed == TRUE)? "True" : "False");
        }

        i++;
    }
    zdata_release_lock(ZDataRoot(zway));
    zway_devices_list_free(node_array);
}

bool    cmd_sensor_force_interview(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 3));
    zway_device_interview_force(zway, node_id);
}

bool    cmd_sensor_command_interview(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 3));
    ZWBYTE instance_id = variant_get_int(stack_peek_at(params, 5));
    ZWBYTE command_id = variant_get_int(stack_peek_at(params, 7));
    zway_command_interview(zway, node_id, instance_id, command_id);
}

void cli_print_data_holder(vty_t* vty, ZDataHolder data)
{
    zdata_acquire_lock(ZDataRoot(zway));

    char* path = zdata_get_path(data);

    ZWDataType type;
    zdata_get_type(data, &type);

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
            vty_write(vty, "%-65s%-25s\n", path, "Empty");
            break;
        case Boolean:
            zdata_get_boolean(data, &bool_val);
            if (bool_val)
                vty_write(vty, "%-65s%-25s\n", path, ZSTR("True"));
            else
                vty_write(vty, "%-65s%-25s\n", path, ZSTR("False"));
            break;
        case Integer:
            zdata_get_integer(data, &int_val);
            vty_write(vty, "%-65s%d (0x%08x)\n", path, int_val, int_val);
            break;
        case Float:
            zdata_get_float(data, &float_val);
            vty_write(vty, "%-65s%-25f\n", path, float_val);
            break;
        case String:
            zdata_get_string(data, &str_val);
            vty_write(vty, "%-65s\"%s\"\n", path, str_val);
            break;
        case Binary:
            zdata_get_binary(data, &binary, &len);
            vty_write(vty, "%-65sbyte[%d]\n", path, len);
            //zway_dump(zway, Debug, ZSTR("  "), len, binary);
            break;
        case ArrayOfInteger:
            zdata_get_integer_array(data, &int_arr, &len);
            vty_write(vty, "%-65sint[%d]\n", path, len);
            for (i = 0; i < len; i++)
                vty_write(vty, ZSTR("  [%02d] %d"), i, int_arr[i]);
            break;
        case ArrayOfFloat:
            zdata_get_float_array(data, &float_arr, &len);
            vty_write(vty, "%-65sfloat[%d]\n", path, len);
            for (i = 0; i < len; i++)
                vty_write(vty, ZSTR("  [%02d] %f"), i, float_arr[i]);
            break;
        case ArrayOfString:
            zdata_get_string_array(data, &str_arr, &len);
            vty_write(vty, "%-65sstring[%d]\n", path, len);
            for (i = 0; i < len; i++)
                vty_write(vty, ZSTR("  [%02d] \"%s\""), i, str_arr[i]);
            break;
    }
    free(path);    

    ZDataIterator child = zdata_first_child(data);
    while (child != NULL)
    {
        cli_print_data_holder(vty, child->data);
        child = zdata_next_child(child);
    }

    zdata_release_lock(ZDataRoot(zway));
}
