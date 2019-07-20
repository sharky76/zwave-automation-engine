#include "vdev.h"
#include "event.h"
#include "event_log.h"
#include "event_pump.h"

void vdev_delete_event(void* arg)
{
    vdev_event_data_t* e = (vdev_event_data_t*)arg;
    free(e);
}

void    vdev_post_event(int event_id, int vdev_id, int command_id, int instance_id)
{
    event_pump_t* pump = event_dispatcher_get_pump("EVENT_PUMP");
    vdev_event_data_t* event_data = calloc(1, sizeof(vdev_event_data_t));
    event_data->vdev_id = vdev_id;
    event_data->event_id = command_id;
    event_data->instance_id = instance_id;

    event_pump_send_event(pump, event_id, event_data, &vdev_delete_event);
}