#include "cli_sensor.h"
#include <stack.h>
#include <ZWayLib.h>
#include <ZData.h>
#include <ZPlatform.h>
#include "resolver.h"
#include "config.h"
#include "command_class.h"
#include "sensor_manager.h"

extern ZWay zway;

bool    cmd_sensor_info(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_info_noname(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_show_nodes(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_guessed_info(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_show_node_info(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_force_interview(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_command_interview(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_set_descriptor(vty_t* vty, variant_stack_t* params);
bool    cmd_sensor_show_descriptor(vty_t* vty, variant_stack_t* params);
bool cmd_enter_sensor_descriptor_node(vty_t* vty, variant_stack_t* params);
bool cmd_sensor_remove_descriptor(vty_t* vty, variant_stack_t* params);

bool cmd_sensor_descriptor_set_role(vty_t* vty, variant_stack_t* params);
bool cmd_sensor_descriptor_set_name(vty_t* vty, variant_stack_t* params);
bool cmd_sensor_descriptor_set_notification(vty_t* vty, variant_stack_t* params);
bool cmd_sensor_descriptor_set_sensor_alarm(vty_t* vty, variant_stack_t* params);
bool cmd_sensor_descriptor_set_instance_name(vty_t* vty, variant_stack_t* params);
bool cmd_sensor_descriptor_remove_instance_name(vty_t* vty, variant_stack_t* params);

// Forward declaration of utility methods
void cli_print_data_holder(vty_t* vty, ZDataHolder dh);

cli_node_t*     sensor_descriptor_node;

cli_command_t   sensor_root_list[] = {
    {"list sensor",                             cmd_sensor_show_nodes,     "List sensors"},
    {"list sensor brief",                       cmd_sensor_show_node_info, "Show sensor nodes information"},
    {"show sensor name WORD",              cmd_sensor_info,           "Display sensor data"},
    {"show sensor node-id INT",            cmd_sensor_info_noname,           "Display sensor data"},
    {"show sensor node-id INT instance INT command-class INT",          cmd_sensor_info_noname,           "Display sensor data"},
    {"show sensor guessed-info node-id INT",    cmd_sensor_guessed_info,   "Display guessed sensor information"},
    {"sensor interview node-id INT",            cmd_sensor_force_interview, "Force interview for a sensor"},
    {"sensor interview node-id INT instance INT command-class INT", cmd_sensor_command_interview, "Run a command interview"},
    {"sensor set-descriptor node-id INT WORD",              cmd_sensor_set_descriptor, "Set ZDDX descriptor file for sensor"},
    {"show sensor descriptor", cmd_sensor_show_descriptor, "Show sensor descriptor"},
    {"sensor descriptor node-id INT", cmd_enter_sensor_descriptor_node, "Confgure sensor descriptor"},
    {"no sensor descriptor node-id INT", cmd_sensor_remove_descriptor, "Remove sensor descriptor"},
    {NULL,                   NULL,                          NULL}
};

cli_command_t sensor_descriptor_command_list[] = {
    {"command-class INT role MotionSensor|ContactSensor|LeakSensor|OccupancySensor|GarageDoorOpener|Lightbulb|SmokeSensor|LockMechanism|Disabled", cmd_sensor_descriptor_set_role, "Set sensor role"},
    {"name LINE",                                                  cmd_sensor_descriptor_set_name, "Set sensor name"},
    {"instance INT name LINE",                                     cmd_sensor_descriptor_set_instance_name, "Set name for a sensor instance"},
    {"no instance INT name LINE",                                     cmd_sensor_descriptor_remove_instance_name, "Remove name for a sensor instance"},
    //{"notification MotionDetected|LeakDetected|SirenActive|SmokeDetected",       cmd_sensor_descriptor_set_notification, "Set sensor notification"},
    {"sensor-alarm INT role Tampered|LeakDetected|Smoke|CO|CO2|Heat|LockOperation",  cmd_sensor_descriptor_set_sensor_alarm, "Set sensor alarm notification"},
    {NULL,                   NULL,                          NULL}
};

void    cli_sensor_init(cli_node_t* parent_node)
{
    cli_append_to_node(parent_node, sensor_root_list);
    cli_install_node(&sensor_descriptor_node, parent_node, sensor_descriptor_command_list, "sensor-descriptor", "sensor-descriptor");
}

bool    cmd_sensor_info(vty_t* vty, variant_stack_t* params)
{
    const char* device_name = variant_get_string(stack_peek_at(params, 3));

    device_record_t* record = resolver_get_device_record(device_name);

    if(NULL != record)
    {
        vty_write(vty, "%-65s%-25s%s", "Path", "Value", VTY_NEWLINE(vty));
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
        vty_error(vty, "No such device %s%s", device_name, VTY_NEWLINE(vty));
    }
}

// show sensor data node-id INT instance INT command-class INT
bool    cmd_sensor_info_noname(vty_t* vty, variant_stack_t* params)
{
    vty_write(vty, "%-65s%-25s%s", "Path", "Value", VTY_NEWLINE(vty));
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

    vty_write(vty, "%-10s%-20s%-20s%-20s%-20s%-10s%s", "Node ID", "Resolved Name", "Instance", "Command Class", "Interview Done", "Is Failed", VTY_NEWLINE(vty));
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
                        vty_write(vty, "%-20s %-20d%-20d (0x%x)%-11s%-20s%-10s%s", record->deviceName, record->instanceId, record->commandId, record->commandId, "",
                                  (zway_device_is_interview_done(zway,node_array[i]) == TRUE)? "True" : "False",
                                  (is_failed == TRUE)? "True" : "False", VTY_NEWLINE(vty));
                    }
                    else
                    {
                        command_class_t* cmd_class = get_command_class_by_id(commands[k]);
                        if(NULL == cmd_class)
                        {
                            vty_write(vty, "%-20s %-20d%d (0x%x)%-11s%-20s%-10s%s", "Unresolved", instances[j], commands[k], commands[k], "",
                                      (zway_device_is_interview_done(zway,node_array[i]) == TRUE)? "True" : "False",
                                      (is_failed == TRUE)? "True" : "False", VTY_NEWLINE(vty));
                        }
                        else
                        {
                            vty_write(vty, "%-20s %-20d%d (0x%x)%-11s%-20s%-10s%s", cmd_class->command_name, instances[j], commands[k], commands[k], "",
                                      (zway_device_is_interview_done(zway,node_array[i]) == TRUE)? "True" : "False",
                                      (is_failed == TRUE)? "True" : "False", VTY_NEWLINE(vty));
                        }
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
                    vty_write(vty, "%-20s%-20d%d (0x%x)%-11s%-20s%-10s%s", record->deviceName, record->instanceId, record->commandId, record->commandId, "",
                              (zway_device_is_interview_done(zway,node_array[i]) == TRUE)? "True" : "False",
                              (is_failed == TRUE)? "True" : "False", VTY_NEWLINE(vty));
                }
                else
                {
                    command_class_t* cmd_class = get_command_class_by_id(commands[k]);

                    if(NULL == cmd_class)
                    {
                        vty_write(vty, "%-20s%-20d%d (0x%x)%-11s%-20s%-10s%s", "Unresolved", 0, commands[k], commands[k], "",
                                  (zway_device_is_interview_done(zway,node_array[i]) == TRUE)? "True" : "False",
                                  (is_failed == TRUE)? "True" : "False", VTY_NEWLINE(vty));
                    }
                    else
                    {
                        vty_write(vty, "%-20s %-20d%d (0x%x)%-11s%-20s%-10s%s", cmd_class->command_name, 0, commands[k], commands[k], "",
                                      (zway_device_is_interview_done(zway,node_array[i]) == TRUE)? "True" : "False",
                                      (is_failed == TRUE)? "True" : "False", VTY_NEWLINE(vty));
                    }
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
        vty_write(vty, "%-10s%-45s%-45s%-45s%s", "Score", "Device", "Vendor", "ZDDX", VTY_NEWLINE(vty));
        while(NULL != pProduct)
        {
            if(pProduct->score >= savedScore)
            {
                //LOG_DEBUG("Guessed info: score %d, device: %s, vendor: %s", pProduct->score, pProduct->product, pProduct->vendor);
                vty_write(vty, "%-10d%-45s%-45s%-45s%s", pProduct->score, pProduct->product, pProduct->vendor, pProduct->file_name, VTY_NEWLINE(vty));
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

    vty_write(vty, "%-10s%-25s%-25s%-20s%-20s%s", "Node ID", "Device Type", "Name", "Interview Done", "Is Failed", VTY_NEWLINE(vty));
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

            vty_write(vty, "%-10d%-25s%-25s%-20s%-20s%s", 
                      node_array[i], 
                      str_val,
                      resolver_name_from_node_id(node_array[i]),
                      (zway_device_is_interview_done(zway,node_array[i]) == TRUE)? "True" : "False",
                       (is_failed == TRUE)? "True" : "False",
                      VTY_NEWLINE(vty));
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
            vty_write(vty, "%-65s%-25s%s", path, "Empty", VTY_NEWLINE(vty));
            break;
        case Boolean:
            zdata_get_boolean(data, &bool_val);
            if (bool_val)
                vty_write(vty, "%-65s%-25s%s", path, ZSTR("True"), VTY_NEWLINE(vty));
            else
                vty_write(vty, "%-65s%-25s%s", path, ZSTR("False"), VTY_NEWLINE(vty));
            break;
        case Integer:
            zdata_get_integer(data, &int_val);
            vty_write(vty, "%-65s%d (0x%08x)%s", path, int_val, int_val, VTY_NEWLINE(vty));
            break;
        case Float:
            zdata_get_float(data, &float_val);
            vty_write(vty, "%-65s%-25f%s", path, float_val, VTY_NEWLINE(vty));
            break;
        case String:
            zdata_get_string(data, &str_val);
            vty_write(vty, "%-65s\"%s\"%s", path, str_val, VTY_NEWLINE(vty));
            break;
        case Binary:
            zdata_get_binary(data, &binary, &len);
            vty_write(vty, "%-65sbyte[%d] ", path, len);
            for (i = 0; i < len; i++)
                vty_write(vty, ZSTR(" 0x%hx"), i, binary[i]);

            vty_write(vty, "%s", VTY_NEWLINE(vty));
            break;
        case ArrayOfInteger:
            zdata_get_integer_array(data, &int_arr, &len);
            vty_write(vty, "%-65sint[%d] ", path, len);
            for (i = 0; i < len; i++)
                vty_write(vty, ZSTR(" [%02d] %d"), i, int_arr[i]);

            vty_write(vty, "%s", VTY_NEWLINE(vty));

            break;
        case ArrayOfFloat:
            zdata_get_float_array(data, &float_arr, &len);
            vty_write(vty, "%-65sfloat[%d] ", path, len);
            for (i = 0; i < len; i++)
                vty_write(vty, ZSTR(" [%02d] %f"), i, float_arr[i]);

            vty_write(vty, "%s", VTY_NEWLINE(vty));

            break;
        case ArrayOfString:
            zdata_get_string_array(data, &str_arr, &len);
            vty_write(vty, "%-65sstring[%d] ", path, len);
            for (i = 0; i < len; i++)
                vty_write(vty, ZSTR(" [%02d] \"%s\""), i, str_arr[i]);

            vty_write(vty, "%s", VTY_NEWLINE(vty));

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

bool    cmd_sensor_set_descriptor(vty_t* vty, variant_stack_t* params)
{
    int node_id = variant_get_int(stack_peek_at(params, 3));
    const char* descriptor = variant_get_string(stack_peek_at(params, 4));

    char file_name[256] = {0};
    snprintf(file_name, 255, "%s.xml", descriptor);
    ZWError err = zway_device_load_xml(zway, node_id, file_name);

    return (err == 0);
}

bool    cmd_sensor_remove_descriptor(vty_t* vty, variant_stack_t* params)
{
    int node_id = variant_get_int(stack_peek_at(params, 4));
    sensor_manager_remove_descriptor(node_id);
}

void    show_sensor_notification_helper(char* notification, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    vty_write(vty, " notification %s%s", notification, VTY_NEWLINE(vty));
}

void show_sensor_roles_helper(hash_node_data_t* node_data, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    vty_write(vty, " command-class %d role %s%s", node_data->key, variant_get_string(node_data->data), VTY_NEWLINE(vty));
}

void show_sensor_alarm_roles_helper(hash_node_data_t* node_data, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    vty_write(vty, " sensor-alarm %d role %s%s", node_data->key, variant_get_string(node_data->data), VTY_NEWLINE(vty));
}

void    show_sensor_descriptor_helper(sensor_descriptor_t* sensor_desc, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    vty_write(vty, "sensor descriptor node-id %d%s", sensor_desc->node_id, VTY_NEWLINE(vty));

    if(sensor_desc->multi_instance)
    {
        for(int i = 0; i < 256; i++)
        {
            if(NULL != sensor_desc->name[i])
            {
                vty_write(vty, " instance %d name %s%s", i, sensor_desc->name[i], VTY_NEWLINE(vty));
            }
        }
    }
    else if(NULL != sensor_desc->name[0])
    {
        vty_write(vty, " name %s%s", sensor_desc->name[0], VTY_NEWLINE(vty));
    }

    variant_hash_for_each(sensor_desc->sensor_roles, show_sensor_roles_helper, vty);
    variant_hash_for_each(sensor_desc->alarm_roles, show_sensor_alarm_roles_helper, vty);
    variant_hash_for_each_value(sensor_desc->supported_notification_set, char*, show_sensor_notification_helper, vty);

    vty_write(vty, "!%s", VTY_NEWLINE(vty));
}

bool    cmd_sensor_show_descriptor(vty_t* vty, variant_stack_t* params)
{
    sensor_manager_for_each(show_sensor_descriptor_helper, vty);
}

bool cmd_enter_sensor_descriptor_node(vty_t* vty, variant_stack_t* params)
{
    cli_enter_node(vty, "sensor-descriptor");
    int node_id = variant_get_int(stack_peek_at(params, 3));

    sensor_descriptor_node->context_data.int_data = node_id;

    if(NULL == sensor_manager_get_descriptor(node_id))
    {
        sensor_manager_add_descriptor(node_id);
    }
}

bool cmd_sensor_descriptor_set_role(vty_t* vty, variant_stack_t* params)
{
    int command_id = variant_get_int(stack_peek_at(params, 1));
    const char* role = variant_get_string(stack_peek_at(params, 3));

    sensor_manager_set_role(sensor_descriptor_node->context_data.int_data, command_id, role);
}

bool cmd_sensor_descriptor_set_name(vty_t* vty, variant_stack_t* params)
{
    char name[256] = {0};
    cli_assemble_line(params, 1, name, 256);

    sensor_manager_set_name(sensor_descriptor_node->context_data.int_data, name);
}

bool cmd_sensor_descriptor_set_instance_name(vty_t* vty, variant_stack_t* params)
{
    int instance_id = variant_get_int(stack_peek_at(params, 1));
    char name[256] = {0};
    cli_assemble_line(params, 3, name, 256);

    sensor_manager_set_instance_name(sensor_descriptor_node->context_data.int_data, instance_id, name);
}

bool cmd_sensor_descriptor_remove_instance_name(vty_t* vty, variant_stack_t* params)
{
    int instance_id = variant_get_int(stack_peek_at(params, 2));
    char name[256] = {0};
    cli_assemble_line(params, 4, name, 256);

    sensor_manager_remove_instance_name(sensor_descriptor_node->context_data.int_data, instance_id, name);
}

bool cmd_sensor_descriptor_set_notification(vty_t* vty, variant_stack_t* params)
{
    const char* notification = variant_get_string(stack_peek_at(params, 1));

    sensor_manager_set_notification(sensor_descriptor_node->context_data.int_data, notification);
}

bool cmd_sensor_descriptor_set_sensor_alarm(vty_t* vty, variant_stack_t* params)
{
    int alarm_id = variant_get_int(stack_peek_at(params, 1));
    const char* role = variant_get_string(stack_peek_at(params, 3));

    sensor_manager_set_alarm_role(sensor_descriptor_node->context_data.int_data, alarm_id, role);
}
