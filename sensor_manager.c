#include "sensor_manager.h"
#include "hash.h"
#include <stdlib.h>
#include <string.h>

void delete_sensor_role(void* arg)
{
    sensor_role_t* sensor_role = (sensor_role_t*)arg;
    free(sensor_role->role);
    free(sensor_role);
}

hash_table_t*   sensor_role_table;

void    sensor_manager_init()
{
    sensor_role_table = variant_hash_init();
}

void    sensor_manager_set_role(int node_id, const char* role)
{
    sensor_role_t* sensor_role = malloc(sizeof(sensor_role_t));
    sensor_role->node_id = node_id;
    sensor_role->role = strdup(role);

    variant_hash_remove(sensor_role_table, node_id);
    variant_hash_insert(sensor_role_table, node_id, variant_create_ptr(DT_PTR, sensor_role, &delete_sensor_role));
}

void    sensor_manager_remove_role(int node_id)
{
    variant_hash_remove(sensor_role_table, node_id);
}

char*   sensor_manager_get_role(int node_id)
{
    variant_t* role_variant = variant_hash_get(sensor_role_table, node_id);

    if(NULL != role_variant)
    {
        sensor_role_t* sensor_role = VARIANT_GET_PTR(sensor_role_t, role_variant);
        return sensor_role->role;
    }

    return NULL;
}

typedef struct
{
    void (*sensor_visitor)(sensor_role_t*, void*);
    void* visitor_arg;
} sensor_manager_visitor_data_t;

void call_sensor_visitor(hash_node_data_t* hash_node, void* arg)
{
    sensor_manager_visitor_data_t* sensor_data = (sensor_manager_visitor_data_t*)arg;
    sensor_role_t* sensor_role = (sensor_role_t*)variant_get_ptr(hash_node->data);

    sensor_data->sensor_visitor(sensor_role, sensor_data->visitor_arg);
}

void    sensor_manager_for_each(void (*visitor)(sensor_role_t*, void*), void* arg)
{
    sensor_manager_visitor_data_t sensor_data = {
        .sensor_visitor = visitor,
        .visitor_arg = arg
    };

    variant_hash_for_each(sensor_role_table, call_sensor_visitor, &sensor_data);
}
