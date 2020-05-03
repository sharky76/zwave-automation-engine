import logging
import events

class Context:
	def __init__(self):
		self.id = 0
		self.lum = 0
		self.temp = 0
		self.motion = False
		self.humidity = 0
		self.buttonPressed = False

def on_init(id):
	context = Context
	logging.log_info(id, "ON_INIT called!")
	events.register_context(id, context)
	events.register_device_events(id)
	context.id = id
	return True

def on_device_event(context, device_id):
	if context:
		logging.log_info(context.id, "ON_DEVICE_EVENT: device_id = " + str(device_id))

		if device_id == 263 or device_id == 15:
			events.register_data_events(context.id, device_id)

def on_data_event(context, type, node_id, instance_id, command_id, dataHolder):
	if context:
		logging.log_info(context.id, "Type: {}, node_id: {}, instance: {}, command: {}, data: {}".format(type, node_id, instance_id, command_id, dataHolder))
		if node_id == 15:
			updateMotionSensorState(context, command_id, dataHolder)
		elif node_id == 263:
			updateVirtualButtonState(context, command_id, dataHolder)

def updateMotionSensorState(context, command_id, dataHolder):
	pass

def updateVirtualButtonState(context, command_id, dataHolder):
	pass
