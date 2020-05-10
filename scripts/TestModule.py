import logging
import events
import command
import json
import datetime
from enum import Enum

##
#States: ImplicitOff, ImplicitOn, ExplicitOff, explicitOn 
#Events: MotionDetected, NoMotionDetected, ButtonOn, ButtonOff
#Initial State: ImplicitOff
#Transitions:
#ImplicitOff -> MotionDetected -> ImplicitOff (timeofday) / ImplicitOn
#ImplicitOff -> NoMotionDetected -> ImplicitOff
#ImplicitOff -> ButtonOn -> ExplicitOn
#ImplicitOff -> ButtonOff -> ImplicitOff

#ImplicitOn -> MotionDetected -> ImplicitOn
#ImplicitOn -> NoMotionDetected -> ImplicitOff
#ImplicitOn -> ButtonOn -> ImplicitOn
#ImplicitOn -> ButtonOff -> ExplicitOff 


#ExplicitOff -> MotionDetected -> ExplicitOff / ImplicitOn (timeofday)
#ExplicitOff -> NoMotionDetected -> ExplicitOff
#ExplicitOff -> ButtonOn -> ExplicitOn
#ExplicitOff -> ButtonOff -> ExplicitOff

#ExplicitOn -> MotionDetected -> ExplicitOn
#ExplicitOn -> NoMotionDetected -> ExplicitOn
#ExplicitOn -> ButtonOn -> ExplicitOn
#ExplicitOn -> ButtonOff -> ExplicitOff

##

class States(Enum):
	ImplicitOff = 1
	ImplicitOn = 2
	ExplicitOff = 3
	ExplicitOn = 4

class Events(Enum):
	MotionDetected = 1
	NoMotionDetected = 2
	ButtonOn = 3
	ButtonOff = 4

class FSM:
	def __init__(self, context, button_id, button_command):
		self.stateTransitions = {
			States.ImplicitOff: { 
				Events.MotionDetected: self.onImplicitOnOrImplicitOffTimeOfDay,
			 	Events.NoMotionDetected: self.Noop,
			 	Events.ButtonOn: self.onExplicitOn,
			 	Events.ButtonOff: self.Noop
			},

			States.ImplicitOn: {
				Events.MotionDetected: self.Noop,
				Events.NoMotionDetected: self.onImplicitOff,
				Events.ButtonOn: self.Noop,
				Events.ButtonOff: self.onExplicitOff
			},

			States.ExplicitOff: {
				Events.MotionDetected: self.onImplicitOnOrExplicitOffTimeOfDay,
				Events.NoMotionDetected: self.Noop,
				Events.ButtonOn: self.onExplicitOn,
				Events.ButtonOff: self.Noop
			},

			States.ExplicitOn: {
				Events.MotionDetected: self.Noop,
				Events.NoMotionDetected: self.Noop,
				Events.ButtonOn: self.Noop,
				Events.ButtonOff: self.onExplicitOff
			}
		}

		self.currentState = States.ImplicitOff
		self.context = context
		self.button_id = button_id
		self.button_command = button_command
		self.nextImplicitOnStart = datetime.datetime.now()

	def changeState(self, event):
		state = self.currentState
		self.stateTransitions[self.currentState][event]()
		logging.log_info(self.context.id, "Switched state from " + str(state) + " on event " + str(event) + " to " + str(self.currentState))

	def Noop(self):
		pass

	def onImplicitOnOrImplicitOffTimeOfDay(self):
		if datetime.datetime.now().hour > 7 and datetime.datetime.now().hour < 23:
			if self.context.lum < 15:
				self.currentState = States.ImplicitOn
				command.set_boolean(self.context.id, self.button_id, 0, self.button_command, True)

	def onImplicitOnOrExplicitOffTimeOfDay(self):
		if datetime.datetime.now() >= self.nextImplicitOnStart and datetime.datetime.now().hour < 23:
			if self.context.lum < 15:
				self.currentState = States.ImplicitOn
				command.set_boolean(self.context.id, self.button_id, 0, self.button_command, True)

	def onExplicitOn(self):
		self.currentState = States.ExplicitOn
		command.set_boolean(self.context.id, self.button_id, 0, self.button_command, True)

	def onImplicitOff(self):
		self.currentState = States.ImplicitOff
		command.set_boolean(self.context.id, self.button_id, 0, self.button_command, False)

	def onExplicitOff(self):
		self.currentState = States.ExplicitOff
		command.set_boolean(self.context.id, self.button_id, 0, self.button_command, False)
		dt = datetime.datetime.now() + datetime.timedelta(days=1)
		self.nextImplicitOnStart = dt.replace(hour=7, minute=0, second=0, microsecond=0)

class Context:
	def __init__(self):
		self.id = 0
		self.lum = 0
		self.temp = 0
		self.motion = False
		self.humidity = 0
		self.fsm = FSM(self, 263, 37)

def on_init(id):
	context = Context()
	events.register_context(id, context)
	events.register_device_events(id)
	context.id = id
	
	return True

def on_device_event(context, device_id):
	if context:
		#device_name = resolver.name_from_id(device_id)
		logging.log_debug(context.id, "ON_DEVICE_EVENT: device_id = " + str(device_id))

		if device_id == 263 or device_id == 26:
			events.register_data_events(context.id, device_id)

def on_data_event(context, type, node_id, instance_id, command_id, dataHolder):
		if context:
			logging.log_debug(context.id, "Type: {}, node_id: {}, instance: {}, command: {}, data: {}".format(type, node_id, instance_id, command_id, dataHolder))
			if node_id == 26:
				if command_id == 113:
					updateMotionSensorState(context, node_id, command_id, dataHolder)
				elif command_id == 49:
					updateSensorEnvironment(context, node_id, command_id, dataHolder)
			elif node_id == 263:
				updateVirtualButtonState(context, node_id, command_id, dataHolder)

def updateMotionSensorState(context, node_id, command_id, dataHolder):
	val = json.loads(command.get_value(context.id, node_id, 0, command_id, "7.event"))
	if val['device_data']['event'] == 8:
		context.fsm.changeState(Events.MotionDetected)
	else:
		context.fsm.changeState(Events.NoMotionDetected)
	
def updateSensorEnvironment(context, node_id, command_id, dataHolder):
	tempval = json.loads(command.get_value(context.id, node_id, 0, command_id, "1"))
	humidval = json.loads(command.get_value(context.id, node_id, 0, command_id, "5"))
	lumval = json.loads(command.get_value(context.id, node_id, 0, command_id, "3"))

	context.temp = tempval['device_data']['val']
	context.humidity = humidval['device_data']['val']
	context.lum = lumval['device_data']['val']

def updateVirtualButtonState(context, node_id, command_id, dataHolder):
	val = json.loads(command.get_value(context.id, node_id, 0, command_id))
	if val['device_data']['level']:
		context.fsm.changeState(Events.ButtonOn)
	else:
		context.fsm.changeState(Events.ButtonOff)
