#include "cli_scene.h"
#include "scene_manager.h"
#include "scene_action.h"

//extern cli_command_t   scene_command_list[];
cli_node_t*     scene_node;

//extern cli_command_t   scene_action_command_list[];
cli_node_t*     scene_action_node;


extern bool    cmd_exit_node(vty_t* vty, variant_stack_t* params);
extern bool    cmd_enter_node(vty_t* vty, variant_stack_t* params);

bool cmd_enter_scene_node(vty_t* vty, variant_stack_t* params);
bool cmd_del_scene(vty_t* vty, variant_stack_t* params);
bool cmd_list_scenes(vty_t* vty, variant_stack_t* params);
bool cmd_show_scene(vty_t* vty, variant_stack_t* params);
bool cmd_set_scene_source(vty_t* vty, variant_stack_t* params);
bool cmd_set_scene_condition(vty_t* vty, variant_stack_t* params);
bool cmd_exit_scene_node(vty_t* vty, variant_stack_t* params);

bool cmd_config_scene_action_script(vty_t* vty, variant_stack_t* params);
bool cmd_config_scene_action_scene(vty_t* vty, variant_stack_t* params);
bool cmd_config_scene_action_command(vty_t* vty, variant_stack_t* params);
bool cmd_add_action_environment(vty_t* vty, variant_stack_t* params);

bool cmd_delete_scene_action_script(vty_t* vty, variant_stack_t* params);
bool cmd_delete_scene_action_scene(vty_t* vty, variant_stack_t* params);
bool cmd_delete_scene_action_command(vty_t* vty, variant_stack_t* params);
bool cmd_delete_action_environment(vty_t* vty, variant_stack_t* params);

bool cmd_scene_show_action(vty_t* vty, variant_stack_t* params);
bool cmd_scene_show_condition(vty_t* vty, variant_stack_t* params);

void    show_scene_helper(scene_t* scene, void* arg);
void    show_scene_action_helper(action_t* action, void* arg);
void    show_scene_action_env_helper(env_t* env, void* arg);

cli_command_t   scene_root_list[] = {
    {"scene LINE",           cmd_enter_scene_node,          "Configure scene"},
    {"no scene LINE",        cmd_del_scene,                 "Delete scene"},
    {"show scene",           cmd_list_scenes,               "List loaded scenes"},
    {"show scene LINE",      cmd_show_scene,                "Show scene configuration"},
    {NULL,                          NULL,                   NULL}
};

cli_command_t   scene_command_list[] = {
    {"source WORD",             cmd_set_scene_source,      "Set scene source"},
    {"condition LINE",          cmd_set_scene_condition,   "Set scene condition"},
    {"action script WORD",      cmd_config_scene_action_script,   "Configure scene action script"},
    {"no action script WORD",   cmd_delete_scene_action_script,   "Delete scene action script"},
    {"action scene LINE",       cmd_config_scene_action_scene,    "Configure scene action scene"},
    {"no action scene LINE",    cmd_delete_scene_action_scene,    "Delete scene action scene"},
    {"action command LINE",     cmd_config_scene_action_command,    "Configure scene action command"},
    {"no action command LINE",  cmd_delete_scene_action_command,    "Delete scene action command"},
    {"show action",             cmd_scene_show_action,              "Show scene actions"},
    {"show condition",          cmd_scene_show_condition,           "Show scene condition"},
    //{"end",                     cmd_exit_scene_node,             "Exit scene configuration"},
    {NULL,                          NULL,                   NULL}
};
//cli_node_t* scene_node;

cli_command_t   scene_action_command_list[] = {
    {"environment WORD value LINE",     cmd_add_action_environment,     "Add environment to script action"},
    {"no environment WORD",             cmd_delete_action_environment,  "Remove environment from script action"},
    //{"end",                             cmd_exit_node,                  "Exit environment configuration"},
    {NULL,                          NULL,                       NULL}
};
//cli_node_t* scene_action_node;

void    cli_scene_init(cli_node_t* parent_node)
{
    cli_append_to_node(parent_node, scene_root_list);
    cli_install_node(&scene_node, parent_node, scene_command_list, "scene", "scene");
    cli_install_node(&scene_action_node, scene_node, scene_action_command_list, "action", "action");
}

bool cmd_enter_scene_node(vty_t* vty, variant_stack_t* params)
{
    cmd_enter_node(vty, params);
    char scene_name[256] = {0};
    cli_assemble_line(params, 1, scene_name);
    scene_node->context = strdup(scene_name);
    if(NULL == scene_manager_get_scene(scene_node->context))
    {
        scene_manager_add_scene(scene_node->context);
    }
}

bool cmd_exit_scene_node(vty_t* vty, variant_stack_t* params)
{
    free(scene_node->context);
    cmd_exit_node(vty, params);
}

bool cmd_del_scene(vty_t* vty, variant_stack_t* params)
{
    char scene_name[256] = {0};
    cli_assemble_line(params, 2, scene_name);
    if(NULL != scene_manager_get_scene(scene_name))
    {
        scene_manager_remove_scene(scene_name);
    }
}

bool cmd_list_scenes(vty_t* vty, variant_stack_t* params)
{
    scene_manager_for_each(show_scene_helper, vty);
}

bool cmd_show_scene(vty_t* vty, variant_stack_t* params)
{
    char scene_name[256] = {0};
    cli_assemble_line(params, 2, scene_name);
    scene_t* scene = scene_manager_get_scene(scene_name);

    if(NULL != scene)
    {
        show_scene_helper(scene, vty);
    }
}

