#include "Button_cli.h"
#include "Button_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

cli_node_t* Button_node;
bool    cmd_add_button(vty_t* vty, variant_stack_t* params);
bool    cmd_del_button(vty_t* vty, variant_stack_t* params);

cli_command_t    Button_command_list[] = {
    {"instance INT name WORD",       cmd_add_button,   "Add button instance"},
    {"no instance INT",       cmd_del_button,   "Delete button instance"},
    {NULL,  NULL,   NULL}
};

void Button_cli_create(cli_node_t* parent_node)
{
    cli_install_node(&Button_node, parent_node, Button_command_list, "Button", "vdev-Button");
}

char** Button_get_config(vty_t* vty)
{
    stack_for_each(button_list, button_entry_variant)
    {
        button_entry_t* entry = VARIANT_GET_PTR(button_entry_t, button_entry_variant);
        vty_write(vty, " instance %d name %s%s", 
                entry->instance,
                entry->name, VTY_NEWLINE(vty));
    }

    return NULL;
}

bool    cmd_add_button(vty_t* vty, variant_stack_t* params)
{
    int instance = variant_get_int(stack_peek_at(params, 1));
    const char* name = variant_get_string(stack_peek_at(params, 3));

    return button_add(instance, name) != NULL;
}

bool    cmd_del_button(vty_t* vty, variant_stack_t* params)
{
    int instance = variant_get_int(stack_peek_at(params, 2));
    return button_del(instance);
}