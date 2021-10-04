import logging
import events
import command
import json
import datetime

class Context:
    def __init__(self):
        self.id = 0
        self.watt = 0
        self.watt_prev = 0
        self.amp = 0
        self.volt = 0
        self.kwh = False


def on_init(id):
	context = Context()
	events.register_context(id, context)
	events.register_device_events(id)
	context.id = id

	return True


def on_device_event(context, device_id):
	if context:
		#device_name = resolver.name_from_id(device_id)
		logging.log_debug(
		    context.id, "ON_DEVICE_EVENT: device_id = " + str(device_id))

		if device_id == 32 or device_id == 263:
			events.register_data_events(context.id, device_id)


def on_data_event(context, type, node_id, instance_id, command_id, dataHolder):
    if context:
        #logging.log_debug(context.id, "Type: {}, node_id: {}, instance: {}, command: {}, data: {}".format(
        #    type, node_id, instance_id, command_id, dataHolder))
        if node_id == 32:
            if command_id == 50:
                updatePowerMeter(context, node_id, command_id, dataHolder)

            #elif node_id == 263 and instance_id == 1 and command_id == 37:
            #    updateVirtualButtonState(
            #        context, node_id, command_id, dataHolder)


def updatePowerMeter(context, node_id, command_id, dataHolder):
    val = json.loads(dataHolder)
    #logging.log_debug(context.id, "dataHolder = " + str(dataHolder))
    if val['scale'] == 2:
        context.watt_prev = context.watt
        context.watt = val['val']
        updateButtonState(context)
    elif val['scale'] == 4:
        context.volt = val['val']
    elif val['scale'] == 5:
        context.amp = val['val']
    elif val['scale'] == 0:
        context.kwh = val['val']

def updateButtonState(context):
    if context.watt == 0 and context.watt_prev > 1:
        command.set_boolean(context.id, 263, 1, 37, True)
    else:
        command.set_boolean(context.id, 263, 1, 37, False)


def updateVirtualButtonState(context, node_id, command_id, dataHolder):
	val = json.loads(dataHolder)
	if val['level']:
		command.set_boolean(context.id, 263, 1, 37, True)
	else:
		command.set_boolean(context.id, 263, 1, 37, False)
