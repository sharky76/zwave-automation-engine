#include "Sensor_cli.h"
#include "Sensor_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

cli_node_t* Button_node;
bool    cmd_add_sensor(vty_t* vty, variant_stack_t* params);
bool    cmd_del_sensor(vty_t* vty, variant_stack_t* params);

cli_command_t    Button_command_list[] = {
    {"instance INT name WORD",       cmd_add_sensor,   "Add sensor instance"},
    {"no instance INT",       cmd_del_sensor,   "Delete sensor instance"},
    {NULL,  NULL,   NULL}
};

void Sensor_cli_create(cli_node_t* parent_node)
{
    cli_install_node(&Button_node, parent_node, Button_command_list, "Sensor", "vdev-Sensor");
}

char** Sensor_get_config(vty_t* vty)
{
    stack_for_each(sensor_list, sensor_entry_variant)
    {
        sensor_entry_t* entry = VARIANT_GET_PTR(sensor_entry_t, sensor_entry_variant);
        vty_write(vty, " instance %d name %s%s", 
                entry->instance,
                entry->name, VTY_NEWLINE(vty));
    }

    return NULL;
}

bool    cmd_add_sensor(vty_t* vty, variant_stack_t* params)
{
    int instance = variant_get_int(stack_peek_at(params, 1));
    const char* name = variant_get_string(stack_peek_at(params, 3));

    return sensor_add(instance, name) != NULL;
}

bool    cmd_del_sensor(vty_t* vty, variant_stack_t* params)
{
    int instance = variant_get_int(stack_peek_at(params, 2));
    return sensor_del(instance);
}