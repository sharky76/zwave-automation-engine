#include "scene_action.h"
#include "command_parser.h"
#include "scene_manager.h"
#include "script_action_handler.h"
#include "logger.h"

void scene_action_exec_scene(action_t* action);
void scene_action_exec_script(action_t* action);
void scene_action_exec_command(action_t* action);

DECLARE_LOGGER(SceneAction)

void environment_delete(void* arg)
{
    env_t* env = (env_t*)arg;
    free(env->name);
    free(env->value);
    //stack_free(env->compiled_value);
    free(env);
}

env_t*      create_environment(struct json_object* record)
{
    env_t* new_env = (env_t*)malloc(sizeof(env_t));

    json_object_object_foreach(record, key, val)
    {
        if(strcmp(key, "name") == 0)
        {
            LOG_DEBUG(SceneAction, "Name: %s", json_object_get_string(val));
            new_env->name = strdup(json_object_get_string(val));
        }
        else if(strcmp(key, "value") == 0)
        {
            LOG_DEBUG(SceneAction, "Value: %s", json_object_get_string(val));
            new_env->value = strdup(json_object_get_string(val));
        }
    }
    
    return new_env;
}

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

action_t*  scene_action_create_old(struct json_object* record)
{
    action_t* new_action = (action_t*)malloc(sizeof(action_t));
    new_action->environment = stack_create();

    json_object_object_foreach(record, key, val)
    {
        if(strcmp(key, "url") == 0)
        {
            LOG_DEBUG(SceneAction, "New_action URL: %s", json_object_get_string(val));
            new_action->url = strdup(json_object_get_string(val));

            char scheme[10] = {0};
            char path[100] = {0};

            // cut the schema part from thw path
            sscanf(new_action->url, "%9[^:]://%99[^:]", scheme, path);
            new_action->path = strdup(path);

            if(strcmp(scheme, "file") == 0)
            {
                new_action->type = A_SCRIPT;
            }
            else if(strcmp(scheme, "scene") == 0)
            {
                new_action->type = A_SCENE;
            }
            else if(strcmp(scheme, "cmd") == 0)
            {
                new_action->type = A_COMMAND;
            }
        }
        else if(strcmp(key, "environment") == 0)
        {
            int num_environment = json_object_array_length(val);

            for(int i = 0; i < num_environment; i++)
            {
                struct json_object* record = json_object_array_get_idx(val, i);
                env_t* new_env = create_environment(record);
                stack_push_back(new_action->environment, variant_create_ptr(DT_PTR, new_env, &environment_delete));
            }
        }
    }

    return new_action;
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

void scene_action_exec_command(action_t* action)
{
    bool isOk;
    variant_stack_t* compiled = command_parser_compile_expression(action->path, &isOk);

    if(isOk)
    {
        command_parser_execute_expression(compiled);
    }
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
            LOG_ERROR(SceneAction, "Error compiling environment value: %s", env->value);
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

