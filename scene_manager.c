#include "scene_manager.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <json-c/json.h>
#include "command_parser.h"
#include "logger.h"
#include "variant_types.h"

static variant_stack_t* scene_list = NULL;

void scene_manager_init(const char* scene_dir)
{
    LOG_ADVANCED("Initializing scene manager");
    scene_list = stack_create();

    /*DIR *dp;
    struct dirent *ep;     
    dp = opendir (scene_dir);

    char full_path[256] = {0};

    if(dp != NULL)
    {
        while (ep = readdir (dp))
        {
            strcat(full_path, scene_dir);
            strcat(full_path, "/");
            strcat(full_path, ep->d_name);
            struct json_object* scene_object = json_object_from_file(full_path);

            if(NULL != scene_object)
            {
                LOG_DEBUG("Loading scene from %s", full_path);
                scene_t* scene = scene_load(scene_object);
                if(scene->is_valid)
                {
                    stack_push_back(scene_list, variant_create_ptr(DT_PTR, scene, &scene_delete));
                }
                else
                {
                    LOG_ERROR("Error loading scene %s", scene->name);
                    scene_delete(scene);
                }
            }
            memset(full_path, 0, sizeof(full_path));
        }

        closedir (dp);
        LOG_ADVANCED("Scene manager initialized with %d scenes", scene_list->count);
    }
    else
    {
        LOG_ERROR("Couldn't open the directory");
    }*/
}

void scene_manager_add_scene(const char* name)
{
    scene_t* scene = scene_create(name);
    stack_push_back(scene_list, variant_create_ptr(DT_PTR, scene, &scene_delete));
}

void scene_manager_remove_scene(const char* name)
{
    stack_for_each(scene_list, scene_variant)
    {
        scene_t* scene = (scene_t*)variant_get_ptr(scene_variant);
        if(strcmp(scene->name, name) == 0)
        {
            stack_remove(scene_list, scene_variant);
            variant_free(scene_variant);
        }
    }
}

void scene_manager_free()
{
    stack_free(scene_list);
}

#define scene_manager_foreach_scene(_source, _scene)    \
scene_t* _scene;                                        \
stack_item_t* head_item = scene_list->head;             \
for(int i = 0; i < scene_list->count; i++)              \
{                                                       \
    _scene = variant_get_ptr(head_item->data);          \
    head_item = head_item->next;                        \
    if(strcmp(_scene->source, _source) == 0)               

void scene_manager_on_event(event_t* event)
{
    const char* scene_name = NULL;
    switch(event->source->type)
    {
    case DT_SERVICE_EVENT_DATA:
        scene_name = variant_get_string(event->source);
        LOG_ADVANCED("Scene event from service: %s", scene_name);
        break;
    case DT_DEV_EVENT_DATA:
        {
            device_event_data_t* event_data = (device_event_data_t*)variant_get_ptr(event->source);
            LOG_DEBUG("Scene event from device: %s with command: 0x%x", event_data->device_name, event_data->command_id);
    
            command_class_t* command_class = get_command_class_by_id(event_data->command_id);
            if(NULL != command_class)
            {
                LOG_ADVANCED("Scene event from %s", event_data->device_name);
            }
            scene_name = event_data->device_name;
        }
    }

    if(NULL != scene_name)
    {
        scene_manager_foreach_scene(scene_name, scene)
        {
            LOG_DEBUG("Matching scene found: %s", scene->name);
            scene_exec(scene);
        }}
    }
    else
    {
        LOG_DEBUG("Scene not registered");
    }
}

scene_t*    scene_manager_get_scene(const char* name)
{
    stack_for_each(scene_list, scene_variant)
    {
        scene_t* scene = (scene_t*)variant_get_ptr(scene_variant);
        if(strcmp(scene->name, name) == 0)
        {
            return scene;
        }
    }

    return NULL;
}

void    scene_manager_for_each(void (*visitor)(scene_t*, void*), void* arg)
{
    stack_for_each(scene_list, scene_variant)
    {
        scene_t* scene = (scene_t*)variant_get_ptr(scene_variant);
        visitor(scene, arg);
    }
}
