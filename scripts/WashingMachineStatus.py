import logging
import events
import command
import json
import datetime
from enum import Enum

# States:
# Machine Alert Off-Pending, Machine Alert Off, Machine Alert On-Pending, Machine Alert On, Machine Alert Off Standby

# Events:
# Watt Above 2, Watt Below 2

# Initial state:
# Machine Alert Off

# Transitions:
# Machine Alert Off -> Watt Above 2 -> Machine Alert Off Standby
# Machine Alert Off -> Watt Below 2 -> Machine Alert Off 

# Machine Alert On-Pending -> Watt Above 2 -> Machine Alert Off Standby
# Machine Alert On-Pending -> Watt Below 2 -> Machine Alert On

# Machine Alert On -> Watt Above 2 -> Machine Alert Off Standby
# Machine Alert On -> Watt Below 2 -> Machine Alert Off-Pending

# Machine Alert Off-Pending -> Watt Above 2 -> Machine Alert Off 
# Machine Alert Off-Pending -> Watt Below 2 -> Machine Alert Off 

# Machine Alert Off Standby -> Watt Above 2 -> Macine Alert Off Standby
# Machine Alert Off Standby -> Watt Below 2 -> Machine Alert On-Pending

class States(Enum):
    AlertOffPending = 1
    AlertOff = 2
    AlertOnPending = 3
    AlertOn = 4
    AlertOffStandby = 5

class Events(Enum):
    WattAboveThreshold = 1
    WattBelowThreshold = 2

class FSM:
    def __init__(self, context, button_id, button_command):
        self.stateTransitions = {
            States.AlertOff: {
                Events.WattAboveThreshold: [self.Noop, States.AlertOffStandby],
                Events.WattBelowThreshold: [self.Noop, States.AlertOff],
            },

            States.AlertOnPending: {
                Events.WattAboveThreshold: [self.Noop, States.AlertOffStandby],
                Events.WattBelowThreshold: [self.MachineAlertOn, States.AlertOn]
            },

            States.AlertOn: {
                Events.WattAboveThreshold: [self.Noop, States.AlertOffStandby],
                Events.WattBelowThreshold: [self.Noop, States.AlertOffPending]
            },

			States.AlertOffPending: { 
				Events.WattAboveThreshold: [self.Noop, States.AlertOffStandby],
			 	Events.WattBelowThreshold: [self.MachineAlertOff, States.AlertOff],
			},

            States.AlertOffStandby: {
                Events.WattAboveThreshold: [self.Noop, States.AlertOffStandby],
                Events.WattBelowThreshold: [self.Noop, States.AlertOnPending]
            }
        }

        self.currentState = States.AlertOff
        self.context = context
        self.button_id = button_id
        self.button_command = button_command

        def changeState(self, event):
            state = self.currentState
            self.stateTransitions[self.currentState][event][0]()
            self.currentState = self.stateTransitions[self.currentState][event][1]
            logging.log_info(self.context.id, "Switched state from " + str(state) + " on event " + str(event) + " to " + str(self.currentState))

        def MachineAlertOff(self):
            command.set_boolean(self.context.id, self.button_id, 0, self.button_command, False)

        def MachineAlertOn(self):
            command.set_boolean(context.id, self.button_id, 0, self.button_command, True)

        def Noop(self):
            pass


class Context:
    def __init__(self):
        self.id = 0
        self.watt = 0
        #self.watt_prev = 0
        self.amp = 0
        self.volt = 0
        self.kwh = False
        self.fsm = None


def on_init(id):
    context = Context()
    events.register_context(id, context)
    events.register_device_events(id)
    context.id = id
    context.fsm = FSM(context, 264, 48)
    return True

def on_device_event(context, device_id):
	if context:
		#device_name = resolver.name_from_id(device_id)
		logging.log_debug(
		    context.id, "ON_DEVICE_EVENT: device_id = " + str(device_id))

		if device_id == 24 or device_id == 264:
			events.register_data_events(context.id, device_id)


def on_data_event(context, type, node_id, instance_id, command_id, dataHolder):
    if context:
        #logging.log_debug(context.id, "Type: {}, node_id: {}, instance: {}, command: {}, data: {}".format(
        #    type, node_id, instance_id, command_id, dataHolder))
        if node_id == 24:
            if command_id == 50:
                updatePowerMeter(context, node_id, command_id, dataHolder)


def updatePowerMeter(context, node_id, command_id, dataHolder):
    val = json.loads(dataHolder)
    #logging.log_debug(context.id, "dataHolder = " + str(dataHolder))
    if val['scale'] == 2:
        #context.watt_prev = context.watt
        context.watt = val['val']
        updateButtonState(context)
    elif val['scale'] == 4:
        context.volt = val['val']
    elif val['scale'] == 5:
        context.amp = val['val']
    elif val['scale'] == 0:
        context.kwh = val['val']

def updateButtonState(context):
    #if context.watt < 2 and context.watt_prev > 2:
    #    command.set_boolean(context.id, 264, 0, 48, True)
    #else:
    #    command.set_boolean(context.id, 264, 0, 48, False)

    if context.watt < 2:
        context.fsm.changeState(Events.WattBelowThreshold)
    else:
        context.fsm.changeState(Events.WattAboveThreshold)
