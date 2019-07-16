#include "device_callbacks.h"
#include "resolver.h"
#include "data_callbacks.h"
#include <ZWayLib.h>
#include <ZData.h>
#include <stdio.h>
#include <logger.h>
#include <event.h>
#include "command_class.h"

DECLARE_LOGGER(DeviceCallback)

void device_added_callback(const ZWay zway, ZWDeviceChangeType type, ZWBYTE node_id, ZWBYTE instance_id, ZWBYTE command_id, void *arg)
{
    event_pump_t* pump = (event_pump_t*)arg;

    if(1 != node_id)
    {
        const char* deviceName = resolver_name_from_id(node_id, instance_id, command_id);

        if(NULL != deviceName)
        {
            LOG_ADVANCED(DeviceCallback, "New device added (%s) - Change type %d, Node Id %d, Instance Id %d, Command Id %d", deviceName, type, node_id, instance_id, command_id);
        }
        else
        {
            LOG_ADVANCED(DeviceCallback, "New unresolved device added - Change type %d, Node Id %d, Instance Id %d, Command Id %d", type, node_id, instance_id, command_id);

            if(NULL == resolver_name_from_node_id(node_id))
            {
                char buf[16] = {0};
                snprintf(buf, 15, "ZWaveDev.%d", node_id);
                resolver_add_device_entry(ZWAVE, buf, node_id);
            }
        }

        //TODO: Replace with pump
        //event_t* device_added_event = event_create(DeviceCallback, "DeviceAddedEvent", variant_create_int32(DT_INT8, node_id));
        //event_post(device_added_event);

        event_pump_send_event(pump, DeviceAddedEvent, (void*)(int)node_id, NULL);

    }
}

void command_removed_callback(const ZWay zway, ZWDeviceChangeType type, ZWBYTE node_id, ZWBYTE instance_id, ZWBYTE command_id, void *arg)
{
    LOG_DEBUG(DeviceCallback, "Command removed node-id %d, instance-id %d, command-id %d", node_id, instance_id, command_id);
}

void command_added_callback(const ZWay zway, ZWDeviceChangeType type, ZWBYTE node_id, ZWBYTE instance_id, ZWBYTE command_id, void *arg)
{
    event_pump_t* pump = (event_pump_t*)arg;

    if(1 != node_id)
    {
        LOG_ADVANCED(DeviceCallback, "New command found - Change type %d, Node Id %d, Instance Id %d, Command Id 0x%x", type, node_id, instance_id, command_id);
        
        zdata_acquire_lock(ZDataRoot(zway));

    	ZDataHolder dataHolder = zway_find_device_instance_cc_data(zway, node_id, instance_id, command_id, NULL);
    
    	if (dataHolder != NULL) 
        {
    		//LOG_DEBUG("Adding callback for node_id=%i instance_id=%i command_id=0x%x...",node_id,instance_id,command_id);

            sensor_event_data_t* event_data = (sensor_event_data_t*)malloc(sizeof(sensor_event_data_t));
            event_data->node_id = node_id;
            event_data->command_id = command_id;
            event_data->instance_id = instance_id;
            event_data->last_update_time = 0;
            event_data->device_name = resolver_name_from_node_id(node_id);
            event_data->callback_added = false;
            event_data->event_pump = pump;
            
            if(NULL == event_data->device_name || 
               NULL == resolver_name_from_id(node_id, instance_id, command_id))
            {
                char default_name[MAX_DEVICE_NAME_LEN] = {0};
                command_class_t* cmd_class = get_command_class_by_id(command_id);
                if(NULL != cmd_class)
                {
                    snprintf(default_name, MAX_DEVICE_NAME_LEN-1, "%d.%d.%s", node_id, instance_id, cmd_class->command_name);
                }
                else
                {
                    snprintf(default_name, MAX_DEVICE_NAME_LEN-1, "%d.%d.%d", node_id, instance_id, command_id);
                }

                LOG_ADVANCED(DeviceCallback, "Adding default name for device: %s", default_name);
                resolver_add_entry(ZWAVE, default_name, node_id, instance_id, command_id);
                event_data->device_name = resolver_name_from_id(node_id, instance_id, command_id);
            }

            zdata_add_callback_ex(dataHolder, &data_change_event_callback, TRUE, event_data);

            if(NULL != event_data->device_name)
            {
                LOG_ADVANCED(DeviceCallback, "New callback added for %s and command class: 0x%x", event_data->device_name, command_id);
            }
            else
            {
                LOG_ADVANCED(DeviceCallback, "New callback added for unknown device node %d and command class: 0x%x", node_id, command_id);
            }
    	} 
        else 
        {
    		LOG_DEBUG(DeviceCallback, "No data holder for deviceId=%i instanceId=%i command_id=%i",node_id,instance_id,command_id);
    	}

    	zdata_release_lock(ZDataRoot(zway));
    }
}

