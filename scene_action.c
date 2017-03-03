#include "scene_action.h"
#include "command_parser.h"
#include "scene_manager.h"
#include "script_action_handler.h"
#include "builtin_service.h"
#include "service_args.h"
#include <logger.h>
#include <hash.h>
#include <crc32.h>

void scene_action_exec_scene(action_t* action);
void scene_action_exec_script(action_t* action);
void scene_action_exec_command(action_t* action);
void scene_action_enable_scene(action_t* action);
void scene_action_disable_scene(action_t* action);

USING_LOGGER(Scene)

void environment_delete(void* arg)
{
    env_t* env = (env_t*)arg;
    free(env->name);
    free(env->value);
    //stack_free(env->compiled_value);
    free(env);
}

void method_stack_delete(void* arg)
{
    method_stack_item_t* item = (method_stack_item_t*)arg;

    free(item->stack_name);
    free(item->name);
    free(item->value);
    free(item);
}

/*env_t*      create_environment(struct json_object* record)
{
    env_t* new_env = (env_t*)malloc(sizeof(env_t));

    json_object_object_foreach(record, key, val)
    {
        if(strcmp(key, "name") == 0)
        {
            LOG_DEBUG(Scene, "Name: %s", json_object_get_string(val));
            new_env->name = strdup(json_object_get_string(val));
        }
        else if(strcmp(key, "value") == 0)
        {
            LOG_DEBUG(Scene, "Value: %s", json_object_get_string(val));
            new_env->value = strdup(json_object_get_string(val));
        }
    }
    
    return new_env;
}*/

action_t*  scene_action_create(ActionType type, const char* record)
{
    action_t* new_action = (action_t*)calloc(1, sizeof(action_t));
    new_action->environment = stack_create();
    new_action->path = strdup(record);
    new_action->type = type;
    return new_action;
}

void       scene_action_add_environment(action_t* action, const char* name, const char* value)
{
    stack_for_each(action->environment, env_variant)
    {
        env_t* env = (env_t*)variant_get_ptr(env_variant);
        if(strcmp(env->name, name) == 0)
        {
            stack_remove(action->environment, env_variant);
            break;
        }
    }

    env_t* new_env = (env_t*)malloc(sizeof(env_t));
    new_env->name = strdup(name);
    new_env->value = strdup(value);
    stack_push_back(action->environment, variant_create_ptr(DT_PTR, new_env, &environment_delete));
}

void       scene_action_del_environment(action_t* action, const char* name)
{
    stack_for_each(action->environment, env_variant)
    {
        env_t* env = (env_t*)variant_get_ptr(env_variant);
        if(strcmp(env->name, name) == 0)
        {
            stack_remove(action->environment, env_variant);
            variant_free(env_variant);
            break;
        }
    }
}

void       scene_action_add_method_stack_item(action_t* action, const char* stack_name, const char* name, const char* value)
{
    stack_for_each(action->environment, env_variant)
    {
        method_stack_item_t* env = (method_stack_item_t*)variant_get_ptr(env_variant);
        if(strcmp(env->name, name) == 0 && strcmp(env->stack_name, stack_name) == 0)
        {
            stack_remove(action->environment, env_variant);
            break;
        }
    }

    method_stack_item_t* new_env = (method_stack_item_t*)malloc(sizeof(method_stack_item_t));
    new_env->stack_name = strdup(stack_name);
    new_env->name = strdup(name);
    new_env->value = strdup(value);
    stack_push_back(action->environment, variant_create_ptr(DT_PTR, new_env, &method_stack_delete));
}

void       scene_action_del_method_stack_item(action_t* action, const char* stack_name, const char* name)
{
    stack_for_each(action->environment, env_variant)
    {
        method_stack_item_t* env = (method_stack_item_t*)variant_get_ptr(env_variant);
        if(strcmp(env->name, name) == 0 && strcmp(env->stack_name, stack_name) == 0)
        {
            stack_remove(action->environment, env_variant);
            variant_free(env_variant);
            break;
        }
    }
}

void  scene_action_exec(action_t* action)
{
    switch(action->type)
    {
    case A_SCRIPT:
        scene_action_exec_script(action);
        break;
    case A_SCENE:
        scene_action_exec_scene(action);
        break;
    case A_COMMAND:
        scene_action_exec_command(action);
        break;
    case A_ENABLE:
        scene_action_enable_scene(action);
        break;
    case A_DISABLE:
        scene_action_disable_scene(action);
        break;
    default:
        break;
    }
}

