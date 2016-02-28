#include "scene_manager.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include "command_parser.h"
#include "logger.h"
#include "variant_types.h"
#include "service_manager.h"

static variant_stack_t* scene_list = NULL;

DECLARE_LOGGER(SceneManager)

void scene_manager_init()
{
    LOG_ADVANCED(SceneManager, "Initializing scene manager");
    scene_list = stack_create();
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

#define scene_manager_foreach_source(_source, _scene)    \
scene_t* _scene;                                        \
stack_item_t* head_item = scene_list->head;             \
for(int i = 0; i < scene_list->count; i++)              \
{                                                       \
    _scene = variant_get_ptr(head_item->data);          \
    head_item = head_item->next;                        \
    if(strcmp(_scene->source, _source) == 0)               

#define scene_manager_foreach_scene(_scene_name, _scene)    \
scene_t* _scene;                                        \
stack_item_t* head_item = scene_list->head;             \
for(int i = 0; i < scene_list->count; i++)              \
{                                                       \
    _scene = variant_get_ptr(head_item->data);          \
    head_item = head_item->next;                        \
    if(strcmp(_scene->name, _scene_name) == 0)               


void scene_manager_on_event(event_t* event)
{
    switch(event->data->type)
    {
    case DT_SERVICE_EVENT_DATA:
        {
            const char* scene_name = variant_get_string(event->data);
            service_t* calling_service = service_manager_get_class_by_id(event->source_id);

            if(NULL != calling_service)
            {
                LOG_DEBUG(SceneManager, "Scene event from service: %s with data: %s", calling_service->service_name, scene_name);
                if(NULL != scene_name)
                {
                    scene_manager_foreach_scene(scene_name, scene)
                    {
                        LOG_ADVANCED(SceneManager, "Scene event from service: %s for scene: %s", calling_service->service_name, scene->name);
                        scene_exec(scene);
                    }}
                }
                else
                {
                    LOG_ERROR(SceneManager, "Scene not registered");
                }
            }
            else
            {
                LOG_ERROR(SceneManager, "Event from unknown service: %d", event->source_id);
            }
        }
        break;
    case DT_SENSOR_EVENT_DATA:
        {
            const char* scene_source = NULL;
            device_event_data_t* event_data = (device_event_data_t*)variant_get_ptr(event->data);
            LOG_DEBUG(SceneManager, "Scene event from device: %s with command: 0x%x", event_data->device_name, event_data->command_id);
    
            command_class_t* command_class = get_command_class_by_id(event_data->command_id);
            if(NULL != command_class)
            {
                LOG_ADVANCED(SceneManager, "Scene event from %s", event_data->device_name);
            }
            scene_source = event_data->device_name;

            if(NULL != scene_source)
            {
                scene_manager_foreach_source(scene_source, scene)
                {
                    LOG_DEBUG(SceneManager, "Matching scene found: %s", scene->name);
                    scene_exec(scene);
                }}
            }
            else
            {
                LOG_DEBUG(SceneManager, "Scene not registered");
            }
        }
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
