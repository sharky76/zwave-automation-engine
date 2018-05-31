#include "sensor_manager.h"
#include "crc32.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void delete_sensor_descriptor(void* arg)
{
    sensor_descriptor_t* sensor_desc = (sensor_descriptor_t*)arg;

    for(int i = 0; i < 256; i++)
    {
        free(sensor_desc->name[i]);
    }
    variant_hash_free(sensor_desc->sensor_roles);
    variant_hash_free(sensor_desc->alarm_roles);
    variant_hash_free(sensor_desc->supported_notification_set);

    free(sensor_desc);
}

hash_table_t*   sensor_descriptor_table;

sensor_descriptor_t*    sensor_descriptor_new(int node_id)
{
    sensor_descriptor_t* sensor_desc = malloc(sizeof(sensor_descriptor_t));
    sensor_desc->node_id = node_id;
    sensor_desc->sensor_roles = variant_hash_init();
    sensor_desc->alarm_roles = variant_hash_init();
    sensor_desc->multi_instance = false;
    memset(sensor_desc->name, 0, sizeof(char*)*256);
    sensor_desc->supported_notification_set = variant_hash_init();

    return sensor_desc;
}

void    sensor_manager_init()
{
    sensor_descriptor_table = variant_hash_init();
}

void    sensor_manager_set_role(int node_id, int command_id, const char* role)
{
    variant_t* sensor_desc_variant = variant_hash_get(sensor_descriptor_table, node_id);

    if(NULL == sensor_desc_variant)
    {
        sensor_descriptor_t* sensor_desc = sensor_descriptor_new(node_id);
        //sensor_desc->role = strdup(role);

        variant_hash_insert(sensor_desc->sensor_roles, 
                            command_id, 
                            variant_create_string(strdup(role)));

        variant_hash_insert(sensor_descriptor_table, node_id, variant_create_ptr(DT_PTR, sensor_desc, &delete_sensor_descriptor));
    }
    else
    {
        sensor_descriptor_t* sensor_desc = VARIANT_GET_PTR(sensor_descriptor_t, sensor_desc_variant);

        variant_hash_remove(sensor_desc->sensor_roles, command_id);
        variant_hash_insert(sensor_desc->sensor_roles, 
                            command_id, 
                            variant_create_string(strdup(role)));
    }
}

void    sensor_manager_set_alarm_role(int node_id, int alarm_id, const char* role)
{
    variant_t* sensor_desc_variant = variant_hash_get(sensor_descriptor_table, node_id);

    if(NULL == sensor_desc_variant)
    {
        sensor_descriptor_t* sensor_desc = sensor_descriptor_new(node_id);
        //sensor_desc->role = strdup(role);

        variant_hash_insert(sensor_desc->alarm_roles, 
                            alarm_id, 
                            variant_create_string(strdup(role)));

        variant_hash_insert(sensor_descriptor_table, node_id, variant_create_ptr(DT_PTR, sensor_desc, &delete_sensor_descriptor));
    }
    else
    {
        sensor_descriptor_t* sensor_desc = VARIANT_GET_PTR(sensor_descriptor_t, sensor_desc_variant);

        variant_hash_remove(sensor_desc->alarm_roles, alarm_id);
        variant_hash_insert(sensor_desc->alarm_roles, 
                            alarm_id, 
                            variant_create_string(strdup(role)));
    }
}

void    sensor_manager_set_name(int node_id, const char* name)
{
    sensor_manager_set_instance_name(node_id, 0, name);
}

void    sensor_manager_set_instance_name(int node_id, int instance_id, const char* name)
{
    variant_t* sensor_desc_variant = variant_hash_get(sensor_descriptor_table, node_id);

    if(NULL == sensor_desc_variant)
    {
        sensor_descriptor_t* sensor_desc = sensor_descriptor_new(node_id);
        sensor_desc->name[instance_id] = strdup(name);

        variant_hash_insert(sensor_descriptor_table, node_id, variant_create_ptr(DT_PTR, sensor_desc, &delete_sensor_descriptor));
    }
    else
    {
        sensor_descriptor_t* sensor_desc = VARIANT_GET_PTR(sensor_descriptor_t, sensor_desc_variant);
        free(sensor_desc->name[instance_id]);
        sensor_desc->name[instance_id] = strdup(name);

        if(instance_id > 0)
        {
            sensor_desc->multi_instance = true;
        }
    }
}

