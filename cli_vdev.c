#include "cli_vdev.h"
#include "vdev_manager.h"
#include "vty.h"
#include <vdev.h>
#include "sensor_manager.h"

cli_node_t*     vdev_node;

extern bool    cmd_exit_node(vty_t* vty, variant_stack_t* params);
extern bool    cmd_enter_node(vty_t* vty, variant_stack_t* params);

bool cmd_enter_vdev_node(vty_t* vty, variant_stack_t* params);
bool cmd_list_vdev(vty_t* vty, variant_stack_t* params);
bool cmd_list_vdev_full(vty_t* vty, variant_stack_t* params);
bool cmd_show_vdev_methods(vty_t* vty, variant_stack_t* params);
bool cmd_show_vdev_config(vty_t* vty, variant_stack_t* params);

void show_vdev_helper(vdev_t* service, void* arg);
void show_vdev_full_helper(vdev_t* service, void* arg);

void show_vdev_method_helper(vdev_command_t* method, void* arg);
void show_vdev_config_helper(vdev_t* service, void* arg);

cli_command_t   vdev_root_list[] = {
    {"show virtual-device",                cmd_show_vdev_config,      "Show Virtual device configuration"},
    {"list virtual-device",         cmd_list_vdev_full,             "List loaded virtual devices and command classes"},
    {"list virtual-device brief",         cmd_list_vdev,             "List loaded virtual devices"},
    {"show virtual-device methods WORD",  cmd_show_vdev_methods,      "Show virtual device methods"},
    {NULL,                          NULL,                   NULL}
};

cli_node_t* vdev_parent_node;

void    cli_vdev_init(cli_node_t* parent_node)
{
    cli_append_to_node(parent_node, vdev_root_list);
    cli_install_node(&vdev_node, parent_node, NULL, "vdev", "vdev");
    vdev_parent_node = parent_node;
}

void    cli_add_vdev(const char* vdev_name)
{
    char* full_command = calloc(256, sizeof(char));
    snprintf(full_command, 255, "virtual-device %s", vdev_name);     

    cli_command_t* command_list = calloc(2, sizeof(cli_command_t));
    command_list[0].name = full_command;
    command_list[0].func=cmd_enter_vdev_node;
    command_list[0].help=strdup("Configure virtual device");

    cli_append_to_node(vdev_parent_node, command_list);
}

bool cmd_enter_vdev_node(vty_t* vty, variant_stack_t* params)
{
    const char* vdev_name = variant_get_string(stack_peek_at(params, 1));
    variant_t* vdev_param = stack_pop_front(params);
    variant_free(vdev_param);

    if(vdev_manager_is_vdev_exists(vdev_name))
    {
        cmd_enter_node(vty, params);
        //service_node->context = strdup(service_name);
    }
    else
    {
        vty_error(vty, "No such virtual device%s", VTY_NEWLINE(vty));
    }
}

bool cmd_list_vdev(vty_t* vty, variant_stack_t* params)
{
    vty_write(vty, "%-10s%-20s%s%s", "ID", "Name", "Description", VTY_NEWLINE(vty));
    vdev_manager_for_each(show_vdev_helper, vty);
}

bool cmd_list_vdev_full(vty_t* vty, variant_stack_t* params)
{
    vty_write(vty, "%-10s%-20s%-10s%-30s%-13s%s%s", "ID", "Name", "Instance", "Description", "CommandClass", "Help", VTY_NEWLINE(vty));
    vdev_manager_for_each(show_vdev_full_helper, vty);
}

bool cmd_show_vdev_methods(vty_t* vty, variant_stack_t* params)
{
    vty_write(vty, "%-20s%s%s", "Name", "Description", VTY_NEWLINE(vty));

    vdev_manager_for_each_method(variant_get_string(stack_peek_at(params, 3)), show_vdev_method_helper, vty);
}

bool cmd_show_vdev_config(vty_t* vty, variant_stack_t* params)
{
    vdev_manager_for_each(show_vdev_config_helper, vty);
}

void show_vdev_helper(vdev_t* vdev, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    vty_write(vty, "%-10d%-20s%s%s", vdev->vdev_id, vdev->name, "TODO", VTY_NEWLINE(vty));
}

void show_vdev_full_helper(vdev_t* vdev, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    // "ID", "Name", Instance", "Description", "Command Class", "Help"
    variant_stack_t* instances_list = resolver_get_instances(vdev->vdev_id);
    sensor_descriptor_t* descriptor = sensor_manager_get_descriptor(vdev->vdev_id);

    stack_for_each(instances_list, instance_variant)
    {
        device_record_t* record = VARIANT_GET_PTR(device_record_t, instance_variant);
        int instance = record->instanceId;
        
        stack_for_each(vdev->supported_method_list, vdev_command_variant)
        {
            vdev_command_t* vdev_command = VARIANT_GET_PTR(vdev_command_t, vdev_command_variant);
            if(vdev_command->is_command_class)
            {
                device_record_t* instance_record = resolver_resolve_id(vdev->vdev_id, instance, vdev_command->command_id);

                vty_write(vty, "%-10d%-20s%-10d%-30s%-13d%s%s", 
                                vdev->vdev_id, 
                                vdev->name, 
                                instance, 
                                (NULL == instance_record)? vdev->name : instance_record->deviceName, 
                                vdev_command->command_id, 
                                vdev_command->help, VTY_NEWLINE(vty));
            }
        }
    }
    
    stack_free(instances_list);
}

void show_vdev_method_helper(vdev_command_t* method, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    vty_write(vty, "%-20s%s%s", method->name, method->help, VTY_NEWLINE(vty));
}

void show_vdev_config_helper(vdev_t* vdev, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    if(NULL != vdev->get_config_callback)
    {
        vty_write(vty, "virtual-device %s%s", vdev->name, VTY_NEWLINE(vty));
        char** vdev_config = vdev->get_config_callback(vty);
        if(NULL != vdev_config)
        {
            int i = 0;
            char* cfg_string;

            while(cfg_string = vdev_config[i++])
            {
                vty_write(vty, " %s%s", cfg_string, VTY_NEWLINE(vty));
            }
        }

        vty_write(vty, "!%s", VTY_NEWLINE(vty));
    }
}

