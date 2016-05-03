#include "vdev.h"
#include "event.h"

void    vdev_post_event(int vdev_id, int command_id)
{
    vdev_event_data_t* event_data = calloc(1, sizeof(vdev_event_data_t));
    event_data->vdev_id = vdev_id;
    event_data->command_id = command_id;

    event_t* new_event = event_create(DT_VDEV, 
                                      variant_create_ptr(DT_VDEV_EVENT_DATA, event_data, variant_delete_default));
    event_post(new_event);
}