void    sensor_manager_set_notification(int node_id, const char* notification)
{
    variant_t* sensor_desc_variant = variant_hash_get(sensor_descriptor_table, node_id);

    if(NULL == sensor_desc_variant)
    {
        sensor_descriptor_t* sensor_desc = sensor_descriptor_new(node_id);
        variant_hash_insert(sensor_desc->supported_notification_set, 
                            crc32(0, notification, strlen(notification)), 
                            variant_create_string(strdup(notification)));
        variant_hash_insert(sensor_descriptor_table, node_id, variant_create_ptr(DT_PTR, sensor_desc, &delete_sensor_descriptor));
    }
    else
    {
        sensor_descriptor_t* sensor_desc = VARIANT_GET_PTR(sensor_descriptor_t, sensor_desc_variant);

        uint32_t key = crc32(0, notification, strlen(notification));
        variant_hash_remove(sensor_desc->supported_notification_set, key);

        variant_hash_insert(sensor_desc->supported_notification_set, 
                            crc32(0, notification, strlen(notification)), 
                            variant_create_string(strdup(notification)));
    }
}

void    sensor_manager_add_descriptor(int node_id)
{
    variant_t* sensor_desc_variant = variant_hash_get(sensor_descriptor_table, node_id);

    if(NULL == sensor_desc_variant)
    {
        sensor_descriptor_t* sensor_desc = sensor_descriptor_new(node_id);
        variant_hash_insert(sensor_descriptor_table, node_id, variant_create_ptr(DT_PTR, sensor_desc, &delete_sensor_descriptor));
    }
}

void    sensor_manager_remove_descriptor(int node_id)
{
    variant_hash_remove(sensor_descriptor_table, node_id);
}

sensor_descriptor_t*   sensor_manager_get_descriptor(int node_id)
{
    variant_t* sensor_desc_variant = variant_hash_get(sensor_descriptor_table, node_id);

    if(NULL != sensor_desc_variant)
    {
        sensor_descriptor_t* sensor_desc = VARIANT_GET_PTR(sensor_descriptor_t, sensor_desc_variant);
        return sensor_desc;
    }

    return NULL;
}

typedef struct
{
    void (*sensor_visitor)(sensor_descriptor_t*, void*);
    void* visitor_arg;
} sensor_manager_visitor_data_t;

void call_sensor_visitor(hash_node_data_t* hash_node, void* arg)
{
    sensor_manager_visitor_data_t* sensor_data = (sensor_manager_visitor_data_t*)arg;
    sensor_descriptor_t* sensor_desc = (sensor_descriptor_t*)variant_get_ptr(hash_node->data);

    sensor_data->sensor_visitor(sensor_desc, sensor_data->visitor_arg);
}

void    sensor_manager_for_each(void (*visitor)(sensor_descriptor_t*, void*), void* arg)
{
    sensor_manager_visitor_data_t sensor_data = {
        .sensor_visitor = visitor,
        .visitor_arg = arg
    };

    variant_hash_for_each(sensor_descriptor_table, call_sensor_visitor, &sensor_data);
}

void serialize_sensor_notification(char* notification, void* arg)
{
    struct json_object* notification_array = (struct json_object*)arg;

    json_object_array_add(notification_array, json_object_new_string(notification));
}

void serialize_sensor_roles(hash_node_data_t* node_data, void* arg)
{
    struct json_object* role_object = (struct json_object*)arg;

    char command_id_buf[5] = {0};
    snprintf(command_id_buf, 4, "%d", node_data->key);
    json_object_object_add(role_object, command_id_buf, json_object_new_string(variant_get_string(node_data->data)));
}

struct json_object*    sensor_manager_serialize(int node_id)
{
    struct json_object* result = json_object_new_object();
    sensor_descriptor_t* sensor_desc = sensor_manager_get_descriptor(node_id);

    if(NULL != sensor_desc)
    {
        if(NULL != sensor_desc->name[0])
        {   
            struct json_object* names = json_object_new_array();
            for(int i = 0; i < 256; i++)
            {
                if(NULL != sensor_desc->name[i])
                {
                    json_object_array_add(names, json_object_new_string(sensor_desc->name[i]));
                }
            }

            json_object_object_add(result, "name", names);
        }

        if(sensor_desc->sensor_roles->count > 0)
        {
            struct json_object* role_object = json_object_new_object();
            variant_hash_for_each(sensor_desc->sensor_roles, serialize_sensor_roles, role_object);

            json_object_object_add(result, "roles", role_object);
        }

        if(sensor_desc->alarm_roles->count > 0)
        {
            struct json_object* role_object = json_object_new_object();
            variant_hash_for_each(sensor_desc->alarm_roles, serialize_sensor_roles, role_object);

            json_object_object_add(result, "alarmRoles", role_object);
        }

        if(sensor_desc->supported_notification_set->count > 0)
        {
            struct json_object* notification_array = json_object_new_array();
            variant_hash_for_each_value(sensor_desc->supported_notification_set, char*, serialize_sensor_notification, notification_array);

            json_object_object_add(result, "notifications", notification_array);
        }
    }

    return result;
}
