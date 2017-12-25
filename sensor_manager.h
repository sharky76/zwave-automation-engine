
typedef struct sensor_role_t
{
    int     node_id;
    char*   role;
} sensor_role_t;

void    sensor_manager_init();
void    sensor_manager_set_role(int node_id, const char* role);
void    sensor_manager_remove_role(int node_id);
char*   sensor_manager_get_role(int node_id);
void    sensor_manager_for_each(void (*visitor)(sensor_role_t*, void*), void* arg);
