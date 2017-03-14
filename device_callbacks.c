#include "device_callbacks.h"
#include "resolver.h"
#include "data_callbacks.h"
#include <ZWayLib.h>
#include <ZData.h>
#include <stdio.h>
#include <logger.h>
#include <event.h>

DECLARE_LOGGER(DeviceCallback)

void device_added_callback(const ZWay zway, ZWDeviceChangeType type, ZWBYTE node_id, ZWBYTE instance_id, ZWBYTE command_id, void *arg)
{
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
        }
    }
}

void command_removed_callback(const ZWay zway, ZWDeviceChangeType type, ZWBYTE node_id, ZWBYTE instance_id, ZWBYTE command_id, void *arg)
{
    LOG_DEBUG(DeviceCallback, "Command removed node-id %d, instance-id %d, command-id %d", node_id, instance_id, command_id);
}

void command_added_callback(const ZWay zway, ZWDeviceChangeType type, ZWBYTE node_id, ZWBYTE instance_id, ZWBYTE command_id, void *arg)
{
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
            event_data->device_name = resolver_name_from_id(node_id, instance_id, command_id);

            zdata_add_callback_ex(dataHolder, &data_change_event_callback, FALSE, event_data);

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

