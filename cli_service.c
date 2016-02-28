#include "cli_service.h"
#include "service_manager.h"
#include "vty.h"

cli_node_t*     service_node;

extern bool    cmd_exit_node(vty_t* vty, variant_stack_t* params);
extern bool    cmd_enter_node(vty_t* vty, variant_stack_t* params);

bool cmd_enter_service_node(vty_t* vty, variant_stack_t* params);
bool cmd_list_service_classes(vty_t* vty, variant_stack_t* params);
bool cmd_show_service_methods(vty_t* vty, variant_stack_t* params);
bool cmd_show_service_config(vty_t* vty, variant_stack_t* params);

void show_service_helper(service_t* service, void* arg);
void show_service_method_helper(service_method_t* method, void* arg);
void show_service_config_helper(service_t* service, void* arg);

cli_command_t   service_root_list[] = {
    //{"service WORD",         cmd_enter_service_node,        "Configure service"},
    {"show service",                cmd_show_service_config,      "Show service configuration"},
    {"show service classes",         cmd_list_service_classes,             "List loaded services"},
    {"show service methods WORD",  cmd_show_service_methods,      "Show service methods"},
    {NULL,                          NULL,                   NULL}
};

cli_node_t* service_parent_node;

void    cli_service_init(cli_node_t* parent_node)
{
    cli_append_to_node(parent_node, service_root_list);
    cli_install_node(&service_node, parent_node, NULL, "service", "service");
    service_parent_node = parent_node;
}

void    cli_add_service(const char* service_name)
{
    char* full_command = calloc(256, sizeof(char));
    snprintf(full_command, 255, "service %s", service_name);     

    cli_command_t* command_list = calloc(2, sizeof(cli_command_t));
    command_list[0].name = full_command;
    command_list[0].func=cmd_enter_service_node;
    command_list[0].help=strdup("Configure service");

    cli_append_to_node(service_parent_node, command_list);
}

bool cmd_enter_service_node(vty_t* vty, variant_stack_t* params)
{
    const char* service_name = variant_get_string(stack_peek_at(params, 1));
    variant_t* service_param = stack_pop_front(params);
    variant_free(service_param);

    if(service_manager_is_class_exists(service_name))
    {
        cmd_enter_node(vty, params);
        service_node->context = strdup(service_name);
    }
    else
    {
        vty_error(vty, "No such service\n");
    }
}

bool cmd_list_service_classes(vty_t* vty, variant_stack_t* params)
{
    vty_write(vty, "%-20s%s\n", "Name", "Description");
    service_manager_for_each_class(show_service_helper, vty);
}

bool cmd_show_service_methods(vty_t* vty, variant_stack_t* params)
{
    vty_write(vty, "%-20s%s\n", "Name", "Description");
    service_manager_for_each_method(variant_get_string(stack_peek_at(params, 3)), show_service_method_helper, vty);
}

bool cmd_show_service_config(vty_t* vty, variant_stack_t* params)
{
    service_manager_for_each_class(show_service_config_helper, vty);
}

void show_service_helper(service_t* service, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    vty_write(vty, "%-20s%s\n", service->service_name, service->description);
}

void show_service_method_helper(service_method_t* method, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    vty_write(vty, "%-20s%s\n", method->name, method->help);
}

void show_service_config_helper(service_t* service, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    if(NULL != service->get_config_callback)
    {
        vty_write(vty, "service %s\n", service->service_name);
        char** service_config = service->get_config_callback();
        int i = 0;
        char* cfg_string;

        while(cfg_string = service_config[i++])
        {
            vty_write(vty, " %s\n", cfg_string);
        }

        vty_write(vty, "!\n");
    }
}
