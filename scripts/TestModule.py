import logging
import events

mod_id = 0

class Context:
	def __init__(self):
		self.lum = 0
		self.temp = 0
		self.motion = False
		self.humidity = 0
		self.buttonPressed = False

context = Context

def on_init(id):
	global mod_id
	logging.log_info(id, "ON_INIT called!")
	events.register_device_events(id)
	mod_id = id
	return True

def on_device_event(device_id):
	global mod_id
	logging.log_info(mod_id, "ON_DEVICE_EVENT: device_id = " + str(device_id))

	if device_id == 263 or device_id == 15:
		events.register_data_events(mod_id, device_id)

def on_data_event(type, node_id, instance_id, command_id, dataHolder):
	global mod_id
	logging.log_info(mod_id, "Type: {}, node_id: {}, instance: {}, command: {}, data: {}".format(type, node_id, instance_id, command_id, dataHolder))
	if node_id == 15:
		updateMotionSensorState(command_id, dataHolder)
	elif node_id == 263:
		updateVirtualButtonState(command_id, dataHolder)

def updateMotionSensorState(command_id, dataHolder):
	global context
	pass

def updateVirtualButtonState(command_id, dataHolder):
	global context
	pass
