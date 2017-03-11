#include "vdev.h"
#include "event.h"

void vdev_delete_event(void* arg)
{
    vdev_event_data_t* e = (vdev_event_data_t*)arg;
    //free(e->data);
    free(e);
}

void    vdev_post_event(int vdev_id, int event_id, int instance_id, void* data)
{
    vdev_event_data_t* event_data = calloc(1, sizeof(vdev_event_data_t));
    event_data->vdev_id = vdev_id;
    event_data->event_id = event_id;
    event_data->instance_id = instance_id;
    event_data->data = data;
    event_t* new_event = event_create(DT_VDEV, VDEV_DATA_CHANGE_EVENT, variant_create_ptr(DT_VDEV_EVENT_DATA, event_data, vdev_delete_event));
    event_post(new_event);
}
