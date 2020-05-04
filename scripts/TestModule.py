import logging
import events
import command
import json
import datetime

class Context:
	def __init__(self):
		print("Init context")
		self.id = 0
		self.lum = 0
		self.temp = 0
		self.motion = False
		self.humidity = 0
		self.explicitButton = False
		self.implicitButton = False

def on_init(id):
	context = Context()
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
			if datetime.datetime.now().hour > 16:
				#and datetime.datetime.now().hour < 22:
				logging.log_info(context.id, "Type: {}, node_id: {}, instance: {}, command: {}, data: {}".format(type, node_id, instance_id, command_id, dataHolder))
				if node_id == 15 and command_id == 113:
					updateMotionSensorState(context, command_id, dataHolder)
				elif node_id == 263:
					updateVirtualButtonState(context, command_id, dataHolder)
			else:
				context.explicitButton = False
				context.implicitButton = False

def updateMotionSensorState(context, command_id, dataHolder):
	logging.log_info(context.id, "In updateMotionSensorState")
	val = json.loads(command.get_value(context.id, 15, 0, 113, "7.event"))
	if val['device_data']['event'] == 8:
		logging.log_info(context.id, "updateMotionSensorState Motion Detected")
		onMotionDetected(context)
	else:
		logging.log_info(context.id, "updateMotionSensorState No Motion")
		onNoMotionDetected(context)
	
def updateVirtualButtonState(context, command_id, dataHolder):
	logging.log_info(context.id, "In updateVirtualButtonState")
	if not context.implicitButton:
		context.explicitButton = True

def onMotionDetected(context):
	logging.log_info(context.id, "onMotionDetected")
	val = json.loads(command.get_value(context.id, 15, 0, 49, "3"))
	if val['device_data']['val'] < 20 and context.explicitButton == False:
		context.implicitButton = True
		command.set_boolean(context.id, 263, 0, 37, True)
	else:
		context.implicitButton = False

def onNoMotionDetected(context):
	logging.log_info(context.id, "onNoMotionDetected")
	if context.explicitButton == False:
		context.implicitButton = True
		command.set_boolean(context.id, 263, 0, 37, False)
	
