#pragma once

/*
This is scene action file - handles execution of different scene actions
*/

#include "resolver.h"
#include "stack.h"
#include <json-c/json.h>

typedef struct env_t
{
    char*     name;
    char*     value;
    variant_stack_t*  compiled_value;
} env_t;

typedef enum ActionType_e
{
    A_SCRIPT,
    A_SCENE,
    A_COMMAND,
    A_ENABLE,
    A_DISABLE
} ActionType;

typedef struct action_t
{
    ActionType type;
    char*       url;
    char*       path;
    variant_stack_t*    environment;
} action_t;

action_t*  scene_action_create(ActionType type, const char* record);
void       scene_action_add_environment(action_t* action, const char* name, const char* value);
void       scene_action_del_environment(action_t* action, const char* name);
action_t*  scene_action_create_old(struct json_object* record);
void       scene_action_exec(action_t* action);
void       scene_action_delete(void* arg);
void       scene_action_for_each_environment(action_t* action, void (*visitor)(env_t*, void*), void* arg);

