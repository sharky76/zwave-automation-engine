/**
 * Virtual devices manager 
 *  
 * Loads virtual devices objects and manages calls to command 
 * classes 
 */
#include "command_class.h"
#include <event.h>
#include <vdev.h>

//typedef struct vdev_t vdev_t;
//typedef struct vdev_command_t vdev_command_t;

void                vdev_manager_init(const char* vdev_dir);
void                vdev_manager_start_devices();
bool                vdev_manager_is_vdev_exists(const char* vdev_name);
vdev_t*             vdev_manager_get_vdev(const char* vdev_name);
vdev_t*             vdev_manager_get_vdev_by_id(int vdev_id);
command_class_t*    vdev_manager_get_command_class(int vdev_id);
device_record_t*    vdev_manager_create_device_record(const char* vdev_name);
void                vdev_manager_for_each(void (*visitor)(vdev_t*, void*), void* arg);
void                vdev_manager_for_each_method(const char* vdev_name, void (*visitor)(vdev_command_t*, void*), void* arg);
void                vdev_manager_on_event(event_t* event);

