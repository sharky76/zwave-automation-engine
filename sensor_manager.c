#include "sensor_manager.h"
#include "crc32.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void delete_sensor_descriptor(void* arg)
{
    sensor_descriptor_t* sensor_desc = (sensor_descriptor_t*)arg;
    vector_free(sensor_desc->instances);
    free(sensor_desc);
}

void delete_sensor_instance(void* arg)
{
    sensor_instance_t* instance = (sensor_instance_t*)arg;
    free(instance->name);
    variant_hash_free(instance->sensor_roles);
    variant_hash_free(instance->alarm_roles);
    free(instance);
}

bool match_instance(variant_t* value, void* arg)
{
    sensor_instance_t* instance = VARIANT_GET_PTR(sensor_instance_t, value);
    int instance_id = (int)arg;
    return instance->instance_id == instance_id;
}

hash_table_t*   sensor_descriptor_table;

sensor_descriptor_t*    sensor_descriptor_new(int node_id)
{
    sensor_descriptor_t* sensor_desc = malloc(sizeof(sensor_descriptor_t));
    sensor_desc->node_id = node_id;
    sensor_desc->instances = vector_create();

    return sensor_desc;
}

sensor_instance_t* sensor_instance_new(int instance_id)
{
    sensor_instance_t* instance = malloc(sizeof(sensor_instance_t));
    instance->instance_id = instance_id;
    instance->sensor_roles = variant_hash_init();
    instance->alarm_roles = variant_hash_init();
    instance->name = NULL;

    return instance;
}

void    sensor_manager_init()
{
    sensor_descriptor_table = variant_hash_init();
}

void    sensor_manager_set_role(int node_id, int instance_id, int command_id, const char* role)
{
    variant_t* sensor_desc_variant = variant_hash_get(sensor_descriptor_table, node_id);

    if(NULL != sensor_desc_variant)
    {
        sensor_descriptor_t* sensor_desc = VARIANT_GET_PTR(sensor_descriptor_t, sensor_desc_variant);

        variant_t* instance_variant = vector_find(sensor_desc->instances, &match_instance, (void*)instance_id);
        if(NULL != instance_variant)
        {
            sensor_instance_t* instance = VARIANT_GET_PTR(sensor_instance_t, instance_variant);
            variant_hash_remove(instance->sensor_roles, command_id);
            variant_hash_insert(instance->sensor_roles, 
                                command_id, 
                                variant_create_string(strdup(role)));
        }
    }
}

void    sensor_manager_set_alarm_role(int node_id, int instance_id, int alarm_id, const char* role)
{
    variant_t* sensor_desc_variant = variant_hash_get(sensor_descriptor_table, node_id);

    if(NULL != sensor_desc_variant)
    {
        sensor_descriptor_t* sensor_desc = VARIANT_GET_PTR(sensor_descriptor_t, sensor_desc_variant);
        variant_t* instance_variant = vector_find(sensor_desc->instances, &match_instance, (void*)instance_id);
        if(NULL != instance_variant)
        {
            sensor_instance_t* instance = VARIANT_GET_PTR(sensor_instance_t, instance_variant);

            variant_hash_remove(instance->alarm_roles, alarm_id);
            variant_hash_insert(instance->alarm_roles, 
                                alarm_id, 
                                variant_create_string(strdup(role)));
        }
    }
}

void    sensor_manager_set_instance_name(int node_id, int instance_id, const char* name)
{
    variant_t* sensor_desc_variant = variant_hash_get(sensor_descriptor_table, node_id);

    if(NULL != sensor_desc_variant)
    {
        sensor_descriptor_t* sensor_desc = VARIANT_GET_PTR(sensor_descriptor_t, sensor_desc_variant);

        variant_t* instance_variant = vector_find(sensor_desc->instances, &match_instance, (void*)instance_id);
        if(NULL != instance_variant)
        {
            sensor_instance_t* instance = VARIANT_GET_PTR(sensor_instance_t, instance_variant);
            free(instance->name);
            instance->name = strdup(name);
        }
    }
}

