#ifndef DEVICE_CALLBACKS
#define DEVICE_CALLBACKS

// List of callbacks foe device operations
#include <ZWayLib.h>

void device_added_callback(const ZWay zway, ZWDeviceChangeType type, ZWBYTE node_id, ZWBYTE instance_id, ZWBYTE command_id, void *arg);
void command_removed_callback(const ZWay zway, ZWDeviceChangeType type, ZWBYTE node_id, ZWBYTE instance_id, ZWBYTE command_id, void *arg);
void command_added_callback(const ZWay zway, ZWDeviceChangeType type, ZWBYTE node_id, ZWBYTE instance_id, ZWBYTE command_id, void *arg);


#endif // DEVICE_CALLBACKS
