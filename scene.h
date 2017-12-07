#pragma once

// This file holds the scene instance loaded from the JSON file
//#include <json-c/json.h>
#include <ZPlatform.h>
#include "stack.h"
#include "resolver.h"
#include "vty.h"
#include "scene_action.h"

typedef struct action_t action_t;

typedef struct scene_t
{
    char*               name;
    //char*               source;
    variant_stack_t*    source_list;
    variant_stack_t*    condition_list;
    variant_stack_t*    actions;
    bool                is_valid;
    bool                is_enabled;
    int                 command_id;
} scene_t;

scene_t*    scene_create(const char* name);
bool        scene_add_action(scene_t* scene, action_t* action);
void        scene_del_action(scene_t* scene, action_t* action);
action_t*   scene_get_action(scene_t* scene, const char* action_path);
void        scene_add_source(scene_t* scene, const char* source);
void        scene_del_source(scene_t* scene, const char* source);

action_t*   scene_get_action_with_type(scene_t* scene, const char* action_path, ActionType type);

//scene_t*    scene_load(struct json_object* scene_obj);
void        scene_exec(scene_t* scene);
void        scene_delete(void* arg);
void        scene_for_each_action(scene_t* scene, void (*visitor)(action_t*, void*), void* arg);