void    sensor_manager_remove_instance(int node_id, int instance_id)
{
    variant_t* sensor_desc_variant = variant_hash_get(sensor_descriptor_table, node_id);
    sensor_descriptor_t* sensor_desc = VARIANT_GET_PTR(sensor_descriptor_t, sensor_desc_variant);

    variant_t* instance_variant = vector_find(sensor_desc->instances, &match_instance, (void*)instance_id);
    if(NULL != instance_variant)
    {
        vector_remove(sensor_desc->instances, instance_variant);
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

void    sensor_manager_add_instance(int node_id, int instance_id)
{
    sensor_descriptor_t* desc = sensor_manager_get_descriptor(node_id);

    if(NULL != desc)
    {
        sensor_instance_t* instance = sensor_instance_new(instance_id);
        vector_push_back(desc->instances, variant_create_ptr(DT_PTR, instance, &delete_sensor_instance));
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

sensor_instance_t* sensor_manager_get_instance(int node_id, int instance_id)
{
    sensor_descriptor_t* sensor_desc = sensor_manager_get_descriptor(node_id);

    if(NULL != sensor_desc)
    {
        variant_t* instance = vector_peek_at(sensor_desc->instances, instance_id);
        if(NULL != instance)
        {
            return VARIANT_GET_PTR(sensor_instance_t, instance);
        }
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

void serialize_sensor_roles(hash_node_data_t* node_data, void* arg)
{
    struct json_object* role_object = (struct json_object*)arg;

    char command_id_buf[5] = {0};
    snprintf(command_id_buf, 4, "%d", node_data->key);
    json_object_object_add(role_object, command_id_buf, json_object_new_string(variant_get_string(node_data->data)));
}

void serialize_instance(variant_t* value, void* arg)
{
    sensor_instance_t* instance = VARIANT_GET_PTR(sensor_instance_t, value);

    struct json_object* instance_obj = (struct json_object*)arg;
    //struct json_object* result = json_object_new_object();

    //struct json_object* instance_obj = json_object_new_object();
    json_object_object_add(instance_obj, "id", json_object_new_int(instance->instance_id));
    
    if(NULL != instance->name)
    {
        json_object_object_add(instance_obj, "name", json_object_new_string(instance->name));
    }

    struct json_object* role_object = json_object_new_object();
    variant_hash_for_each(instance->sensor_roles, serialize_sensor_roles, role_object);
    json_object_object_add(instance_obj, "roles", role_object);

    struct json_object* alarm_object = json_object_new_object();
    variant_hash_for_each(instance->alarm_roles, serialize_sensor_roles, alarm_object);
    json_object_object_add(instance_obj, "alarmRoles", alarm_object);

    //json_object_array_add(instances_array, instance_obj);

}

/*struct json_object*    sensor_manager_serialize(int node_id)
{
    //struct json_object* result = json_object_new_object();
    sensor_descriptor_t* sensor_desc = sensor_manager_get_descriptor(node_id);
    struct json_object* instances = json_object_new_array();

    if(NULL != sensor_desc)
    {
        vector_for_each(sensor_desc->instances, serialize_instance, (void*)instances);

        //json_object_object_add(result, "instances", instances);
    }

    return instances;
}*/

struct json_object*    sensor_manager_serialize_instance(int node_id, int instance_id)
{
    sensor_descriptor_t* sensor_desc = sensor_manager_get_descriptor(node_id);
    struct json_object* instances = json_object_new_object();

    if(NULL != sensor_desc)
    {
        variant_t* instance_variant = vector_find(sensor_desc->instances, match_instance, (void*)instance_id);
        if(NULL != instance_variant)
        {
            serialize_instance(instance_variant, (void*)instances);
        }
    }

    return instances;
}