void scene_action_delete(void* arg)
{
    action_t* action = (action_t*)arg;
    free(action->url);
    free(action->path);
    stack_free(action->environment);
    free(action);
}

void scene_action_exec_scene(action_t* action)
{
    scene_t* scene = scene_manager_get_scene(action->path);

    if(NULL != scene)
    {
        scene_exec(scene);
    }
}

void scene_action_enable_scene(action_t* action)
{
    scene_t* scene = scene_manager_get_scene(action->path);

    if(NULL != scene)
    {
        scene->is_enabled = true;
    }
}

void scene_action_disable_scene(action_t* action)
{
    scene_t* scene = scene_manager_get_scene(action->path);

    if(NULL != scene)
    {
        scene->is_enabled = false;
    }
}

void scene_action_exec_command(action_t* action)
{
    bool isOk;
    variant_stack_t* compiled = command_parser_compile_expression(action->path, &isOk);

    if(isOk)
    {
        if(action->environment->count > 0)
        {
            // Compile all environment and prepare token table
            //hash_table_t*   token_table = variant_hash_init();
            stack_for_each(action->environment, env_variant)
            {
                method_stack_item_t* env = (method_stack_item_t*)variant_get_ptr(env_variant);
                service_args_stack_create(env->stack_name);

                bool isOk;
                variant_stack_t* compiled_value = command_parser_compile_expression(env->value, &isOk);
        
                if(!isOk)
                {
                    LOG_ERROR(Scene, "Error compiling environment value: %s", env->value);
                }
                else
                {
                    variant_t* env_value = command_parser_execute_expression(compiled_value);
                    
                    if(NULL != env_value)
                    {
                        uint32_t key = crc32(0, env->name, strlen(env->name));
                        //printf("Inserting value with name %s and key %u, and val: %s\n", env->name, key, variant_get_string(env_value));
                        service_args_stack_add(env->stack_name, key, env_value);
                    }
                }

                stack_free(compiled_value);
            }
    
            //builtin_service_stack_create("Expression.ProcessTemplate");
            //builtin_service_stack_add("Expression.ProcessTemplate", variant_create_ptr(DT_PTR, token_table, variant_hash_free_void));
        }

        variant_t* result = command_parser_execute_expression(compiled);

        if(NULL != result)
        {
            variant_free(result);
        }

        service_args_stack_clear();
    }

    stack_free(compiled);
}

// Fork and exec provided script
void scene_action_exec_script(action_t* action)
{
    // First lets build an environment
    char** envp;

    envp = (char**)malloc(sizeof(char*)*(action->environment->count + 1));
    char** anchor = envp;

    stack_for_each(action->environment, env_variant)
    {
        env_t* env = (env_t*)variant_get_ptr(env_variant);
        bool isOk;
        variant_stack_t* compiled_value = command_parser_compile_expression(env->value, &isOk);

        if(!isOk)
        {
            LOG_ERROR(Scene, "Error compiling environment value: %s", env->value);
        }
        else
        {
            variant_t* env_value = command_parser_execute_expression(compiled_value);
            
            if(NULL != env_value)
            {
                char* value_str;
                if(variant_to_string(env_value, &value_str))
                {
                    *envp = (char*)calloc(strlen(env->name)+strlen(value_str)+2, sizeof(char));
                    strcat(*envp, env->name);
                    strcat(*envp, "=");
                    strcat(*envp, value_str);
                    free(value_str);
                }
        
                variant_free(env_value);
                envp++;
            }
        }

        stack_free(compiled_value);
    }

    // Add NULL value
    *envp = NULL;

    // Now our environment is ready, lets fork and exec
    script_exec(action->path, anchor);
}

void    scene_action_for_each_environment(action_t* action, void (*visitor)(env_t*, void*), void* arg)
{
    stack_for_each(action->environment, env_variant)
    {
        env_t* env = (env_t*)variant_get_ptr(env_variant);
        visitor(env, arg);
    }
}

void    scene_action_for_each_method_stack_item(action_t* action, void (*visitor)(method_stack_item_t*, void*), void* arg)
{
    stack_for_each(action->environment, env_variant)
    {
        method_stack_item_t* env = (method_stack_item_t*)variant_get_ptr(env_variant);
        visitor(env, arg);
    }
}