bool cmd_set_scene_source(vty_t* vty, variant_stack_t* params)
{
    scene_t* scene = scene_manager_get_scene(scene_node->context);
    scene->source = strdup(variant_get_string(stack_peek_at(params, 1)));
}

bool cmd_set_scene_condition(vty_t* vty, variant_stack_t* params)
{
    scene_t* scene = scene_manager_get_scene(scene_node->context);

    char condition[1024] = {0};
    cli_assemble_line(params, 1, condition);
    scene->condition = strdup(condition);
}
 
bool cmd_config_scene_action_script(vty_t* vty, variant_stack_t* params)
{
    cmd_enter_node(vty, params);

    scene_t* scene = scene_manager_get_scene(scene_node->context);
    action_t* new_action = scene_get_action(scene, variant_get_string(stack_peek_at(params, 2)));
    if(NULL == new_action)
    {
        new_action = scene_action_create(A_SCRIPT, variant_get_string(stack_peek_at(params, 2)));
        if(!scene_add_action(scene, new_action))
        {
            vty_error(vty, "Unable to add action\n");
        }
    }

    scene_action_node->context = strdup(new_action->path);
}

bool cmd_config_scene_action_scene(vty_t* vty, variant_stack_t* params)
{
    char scene_name[512] = {0};
    cli_assemble_line(params, 2, scene_name);

    scene_t* scene = scene_manager_get_scene(scene_node->context);
    action_t* new_action = scene_get_action(scene, scene_name);
    if(NULL == new_action)
    {
        new_action = scene_action_create(A_SCENE, scene_name);
        scene_add_action(scene, new_action);
    }
}

bool cmd_config_scene_action_command(vty_t* vty, variant_stack_t* params)
{
    char command[1024] = {0};
    cli_assemble_line(params, 2, command);
    scene_t* scene = scene_manager_get_scene(scene_node->context);

    action_t* new_action = scene_get_action(scene, command);
    if(NULL == new_action)
    {
        new_action = scene_action_create(A_COMMAND, command);
        scene_add_action(scene, new_action);
    }
}

bool cmd_add_action_environment(vty_t* vty, variant_stack_t* params)
{
    char command[1024] = {0};
    cli_assemble_line(params, 3, command);
    scene_t* scene = scene_manager_get_scene(scene_node->context);
    action_t* action = scene_get_action(scene, scene_action_node->context);
    scene_action_add_environment(action,
                                 variant_get_string(stack_peek_at(params, 1)),
                                 command);
}

bool cmd_delete_scene_action_script(vty_t* vty, variant_stack_t* params)
{
    scene_t* scene = scene_manager_get_scene(scene_node->context);
    action_t* action = scene_get_action(scene, variant_get_string(stack_peek_at(params, 3)));

    if(NULL != action && action->type == A_SCRIPT)
    {
        scene_del_action(scene, action);
    }
}

bool cmd_delete_scene_action_scene(vty_t* vty, variant_stack_t* params)
{
    char scene_name[512] = {0};
    cli_assemble_line(params, 3, scene_name);

    scene_t* scene = scene_manager_get_scene(scene_node->context);
    action_t* action = scene_get_action(scene, scene_name);

    if(NULL != action && action->type == A_SCENE)
    {
        scene_del_action(scene, action);
    }
}

bool cmd_delete_scene_action_command(vty_t* vty, variant_stack_t* params)
{
    char command[1024] = {0};
    cli_assemble_line(params, 3, command);

    scene_t* scene = scene_manager_get_scene(scene_node->context);
    action_t* action = scene_get_action(scene, command);

    if(NULL != action && action->type == A_COMMAND)
    {
        scene_del_action(scene, action);
    }
}

bool cmd_delete_action_environment(vty_t* vty, variant_stack_t* params)
{
    action_t* action = (action_t*)scene_action_node->context;
    scene_action_del_environment(action, variant_get_string(stack_peek_at(params, 2)));
}

bool cmd_scene_show_action(vty_t* vty, variant_stack_t* params)
{
    scene_t* scene = scene_manager_get_scene(scene_node->context);
    scene_manager_for_each_action(scene, show_scene_action_helper, vty);
}

bool cmd_scene_show_condition(vty_t* vty, variant_stack_t* params)
{
    scene_t* scene = scene_manager_get_scene(scene_node->context);
    vty_write(vty, "condition %s\n", scene->condition);
}

void    show_scene_helper(scene_t* scene, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    vty_write(vty, "scene %s\n", scene->name);
    vty_write(vty, " source %s\n", scene->source);
    vty_write(vty, " condition %s\n", scene->condition);
    
    scene_manager_for_each_action(scene, show_scene_action_helper, vty);

    vty_write(vty, "!\n");
}

void    show_scene_action_helper(action_t* action, void* arg)
{
    vty_t* vty = (vty_t*)arg;

    switch(action->type)
    {
    case A_SCRIPT:
        vty_write(vty, " action script %s\n", action->path);
        scene_action_for_each_environment(action, show_scene_action_env_helper, vty);
        vty_write(vty, " !\n");
        break;
    case A_COMMAND:
        vty_write(vty, " action command %s\n", action->path);
        break;
    case A_SCENE:
        vty_write(vty, " action scene %s\n", action->path);
        break;
    }

}

void    show_scene_action_env_helper(env_t* env, void* arg)
{
    vty_t* vty = (vty_t*)arg;
    vty_write(vty, "  environment %s value %s\n", env->name, env->value);
}
