#include "hash.h"
#include <json-c/json.h>

typedef struct sensor_descriptor_t
{
    int node_id;
    char* name[256];
    hash_table_t*    sensor_roles;
    hash_table_t*    alarm_roles;
    hash_table_t*    supported_notification_set;
    bool             multi_instance;
} sensor_descriptor_t;

void    sensor_manager_init();
void    sensor_manager_set_role(int node_id, int command_id, const char* role);
void    sensor_manager_set_alarm_role(int node_id, int alarm_id, const char* role);
void    sensor_manager_set_name(int node_id, const char* name);
void    sensor_manager_set_instance_name(int node_id, int instance_id, const char* name);
void    sensor_manager_set_notification(int node_id, const char* notification);
void    sensor_manager_add_descriptor(int node_id);
void    sensor_manager_remove_descriptor(int node_id);
sensor_descriptor_t*   sensor_manager_get_descriptor(int node_id);
void    sensor_manager_for_each(void (*visitor)(sensor_descriptor_t*, void*), void* arg);
struct json_object*    sensor_manager_serialize(int node_id);
