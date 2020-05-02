#include "BlinkCamera_cli.h"
#include "BlinkCamera_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

cli_node_t* BlinkCamera_node;
bool    cmd_add_camera(vty_t* vty, variant_stack_t* params);
bool    cmd_del_camera(vty_t* vty, variant_stack_t* params);

cli_command_t    BlinkCamera_command_list[] = {
    {"instance INT name WORD",       cmd_add_camera,   "Add camera instance"},
    {"no instance INT",       cmd_del_camera,   "Delete camera instance"},
    {NULL,  NULL,   NULL}
};

void BlinkCamera_cli_create(cli_node_t* parent_node)
{
    cli_install_node(&BlinkCamera_node, parent_node, BlinkCamera_command_list, "BlinkCamera", "vdev-BlinkCamera");
}

char** BlinkCamera_get_config(vty_t* vty)
{
    stack_for_each(blink_camera_list, camera_entry_variant)
    {
        blink_camera_entry_t* entry = VARIANT_GET_PTR(blink_camera_entry_t, camera_entry_variant);
        vty_write(vty, " instance %d name %s%s", 
                entry->instance,
                entry->name, VTY_NEWLINE(vty));
    }

    return NULL;
}

bool    cmd_add_camera(vty_t* vty, variant_stack_t* params)
{
    int instance = variant_get_int(stack_peek_at(params, 1));
    const char* name = variant_get_string(stack_peek_at(params, 3));

    return blink_camera_add(instance, name) != NULL;
}

bool    cmd_del_camera(vty_t* vty, variant_stack_t* params)
{
    int instance = variant_get_int(stack_peek_at(params, 2));
    return blink_camera_del(instance);
}