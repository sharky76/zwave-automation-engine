#include "cli_resolver.h"
#include "vty.h"
#include "resolver.h"

cli_node_t*     resolver_node;

//extern bool    cmd_exit_node(vty_t* vty, variant_stack_t* params);

bool    cmd_list_resolver(vty_t* vty, variant_stack_t* params);
bool    cmd_add_resolver_entry(vty_t* vty, variant_stack_t* params);
bool    cmd_add_vdev_resolver_entry(vty_t* vty, variant_stack_t* params);
bool    cmd_del_resolver_entry(vty_t* vty, variant_stack_t* params);
void    show_resolver_helper(device_record_t* record, void* arg);

cli_command_t   resolver_root_list[] = {
    {"show resolver",          cmd_list_resolver,              "Show resolver configuration"},
    {"resolver WORD node-id INT instance INT command-class INT",      cmd_add_resolver_entry, "Add resolver entry"},
    {"resolver WORD vdev-id INT instance INT command-class INT",      cmd_add_vdev_resolver_entry, "Add resolver entry"}, // no need for command class!!!???
    {"no resolver WORD",   cmd_del_resolver_entry, "Delete resolver entry"},
    {NULL,                     NULL,                            NULL}
};

/*cli_command_t   resolver_command_list[] = {
    
    {"end",                        cmd_exit_node,          "Exit resolver configuration"},
    {NULL,                          NULL,                   NULL}
};*/
//cli_node_t*  resolver_node;

void    cli_resolver_init(cli_node_t* parent_node)
{
    cli_append_to_node(parent_node, resolver_root_list);
    //cli_install_node(&resolver_node, parent_node, resolver_command_list, "resolver", "resolver");
}

bool    cmd_add_resolver_entry(vty_t* vty, variant_stack_t* params)
{
    const char* device_name = variant_get_string(stack_peek_at(params, 1));

    resolver_add_entry(ZWAVE, variant_get_string(stack_peek_at(params, 1)),
                       variant_get_int(stack_peek_at(params, 3)),
                       variant_get_int(stack_peek_at(params, 5)),
                       variant_get_int(stack_peek_at(params, 7)));

    return true;
}

bool    cmd_add_vdev_resolver_entry(vty_t* vty, variant_stack_t* params)
{
    const char* device_name = variant_get_string(stack_peek_at(params, 1));

    resolver_add_entry(VDEV, variant_get_string(stack_peek_at(params, 1)),
                       variant_get_int(stack_peek_at(params, 3)),
                       variant_get_int(stack_peek_at(params, 5)),
                       variant_get_int(stack_peek_at(params, 7)));

    return true;
}

bool    cmd_del_resolver_entry(vty_t* vty, variant_stack_t* params)
{
    resolver_remove_entry(variant_get_string(stack_peek_at(params, 2)));
}

bool    cmd_list_resolver(vty_t* vty, variant_stack_t* params)
{
    resolver_for_each(show_resolver_helper, vty);
}

void    show_resolver_helper(device_record_t* record, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    if(record->devtype == ZWAVE)
    {
        vty_write(vty, "resolver %s node-id %d instance %d command-class %d%s", record->deviceName, record->nodeId, record->instanceId, record->commandId, VTY_NEWLINE(vty));
    }
    else
    {
        vty_write(vty, "resolver %s vdev-id %d instance %d command-class %d%s", record->deviceName, record->nodeId, record->instanceId, record->commandId, VTY_NEWLINE(vty));
    }
}
