#include "scene_manager.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include "command_parser.h"
#include "logger.h"
#include "variant_types.h"
#include "service_manager.h"
#include <event.h>
#include <hash.h>
#include <crc32.h>
#include "vdev_manager.h"
#include "service_args.h"

//static variant_stack_t* scene_list;
hash_table_t*   scene_table;

USING_LOGGER(Scene)

void    scene_manager_on_vdev_event(event_pump_t* pump, int event_id, void* data, void* context);
void    scene_manager_on_sensor_event(event_pump_t* pump, int event_id, void* data, void* context);
void    scene_manager_on_scene_activation_event(event_pump_t* pump, int event_id, void* data, void* context);

void scene_manager_init()
{
    LOG_ADVANCED(Scene, "Initializing scene manager");
    //scene_list = stack_create();
    scene_table = variant_hash_init();

    event_pump_t* pump = event_dispatcher_get_pump("EVENT_PUMP");
    event_dispatcher_register_handler(pump, VdevDataChangeEvent, scene_manager_on_vdev_event, NULL);
    event_dispatcher_register_handler(pump, SensorDataChangeEvent, scene_manager_on_sensor_event, NULL);
    event_dispatcher_register_handler(pump, SceneActivationEvent, scene_manager_on_scene_activation_event, NULL);

}

void scene_manager_add_scene(const char* name)
{
    scene_t* scene = scene_create(name);
    //stack_push_back(scene_list, variant_create_ptr(DT_PTR, scene, &scene_delete));

    uint32_t key = crc32(0, name, strlen(name));
    variant_hash_insert(scene_table, key, variant_create_ptr(DT_PTR, scene, &scene_delete));
}

void scene_manager_remove_scene(const char* name)
{
    uint32_t key = crc32(0, name, strlen(name));
    variant_hash_remove(scene_table, key);

    /*stack_for_each(scene_list, scene_variant)
    {
        scene_t* scene = (scene_t*)variant_get_ptr(scene_variant);
        if(strcmp(scene->name, name) == 0)
        {
            stack_remove(scene_list, scene_variant);
            variant_free(scene_variant);
        }
    }*/
}

void scene_manager_free()
{
    //stack_free(scene_list);
    variant_hash_free(scene_table);
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

void    scene_manager_on_vdev_event(event_pump_t* pump, int event_id, void* data, void* context)
{
    vdev_event_data_t* event_data = (vdev_event_data_t*)data;
    vdev_t* vdev = vdev_manager_get_vdev_by_id(event_data->vdev_id);

    if(NULL != vdev)
    {
        LOG_DEBUG(Scene, "Scene event from virtual device: %s vdev-id %d, instance-id %d, with command: 0x%x", vdev->name, event_data->vdev_id, event_data->instance_id, event_data->event_id);

        //command_class_t* command_class = vdev_manager_get_command_class_by_id(vdev->vdev_id, event_data->command_id);
        const char* vdev_resolved_name = resolver_name_from_id(event_data->vdev_id, event_data->instance_id, event_data->event_id);
        if(NULL != vdev_resolved_name)
        {
            LOG_DEBUG(Scene, "Matching command-class %s found for virtual device %s", vdev_resolved_name, vdev->name);
            
            // Create SensorEvent argument stack
            service_args_stack_create("SensorEvent");
            service_args_stack_add("SensorEvent", "EventData", variant_create_ptr(DT_PTR, event_data, &variant_delete_none));

            hash_iterator_t* it = variant_hash_begin(scene_table);

            while(!variant_hash_iterator_is_end(variant_hash_iterator_next(it)))
            {
                scene_t* scene = (scene_t*)variant_get_ptr(variant_hash_iterator_value(it));
                //if(NULL != scene->source && strcmp(scene->source, vdev_resolved_name) == 0)
                //{
                stack_for_each(scene->source_list, source_variant)
                {
                    const char* source = variant_get_string(source_variant);
                    if(strcmp(source, vdev_resolved_name) == 0 || strcmp(source, "VDEV") == 0)
                    {
                        LOG_ADVANCED(Scene, "Scene event from virtual device %s for scene %s", vdev->name, scene->name);
                        scene_exec(scene);
                    }
                }
            }

            free(it);
            service_args_stack_destroy("SensorEvent");
        }
        else
        {
            LOG_DEBUG(Scene, "Scene not registered");
        }

    }
}

void    scene_manager_on_sensor_event(event_pump_t* pump, int event_id, void* data, void* context)
{
    const char* scene_source = NULL;
    sensor_event_data_t* event_data = (sensor_event_data_t*)data;
    LOG_DEBUG(Scene, "Scene event from sensor: %s with command: 0x%x", event_data->device_name, event_data->command_id);

    command_class_t* command_class = get_command_class_by_id(event_data->command_id);
    if(NULL != command_class)
    {
        LOG_DEBUG(Scene, "Matching command-class %s found for sensor %s", command_class->command_name, event_data->device_name);

    }

    scene_source = resolver_name_from_id(event_data->node_id, event_data->instance_id, event_data->command_id);
    //scene_source = event_data->device_name;

    if(NULL != scene_source)
    {
        // Create SensorEvent argument stack
        service_args_stack_create("SensorEvent");
        service_args_stack_add("SensorEvent", "EventData", variant_create_ptr(DT_PTR, event_data, variant_delete_none));

        hash_iterator_t* it = variant_hash_begin(scene_table);

        while(!variant_hash_iterator_is_end(variant_hash_iterator_next(it)))
        {
            scene_t* scene = (scene_t*)variant_get_ptr(variant_hash_iterator_value(it));

            // Check all scene sources...
            stack_for_each(scene->source_list, source_variant)
            {
                const char* source = variant_get_string(source_variant);
                if(strcmp(source, scene_source) == 0 || strcmp(source, "Sensor") == 0)
                {
                    LOG_ADVANCED(Scene, "Scene event from sensor %s for scene %s", event_data->device_name, scene->name);
                    scene_exec(scene);
                }
            }
        }

        free(it);
        service_args_stack_destroy("SensorEvent");
    }
    else
    {
        LOG_DEBUG(Scene, "Scene not registered");
    }
}

void    scene_manager_on_scene_activation_event(event_pump_t* pump, int event_id, void* data, void* context)
{
    event_t* event_data = (event_t*)data;
    const char* scene_name = (const char*)event_data->data;
    service_t* calling_service = service_manager_get_class_by_id(event_data->source_id);

    if(NULL != calling_service)
    {
        LOG_DEBUG(Scene, "Scene event from service: %s with data: %s", calling_service->service_name, scene_name);

        if(NULL != scene_name)
        {
            uint32_t key = crc32(0, scene_name, strlen(scene_name));
            variant_t* scene_variant = variant_hash_get(scene_table, key);
            /*scene_manager_foreach_scene(scene_name, scene)
            {
                LOG_ADVANCED(Scene, "Scene event from service: %s for scene: %s", calling_service->service_name, scene->name);
                scene_exec(scene);
            }}*/
            if(NULL != scene_variant)
            {
                scene_t* scene = (scene_t*)variant_get_ptr(scene_variant);
                //if(strcmp(scene->source, calling_service->service_name) == 0)
                //{
                bool scene_called = false;
                stack_for_each(scene->source_list, source_variant)
                {
                    const char* source = variant_get_string(source_variant);
                    if(strcmp(source, calling_service->service_name) == 0)
                    {
                        LOG_ADVANCED(Scene, "Scene event from service: %s for scene: %s", calling_service->service_name, scene->name);
                        scene_called = true;
                        scene_exec(scene);
                    }
                }

                if(!scene_called)
                {
                    LOG_ERROR(Scene, "Scene %s source mismatch: %s", scene->name, calling_service->service_name);
                }
            }
            else
            {
                LOG_ERROR(Scene, "Scene not registered: %s", scene_name);
            }
        }
        else
        {
            LOG_ERROR(Scene, "Scene not registered");
        }
    }
    else
    {
        LOG_ERROR(Scene, "Event from unknown service: %d", event_data->source_id);
    }
}

#if 0
void scene_manager_on_event(event_t* event)
{
    switch(event->data->type)
    {
    case DT_SERVICE_EVENT_DATA:
        {
            service_event_data_t* event_data = variant_get_ptr(event->data);
            const char* scene_name = event_data->data;
            service_t* calling_service = service_manager_get_class_by_id(event->source_id);

            if(NULL != calling_service)
            {
                if(strcmp(scene_name, "tick") != 0)
                {
                    LOG_DEBUG(Scene, "Scene event from service: %s with data: %s", calling_service->service_name, scene_name);
                }
                else
                {
                    break;
                }

                if(NULL != scene_name)
                {
                    uint32_t key = crc32(0, scene_name, strlen(scene_name));
                    variant_t* scene_variant = variant_hash_get(scene_table, key);
                    /*scene_manager_foreach_scene(scene_name, scene)
                    {
                        LOG_ADVANCED(Scene, "Scene event from service: %s for scene: %s", calling_service->service_name, scene->name);
                        scene_exec(scene);
                    }}*/
                    if(NULL != scene_variant)
                    {
                        scene_t* scene = (scene_t*)variant_get_ptr(scene_variant);
                        LOG_ADVANCED(Scene, "Scene event from service: %s for scene: %s", calling_service->service_name, scene->name);
                        scene_exec(scene);
                    }
                    else
                    {
                        LOG_ERROR(Scene, "Scene not registered: %s", scene_name);
                    }
                }
                else
                {
                    LOG_ERROR(Scene, "Scene not registered");
                }
            }
            else
            {
                LOG_ERROR(Scene, "Event from unknown service: %d", event->source_id);
            }
        }
        break;
    case DT_SENSOR_EVENT_DATA:
        {
            const char* scene_source = NULL;
            sensor_event_data_t* event_data = (sensor_event_data_t*)variant_get_ptr(event->data);
            LOG_DEBUG(Scene, "Scene event from sensor: %s with command: 0x%x", event_data->device_name, event_data->command_id);
    
            command_class_t* command_class = get_command_class_by_id(event_data->command_id);
            if(NULL != command_class)
            {
                LOG_DEBUG(Scene, "Matching command-class %s found for sensor %s", command_class->command_name, event_data->device_name);

            }
            scene_source = event_data->device_name;

            if(NULL != scene_source)
            {
                hash_iterator_t* it = variant_hash_begin(scene_table);
    
                while(!variant_hash_iterator_is_end(variant_hash_iterator_next(it)))
                {
                    scene_t* scene = (scene_t*)variant_get_ptr(variant_hash_iterator_value(it));
                    if(NULL != scene->source && strcmp(scene->source, scene_source) == 0)
                    {
                        LOG_ADVANCED(Scene, "Scene event from sensor %s for scene %s", event_data->device_name, scene->name);
                        scene_exec(scene);
                    }
                }

                free(it);

                /*scene_manager_foreach_source(scene_source, scene)
                {
                    LOG_ADVANCED(Scene, "Scene event from sensor %s for scene %s", event_data->device_name, scene->name);
                    scene_exec(scene);
                }}*/
            }
            else
            {
                LOG_DEBUG(Scene, "Scene not registered");
            }
        }
    case DT_VDEV_EVENT_DATA:
        {
            vdev_event_data_t* event_data = (vdev_event_data_t*)variant_get_ptr(event->data);
            vdev_t* vdev = vdev_manager_get_vdev_by_id(event_data->vdev_id);

            if(NULL != vdev)
            {
                LOG_DEBUG(Scene, "Scene event from virtual device: %s vdev-id %d, instance-id %d, with command: 0x%x", vdev->name, event_data->vdev_id, event_data->instance_id, event_data->event_id);
        
                //command_class_t* command_class = vdev_manager_get_command_class_by_id(vdev->vdev_id, event_data->command_id);
                const char* vdev_resolved_name = resolver_name_from_id(event_data->vdev_id, event_data->instance_id, event_data->event_id);
                if(NULL != vdev_resolved_name)
                {
                    LOG_DEBUG(Scene, "Matching command-class %s found for virtual device %s", vdev_resolved_name, vdev->name);
                    
                    hash_iterator_t* it = variant_hash_begin(scene_table);
        
                    while(!variant_hash_iterator_is_end(variant_hash_iterator_next(it)))
                    {
                        scene_t* scene = (scene_t*)variant_get_ptr(variant_hash_iterator_value(it));
                        if(NULL != scene->source && strcmp(scene->source, vdev_resolved_name) == 0)
                        {
                            LOG_ADVANCED(Scene, "Scene event from virtual device %s for scene %s", vdev->name, scene->name);
                            scene_exec(scene);
                        }
                    }
    
                    free(it);
                }
                else
                {
                    LOG_DEBUG(Scene, "Scene not registered");
                }

            }
        }
        break;
    default:
        LOG_ERROR(Scene, "Invalid scene event %d", event->source_id);
    }
}
#endif

scene_t*    scene_manager_get_scene(const char* name)
{
    /*stack_for_each(scene_list, scene_variant)
    {
        scene_t* scene = (scene_t*)variant_get_ptr(scene_variant);
        if(strcmp(scene->name, name) == 0)
        {
            return scene;
        }
    }

    return NULL;*/

    uint32_t key = crc32(0, name, strlen(name));
    variant_t* scene_variant = variant_hash_get(scene_table, key);

    return (NULL == scene_variant)? NULL : (scene_t*)variant_get_ptr(scene_variant);
}

void    scene_manager_for_each(void (*visitor)(scene_t*, void*), void* arg)
{
    /*stack_for_each(scene_list, scene_variant)
    {
        scene_t* scene = (scene_t*)variant_get_ptr(scene_variant);
        visitor(scene, arg);
    }*/

    variant_hash_for_each_value(scene_table, scene_t*, visitor, arg);
}

variant_t*  scene_set_enabled_impl(struct service_method_t* method, va_list args);
variant_t*  scene_exec_impl(struct service_method_t* method, va_list args);
variant_t*  scene_is_enabled_impl(struct service_method_t* method, va_list args);

builtin_service_t*  scene_service_create(hash_table_t* service_table)
{
    builtin_service_t*   service = builtin_service_create(service_table, "Scene", "Scene management methods");
    builtin_service_add_method(service, "SetEnabled", "Set scene enabled (args: scene, bool)", 2, scene_set_enabled_impl);
    builtin_service_add_method(service, "IsEnabled", "Check if scene is enabled (args: scene)", 1, scene_is_enabled_impl);
    builtin_service_add_method(service, "Exec", "Exec scene (args: scene)", 1, scene_exec_impl);

    return service;
}

variant_t*  scene_set_enabled_impl(struct service_method_t* method, va_list args)
{
    variant_t* scene_name_variant = va_arg(args, variant_t*);
    variant_t* enabled_variant = va_arg(args, variant_t*);

    scene_t* scene = scene_manager_get_scene(variant_get_string(scene_name_variant));

    if(NULL != scene)
    {
        scene->is_enabled = variant_get_bool(enabled_variant);
        return variant_create_bool(true);
    }

    return variant_create_bool(false);
}

variant_t*  scene_exec_impl(struct service_method_t* method, va_list args)
{
    variant_t* scene_name_variant = va_arg(args, variant_t*);

    scene_t* scene = scene_manager_get_scene(variant_get_string(scene_name_variant));

    if(NULL != scene)
    {
        scene_exec(scene);
        return variant_create_bool(true);
    }

    return variant_create_bool(false);
}

variant_t*  scene_is_enabled_impl(struct service_method_t* method, va_list args)
{
    variant_t* scene_name_variant = va_arg(args, variant_t*);

    scene_t* scene = scene_manager_get_scene(variant_get_string(scene_name_variant));

    if(NULL != scene)
    {
        return variant_create_bool(scene->is_enabled);
    }

    return variant_create_bool(false);
}

