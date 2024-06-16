var Service, Characteristic;
var request = require("request");
var fs = require('fs');
var os = require('os');

var EventSource = require('eventsource');
var eventSource;

var zae_host;
var zae_port;

module.exports = function(homebridge){
	Service = homebridge.hap.Service;
	Characteristic = homebridge.hap.Characteristic;

	homebridge.registerPlatform("homebridge-zae", "ZAE", ZAEPlatform, false);

	console.log("ZAE registered with homebridge");
}

function ZAEPlatform(log, config, api) {
	var platform = this;
	this.log = log;
	this.config = config;
	this.api = api;

	// Load config path to read event URL
	zae_host = config.zae_host;
	zae_port = config.zae_port;
	
	var eventUrl = (config.username)? 
					'http://' + config.username + ':' + config.password + '@' + zae_host + ':' + zae_port + '/rest/v1/sse' :
					'http://' + zae_host + ':' + zae_port + '/rest/v1/sse';

	console.log("EventURL:", eventUrl);
	this.eventSource = new EventSource(eventUrl);
	this.eventSource.setMaxListeners(128);

	this.eventSource.onopen = function () {
	  console.log("Connection opened")
	};

	this.eventSource.onerror = function () {
	  console.log("Connection error");
	};
}

ZAEPlatform.prototype = {
	httpRequest: function(url, body, method, username, password, sendimmediately, callback) {
		request({
			url: url,
			body: body,
			method: method,
			rejectUnauthorized: false,
			auth: {
				user: this.config["username"],
				pass: this.config["password"],
				sendImmediately: sendimmediately
			}
		},
		function(error, response, body) {
			callback(error, response, body)
		})
	},
	accessories: function(callback) {
		this.log("Getting devices...");
		var url = 'http://' + this.config["zae_host"] + ':' + this.config["zae_port"] + '/rest/v1/devices';
		this.httpRequest(url, "", "GET", "", "", true, function(error, response, body) {
				var accessoriesList = [];
				accessoriesList = this.buildAccessoriesList(body);

				this.log("Found devices " + accessoriesList.length);

				callback(accessoriesList);
			}.bind(this));
	},
	buildAccessoriesList : function(response) {
		var accessoriesList = [];
		var json = JSON.parse(response);
		for(var index in json.devices) {
			this.log("buildAccessoriesList " + json.devices[index].node_id);
			// Attempt to create accessory for each device entry...
			if(json.devices[index].command_classes.length > 0)
			{
				accessoriesList.push(new ZAEAccessory(this.log, this.config, json.devices[index], this));
			}
		}

		return accessoriesList;
	}
}

	// Device parameter: 
	// [ { "sensorTypeString": "General purpose", "level": false, "parameters": [ ], "data_holder": "1" } ]
	function ZAEAccessory(log, config, deviceDescriptor, platform) {
		this.log = log;
		this.deviceDescriptor = deviceDescriptor;
		this.platform = platform;
		// install event handler for each command class
		this.nodeId = deviceDescriptor.node_id;

		this.instanceId = deviceDescriptor.instance_id || 0;
		this.name = (deviceDescriptor.descriptor && deviceDescriptor.descriptor.name) || deviceDescriptor.name;
		
		this.uuid_base = "UUID" + this.nodeId + "-" + deviceDescriptor.instance_id;

		this.commands = deviceDescriptor.command_classes;
		this.config = config;
		this.additionalCharacteristics = [];
		this.serviceList = [];

		this.onServiceCreatedCallback = [];

		this.CharacteristicForService = {
			ContactSensor: Characteristic.ContactSensorState,
			MotionSensor: Characteristic.MotionDetected,
			LeakSensor:Characteristic.LeakDetected,
			SmokeSensor: Characteristic.SmokeDetected,
			Outlet: Characteristic.OutletInUse
		};

		/*this.NotificationForService = {
			MotionDetected: Characteristic.MotionDetected,
			LeakDetected: Characteristic.LeakDetected,
			Tampered: Characteristic.StatusTampered,
			SmokeDetected: Characteristic.SmokeDetected
		};*/

		this.CharacteristicForAlarmType = {
			Smoke: Characteristic.SmokeDetected,
			CO: Characteristic.CarbonMonoxideDetected
		};

		for(var index in deviceDescriptor.command_classes) {
			switch(deviceDescriptor.command_classes[index].id) {
				case 48: // Binary Sensor
					var serviceType = (deviceDescriptor.descriptor.roles && deviceDescriptor.descriptor.roles[48]) || "ContactSensor";
					this.createBinarySensorAccessory(deviceDescriptor.command_classes[index], serviceType);
					break;
				case 49: // Sensor Multilevel
					this.createMultiSensorAccessory(deviceDescriptor.command_classes[index]);
					break;
				case 37: // Switch
					var serviceType = (deviceDescriptor.descriptor.roles && deviceDescriptor.descriptor.roles[37]) || "Switch";
					var service = new Service[serviceType](this.name)
					service.getCharacteristic(Characteristic.On)
						.on('get', this.getBinaryState.bind(
							{
								accessory:this,
								instance:deviceDescriptor.command_classes[index].instance,
								commandClass:deviceDescriptor.command_classes[index].id
							}))
						.on('set', this.setBinaryState.bind(
							{
								accessory:this,
								instance:deviceDescriptor.command_classes[index].instance,
								commandClass:deviceDescriptor.command_classes[index].id
							}));
					this.serviceList.push(service);

					this.registerEventListener(Characteristic.On, 
						{
							service:service,
							node_id:this.nodeId,
							instance:deviceDescriptor.command_classes[index].instance,
							command_class:deviceDescriptor.command_classes[index].id,
							dh:"level"
						});

					break;
				case 91: // Scene Controller
					// The device might have multiple buttons and we want to create
					// multiple switch services for each button. The physical number of
					// buttons is unknown but we try to guess it by reading maxScenes value...
					var numButtons = deviceDescriptor.command_classes[index].dh.maxScenes;

					// The scene numbers assumed to be 1 and 2
					for(var sceneIndex = 1; sceneIndex <= numButtons; sceneIndex++) {
						var service = new Service.StatelessProgrammableSwitch(this.name, sceneIndex);
						service.getCharacteristic(Characteristic.ProgrammableSwitchEvent)
							.on('get', function(callback) {callback(null, Characteristic.ProgrammableSwitchEvent.SINGLE_PRESS)});
						
						this.serviceList.push(service);

						this.registerEventListener(Characteristic.ProgrammableSwitchEvent, 
							{
								service:service,
								node_id:this.nodeId,
								instance:deviceDescriptor.command_classes[index].instance,
								command_class:deviceDescriptor.command_classes[index].id,
								dh:"currentScene",
								valueHolder:"currentScene",	// Read event value from this data holder
								force_value:Characteristic.ProgrammableSwitchEvent.SINGLE_PRESS, // Force Set this value to device (SINGLE_PRESS)
								required_value:sceneIndex	// Only react to the event if value equal to required_value
							});

						// We want to listen to keyAttribute change events to distinguish between single press and
						// long press
						this.registerEventListener(Characteristic.ProgrammableSwitchEvent, 
							{
								service:service,
								node_id:this.nodeId,
								instance:deviceDescriptor.command_classes[index].instance,
								command_class:deviceDescriptor.command_classes[index].id,
								dh:"keyAttribute",
								valueHolder:"keyAttribute",	// Read event value from this data holder (instead of a default "level")
								scene_index:sceneIndex,
								accessory:this,
								convert:function(value) { 
									if(value == 0) {
										return Characteristic.ProgrammableSwitchEvent.SINGLE_PRESS;
									} else if(value == 2) {
										return Characteristic.ProgrammableSwitchEvent.LONG_PRESS;
									}
									else return 0;
								},
								callback:function(value) { 
									var boundFunc = this.accessory.getSensorValue.bind({
										dh: "currentScene",
										valueHolder:"currentScene",
										accessory:this.accessory,
										instance:deviceDescriptor.command_classes[index].instance,
										commandClass:91
								 	});

									boundFunc(function(error, value) {
										if(error) {
											return;
										}
										var currentScene = value;
										if(currentScene == this.scene_index) {
											this.service.getCharacteristic(Characteristic.ProgrammableSwitchEvent)
												.setValue(value);
										}
									}.bind({
										scene_index:this.scene_index,
										service:this.service
									}));
								 } 
							});
					}

					break;
				case 128:
					var service = new Service.BatteryService(this.name);
					service.getCharacteristic(Characteristic.ChargingState)
						.on('get', function(callback) {callback(null, Characteristic.ChargingState.NOT_CHARGEABLE)});

					service.getCharacteristic(Characteristic.StatusLowBattery).on('get', this.getSensorValue.bind(
							{
								dh: "last",
								valueHolder:"last",
								accessory:this,
								instance:deviceDescriptor.command_classes[index].instance,
								commandClass:deviceDescriptor.command_classes[index].id,
								convert:function(value) { return (value < 20)? Characteristic.StatusLowBattery.BATTERY_LEVEL_LOW : Characteristic.StatusLowBattery.BATTERY_LEVEL_NORMAL }
							}));

					service.getCharacteristic(Characteristic.BatteryLevel).on('get', this.getSensorValue.bind(
							{
								dh: "last",
								valueHolder:"last",
								accessory:this,
								instance:deviceDescriptor.command_classes[index].instance,
								commandClass:deviceDescriptor.command_classes[index].id
							}));

					this.serviceList.push(service);
					break;
				case 114:
					var service = new Service.AccessoryInformation();
					var serialNumber = deviceDescriptor.command_classes[index].dh.serialNumber || "0";

					if(serialNumber instanceof Array) {
						serialNumber = serialNumber.map(function(byte) {
	    							return ("00" + (byte & 0xFF).toString(16)).slice(-2)
	  							}).join('');
					}

					service
						.setCharacteristic(Characteristic.Manufacturer, deviceDescriptor.command_classes[index].dh.vendor)
						.setCharacteristic(Characteristic.Model, deviceDescriptor.command_classes[index].dh.model || 
																 deviceDescriptor.deviceTypeString || 
																 deviceDescriptor.name)
						.setCharacteristic(Characteristic.SerialNumber, deviceDescriptor.node_id + '-' + serialNumber);
							
					this.serviceList.push(service);
					break;
				case 113:
					var serviceType = (deviceDescriptor.descriptor.roles && deviceDescriptor.descriptor.roles[113]) || '';
					//var notificationList = deviceDescriptor.descriptor.notifications;
					var sensorAlarmList = deviceDescriptor.descriptor.alarmRoles;
					//if(typeof notificationList !== 'undefined' && notificationList.length > 0) {
						this.createNotificationAccessory(deviceDescriptor.command_classes[index], serviceType, sensorAlarmList);
					//}
					break;
				case 156:
					var sensorAlarmList = deviceDescriptor.descriptor.alarmRoles;
					this.log("Sensor Alarm List:" + sensorAlarmList);
					if(typeof sensorAlarmList !== 'undefined') {
						this.log("Creating alarm roles for " + this.name);
						this.createSensorAlarmAccessory(deviceDescriptor.command_classes[index], sensorAlarmList);
					}

					break;
				case 102: // Barrier
					var serviceType = (deviceDescriptor.descriptor.roles && deviceDescriptor.descriptor.roles[102]) || "Disabled";
					this.createBarrierAccessory(deviceDescriptor.command_classes[index], serviceType);					
					break; 
				case 38:  // Switch Multilevel
					var serviceType = (deviceDescriptor.descriptor.roles && deviceDescriptor.descriptor.roles[38]) || "Disabled";
					this.createLightbulbAccessory(deviceDescriptor.command_classes[index], serviceType);
					break;
				case 98:
					this.createDoorLockAccessory(deviceDescriptor.command_classes[index]);
					break;
				case 241: // Custom Command Class for emulation of Security System
					var service = new Service.SecuritySystem(this.name);
					service.getCharacteristic(Characteristic.SecuritySystemCurrentState)
						.on('get', this.getSensorValue.bind(
							{
								dh: "level",
								valueHolder: "level",
								accessory:this,
								instance:deviceDescriptor.command_classes[index].instance,
								commandClass:deviceDescriptor.command_classes[index].id
							}));

					service.getCharacteristic(Characteristic.SecuritySystemTargetState)
							.on('get', this.getSensorValue.bind(
							{
								dh: "level",
								valueHolder: "level",
								accessory:this,
								instance:deviceDescriptor.command_classes[index].instance,
								commandClass:deviceDescriptor.command_classes[index].id
							}))
							.on('set', this.setSensorValue.bind(
							{
								accessory:this,
								instance:deviceDescriptor.command_classes[index].instance,
								commandClass:deviceDescriptor.command_classes[index].id,
							}));

					this.registerEventListener(Characteristic.SecuritySystemCurrentState, 
					{
						service:service,
						node_id:this.nodeId,
						instance:deviceDescriptor.command_classes[index].instance,
						command_class:deviceDescriptor.command_classes[index].id,
						dh:"level",
					});

					this.registerEventListener(Characteristic.SecuritySystemTargetState, 
					{
						service:service,
						node_id:this.nodeId,
						instance:deviceDescriptor.command_classes[index].instance,
						command_class:deviceDescriptor.command_classes[index].id,
						dh:"level",
					});

					this.serviceList.push(service);
					break;
			}
		}

		for(var index in this.onServiceCreatedCallback) {
			this.onServiceCreatedCallback[index]();
		}

		// Unconditionally configure Tampered status
		this.serviceList.forEach(function(service) {
			service.setCharacteristic(Characteristic.StatusTampered, Characteristic.StatusTampered.NOT_TAMPERED);
			this.registerEventListener(Characteristic.StatusTampered, 
				{
					service:service,
					node_id:this.nodeId,
					instance:0,
					command_class: 113,
					dh:7,
					valueHolder: "event",
					convert:function(value) { 
						if(value == 3) {
							return Characteristic.StatusTampered.TAMPERED;
						} else {
							return Characteristic.StatusTampered.NOT_TAMPERED;
						}
					}
				});
		}.bind(this));

		for(var index in this.additionalCharacteristics) {
			this.serviceList.forEach(function(service) {
				service.getCharacteristic(this.additionalCharacteristics[index].characteristic)
						.on('get', this.getSensorValue.bind(this.additionalCharacteristics[index].callback_data));
			}.bind(this));
		}
	
	};

	ZAEAccessory.prototype = {
		identify: function(callback) {
			this.log("Identify requested for " + this.name + " NodeID: " + this.nodeId);
			callback(); // success
		},
		registerEventListener : function(Characteristics, data) {
			this.log("Add event listener for device " +  this.name + " command_id " + data.command_class);
			this.platform.eventSource.addEventListener('EventLogAddedEvent'+this.nodeId, function(e) {
				//console.log(e.data);
    			var json = JSON.parse(e.data);
				var type = json.type;
				var instance = json.instance_id;
				var command_id = json.command_id;
				var dh = json.data.data_holder;
				//console.log("Event: " + command_id + " " + json.node_id + " " + dh)
				if(command_id == this.command_class && json.node_id == this.node_id && dh == this.dh && instance == this.instance)  {
					//console.log("Event accepted " + command_id + " " + json.node_id + " " + dh);
					var sensorValue = json.data[this.valueHolder || "level"];

					var alarmTypeValue = '';
					if(typeof this.alarmType !== 'undefined')
					{
						alarmTypeValue = json.data[this.alarmType];
					}

					if(typeof this.convert === 'function') {
						sensorValue = this.convert(sensorValue, alarmTypeValue);
					}

					if(typeof this.force_value !== 'undefined') {
						//console.log("Event: Setting force value to " + this.force_value);
						if(typeof this.required_value === 'undefined' || (this.required_value == sensorValue)) {
							if(typeof this.callback === 'function') {
								this.callback(this.force_value);
							} else {
								this.service.getCharacteristic(Characteristics).updateValue(this.force_value);
							}
						}
					} else {
						//console.log("Event: Setting Value to " + sensorValue);

						if(typeof this.required_value === 'undefined' || (this.required_value == sensorValue)) {
							if(typeof this.callback === 'function') {
								this.callback(sensorValue);
							} else {
								//console.log("!!! Event: Setting Value to " + sensorValue);
								this.service.getCharacteristic(Characteristics).updateValue(sensorValue);
							}
						}
					}
				}
			}.bind(data), false);
		},
		createBinarySensorAccessory : function(commandClass, serviceType) {
			if(serviceType != 'Disabled' &&  commandClass.dh.parameters[0] != undefined)
			{
				var service = new Service[serviceType](this.name);
				service.subtype = "node"+this.nodeId+"instance"+commandClass.instance;
				service.getCharacteristic(this.CharacteristicForService[serviceType]).on('get', this.getBinaryState.bind(
						{
							dh: commandClass.dh.parameters[0].data_holder,
							accessory:this,
							instance:commandClass.instance,
							commandClass:commandClass.id
						}));
				this.serviceList.push(service);

				this.registerEventListener(this.CharacteristicForService[serviceType], 
					{
						service:service,
						node_id:this.nodeId,
						instance:commandClass.instance,
						command_class:commandClass.id,
						dh:commandClass.dh.parameters[0].data_holder
					});
			} else {
				this.log("Service 48 is disabled for " + this.name);
			}
		},
		createLightbulbAccessory : function(commandClass, serviceType) {
			if(serviceType != 'Disabled')
			{
				var service = new Service[serviceType](this.name);
				service.subtype = "node"+this.nodeId+"instance"+commandClass.instance;
				service.getCharacteristic(Characteristic.On).on('get', this.getSensorValue.bind(
				{
					dh: "level",
					valueHolder: "level",
					accessory:this,
					instance:commandClass.instance,
					commandClass: commandClass.id,
					convert:function(value) { 
							return value > 0;
						}
				}))
				.on('set', this.setSensorValue.bind(
				{
					accessory:this,
					instance:commandClass.instance,
					commandClass:commandClass.id,
					convert: function(value) {
						if(value) {
							return 100;
						} else {
							return 0;
						}
									
					}
				}));

				service.getCharacteristic(Characteristic.Brightness).on('get', this.getSensorValue.bind(
				{
					dh: "level",
					valueHolder: "level",
					accessory:this,
					instance:commandClass.instance,
					commandClass: commandClass.id,
				}))
				.on('set', this.setSensorValue.bind(
				{
					accessory:this,
					instance:commandClass.instance,
					commandClass:commandClass.id,
				}));

				this.registerEventListener(Characteristic.On, 
					{
						service:service,
						node_id:this.nodeId,
						instance:commandClass.instance,
						command_class:commandClass.id,
						dh:"level",
						valueHolder:"level",
						convert:function(value) {
							return value > 0;
						}
					});
				this.registerEventListener(Characteristic.Brightness, 
					{
						service:service,
						node_id:this.nodeId,
						instance:commandClass.instance,
						command_class:commandClass.id,
						dh:"level",
						valueHolder:"level"
					});

				this.serviceList.push(service);
			}
		},
		createBarrierAccessory : function(commandClass, serviceType) {
			if(serviceType != 'Disabled') 
			{
				var service = new Service[serviceType](this.name);
				service.subtype = "node"+this.nodeId+"instance"+commandClass.instance;
				service.getCharacteristic(Characteristic.CurrentDoorState).on('get', this.getSensorValue.bind(
								{
									dh: "state",
									valueHolder: "state",
									accessory:this,
									instance:commandClass.instance,
									commandClass: commandClass.id,
									convert:function(value) { 
										if(value == 255) {
											return Characteristic.CurrentDoorState.OPEN;
										} else if(value == 0) {
											return Characteristic.CurrentDoorState.CLOSED;
										} else if(value == 254) {
											return Characteristic.CurrentDoorState.OPENING;
										} else if(value == 252) {
											return Characteristic.CurrentDoorState.CLOSING;
										}
									}
								}));
								
				service.getCharacteristic(Characteristic.TargetDoorState)
								.on('get', (callback) => {
									var targetDoorState = service.getCharacteristic(Characteristic.CurrentDoorState).value;
									callback(null, targetDoorState);
								})
								.on('set', this.setSensorValue.bind(
								{
									accessory:this,
									instance:commandClass.instance,
									commandClass:commandClass.id,
									convert: function(value) {
										//console.log("Value: " + value);
										if(value == Characteristic.TargetDoorState.CLOSED) {
											return 0;
										} else if(value == Characteristic.TargetDoorState.OPEN){ 
											return 255;
										}
									}
								}));

				service.setCharacteristic(Characteristic.ObstructionDetected, false);

				this.registerEventListener(Characteristic.CurrentDoorState,
					{
						service:service,
						node_id:this.nodeId,
						instance:commandClass.instance,
						command_class:commandClass.id,
						dh:"state",
						valueHolder:"state",
						convert:function(value) {
							if(value == 255) {
								//console.log("DOOR OPEN");
								service.getCharacteristic(Characteristic.TargetDoorState).updateValue(Characteristic.TargetDoorState.OPEN);
								return Characteristic.CurrentDoorState.OPEN;
							} else if(value == 0) {
								//console.log("DOOR CLOSED");
								service.getCharacteristic(Characteristic.TargetDoorState).updateValue(Characteristic.TargetDoorState.CLOSED);
								return Characteristic.CurrentDoorState.CLOSED;
							} else if(value == 254) {
								//console.log("DOOR OPENING");
								return Characteristic.CurrentDoorState.OPENING;
							} else if(value == 252) {
								//console.log("DOOR CLOSING");
								return Characteristic.CurrentDoorState.CLOSING;
							}
						}
					});
				this.serviceList.push(service);
			}
		},
		createDoorLockAccessory : function(commandClass) {
			var service = new Service.LockMechanism(this.name);
			service.subtype = "node"+this.nodeId+"instance"+commandClass.instance;

			service.getCharacteristic(Characteristic.LockCurrentState).on('get', this.getSensorValue.bind(
				{
					dh: "mode",
					valueHolder: "mode",
					accessory:this,
					instance:commandClass.instance,
					commandClass: commandClass.id,
					convert:function(value) { 
						if(value == 255 ) {
							return Characteristic.LockCurrentState.SECURED;
						} else if(value == 0) {
							return Characteristic.LockCurrentState.UNSECURED;
						} else {
							// Need to read event:
							return "alternative";
						}
					},
					alternativeBindings: {
						commandClass: 113,
						dh:6,
						valueHolder: "event",
						convert:function(value) { 
							if(value == 3 || value == 1 || value == 9) {
								service.getCharacteristic(Characteristic.LockTargetState).updateValue(Characteristic.LockTargetState.SECURED);
								return Characteristic.LockCurrentState.SECURED;
							} else if (value == 4 || value == 2 || value == 6){
								service.getCharacteristic(Characteristic.LockTargetState).updateValue(Characteristic.LockTargetState.UNSECURED);
								return Characteristic.LockCurrentState.UNSECURED;
							} else if (value == 11) {
								service.getCharacteristic(Characteristic.LockTargetState).updateValue(Characteristic.LockTargetState.JAMMED);
								return Characteristic.LockCurrentState.JAMMED;
							} else {
								return Characteristic.LockCurrentState.UNKNOWN;
							}
						}
					}
				}));
				
			service.getCharacteristic(Characteristic.LockTargetState)
							.on('get', (callback) => {
								var targetLockState = service.getCharacteristic(Characteristic.LockCurrentState).value;
								callback(null, targetLockState);
							})
							.on('set', this.setSensorValue.bind(
							{
								accessory:this,
								instance:commandClass.instance,
								commandClass:commandClass.id,
								convert: function(value) {
									//console.log("Value: " + value);
									if(value == Characteristic.LockTargetState.SECURED) {
										return 255;
									} else if(value == Characteristic.LockTargetState.UNSECURED){ 
										return 0;
									}
								}
							}));

			this.registerEventListener(Characteristic.LockCurrentState,
				{
					service:service,
					node_id:this.nodeId,
					instance:commandClass.instance,
					command_class:commandClass.id,
					dh:"mode",
					valueHolder:"mode",
					convert:function(value) {
						if(value == 255 || value == 1) {
							return Characteristic.LockCurrentState.SECURED;
						} else if(value == 0) {
							return Characteristic.LockCurrentState.UNSECURED;
						} else {
							return Characteristic.LockCurrentState.UNKNOWN;
						}
					}
				});
				this.registerEventListener(Characteristic.LockCurrentState, 
					{
						service:service,
						node_id:this.nodeId,
						instance:commandClass.instance,
						command_class: 113,
						dh:6,
						valueHolder: "event",
						convert:function(value) { 
							if(value == 3 || value == 1 || value == 9) {
								service.getCharacteristic(Characteristic.LockTargetState).updateValue(Characteristic.LockTargetState.SECURED);
								return Characteristic.LockCurrentState.SECURED;
							} else if (value == 4 || value == 2 || value == 6){
								service.getCharacteristic(Characteristic.LockTargetState).updateValue(Characteristic.LockTargetState.UNSECURED);
								return Characteristic.LockCurrentState.UNSECURED;
							} else if (value == 11) {
								//service.getCharacteristic(Characteristic.LockTargetState).updateValue(Characteristic.LockTargetState.JAMMED);
								return Characteristic.LockCurrentState.JAMMED;
							} else {
								return Characteristic.LockCurrentState.UNKNOWN;
							}
						}
					});
			this.serviceList.push(service);
		},
		createNotificationAccessory : function(commandClass, serviceType, sensorAlarmList) {
			// Alert notification supported. Lets see what services and characteristics
			// we can create for this device
			if(serviceType != 'Disabled') {
				this.log("Creating notification for " + serviceType);
				switch(serviceType) {
					case 'MotionSensor':
						var service = new Service.MotionSensor(this.name);
						service.subtype = "node"+this.nodeId+"instance"+commandClass.instance;
						service.getCharacteristic(this.CharacteristicForService[serviceType]).on('get', this.getSensorValue.bind(
							{
								dh: 7,
								valueHolder: "event",
								accessory:this,
								instance:commandClass.instance,
								commandClass: 113,
								convert:function(value) { 
									if(value == 8) {
										return true;
									} else {
										return false;
									}
								}
							}));
						this.serviceList.push(service);

						this.registerEventListener(this.CharacteristicForService[serviceType], 
							{
								service:service,
								node_id:this.nodeId,
								instance:commandClass.instance,
								command_class: 113,
								dh:7,
								valueHolder: "event",
								convert:function(value) { 
									if(value == 8) {
										return true;
									} else {
										return false;
									}
								}
							});
					break;
					case 'SmokeSensor':
						// This might be more than just Smoke detector... 
						// Lets check tho sensor alarm list
						if(typeof sensorAlarmList !== 'undefined')
						{
							for(var  index in sensorAlarmList) {
								var sensorAlarm = sensorAlarmList[index];
								this.log("Index: " + index + " alarm: " + sensorAlarm);
								switch(sensorAlarm) {
									case 'Smoke':
										var service = new Service.SmokeSensor(this.name);
										service.subtype = "node"+this.nodeId+"instance"+commandClass.instance+"index"+index;
										service.getCharacteristic(this.CharacteristicForAlarmType[sensorAlarm]).on('get', this.getAlarmV1Value.bind(
											{
												dh: "V1event",
												valueHolder: "level",
												alarmType: "alarmType",
												expectedAlarmType: index,
												accessory:this,
												instance:commandClass.instance,
												commandClass: 113,
												convert:function(value, alarmTypeValue) { 
													//this.accessory.log("Level is " + value + " and alarmType is " + alarmTypeValue + " and expected type is " + this.expectedAlarmType);

													if(value == 255 && alarmTypeValue == this.expectedAlarmType) {
														return Characteristic.SmokeDetected.SMOKE_DETECTED;
													} else {
														return Characteristic.SmokeDetected.SMOKE_NOT_DETECTED;
													}
												}
											}));
										this.serviceList.push(service);
				
										this.registerEventListener(this.CharacteristicForAlarmType[sensorAlarm], 
											{
												service:service,
												node_id:this.nodeId,
												instance:commandClass.instance,
												command_class: 113,
												dh:"V1event",
												valueHolder: "level",
												alarmType: "alarmType",
												expectedAlarmType: index,
												convert:function(value, alarmTypeValue) { 
													if(value == 255 && this.expectedAlarmType == alarmTypeValue) {
														return Characteristic.SmokeDetected.SMOKE_DETECTED;
													} else {
														return Characteristic.SmokeDetected.SMOKE_NOT_DETECTED;
													}
												}
											});
									break;
									case 'CO':
										var service = new Service.CarbonMonoxideSensor(this.name);
										service.subtype = "node"+this.nodeId+"instance"+commandClass.instance+"index"+index;
										service.getCharacteristic(this.CharacteristicForAlarmType[sensorAlarm]).on('get', this.getAlarmV1Value.bind(
											{
												dh: "V1event",
												valueHolder: "level",
												alarmType: "alarmType",
												expectedAlarmType: index,
												accessory:this,
												instance:commandClass.instance,
												commandClass: 113,
												convert:function(value, alarmTypeValue) { 
													//this.accessory.log("Level is " + value + " and alarmType is " + alarmTypeValue + " and expected type is " + this.expectedAlarmType);

													if(value == 255 && alarmTypeValue == this.expectedAlarmType) {
														return Characteristic.CarbonMonoxideDetected.CO_LEVELS_ABNORMAL;
													} else {
														return Characteristic.CarbonMonoxideDetected.CO_LEVELS_NORMAL;
													}
												}
											}));
										this.serviceList.push(service);
				
										this.registerEventListener(this.CharacteristicForAlarmType[sensorAlarm], 
											{
												service:service,
												node_id:this.nodeId,
												instance:commandClass.instance,
												command_class: 113,
												dh:"V1event",
												valueHolder: "level",
												alarmType: "alarmType",
												expectedAlarmType: index,
												convert:function(value, alarmTypeValue) { 
													if(value == 255 && alarmTypeValue == this.expectedAlarmType) {
														return Characteristic.CarbonMonoxideDetected.CO_LEVELS_ABNORMAL;
													} else {
														return Characteristic.CarbonMonoxideDetected.CO_LEVELS_NORMAL;
													}
												}
											});
									break;
								}
							}
						} else {
							var service = new Service.SmokeSensor(this.name);
							service.subtype = "node"+this.nodeId+"instance"+commandClass.instance;
							service.getCharacteristic(this.CharacteristicForService[serviceType]).on('get', this.getSensorValue.bind(
								{
									dh: "V1event",
									valueHolder: "level",
									accessory:this,
									instance:commandClass.instance,
									commandClass: 113,
									convert:function(value) { 
										if(value == 255) {
											return Characteristic.SmokeDetected.SMOKE_DETECTED;
										} else {
											return Characteristic.SmokeDetected.SMOKE_NOT_DETECTED;
										}
									}
								}));
							this.serviceList.push(service);

							this.registerEventListener(this.CharacteristicForService[serviceType], 
								{
									service:service,
									node_id:this.nodeId,
									instance:commandClass.instance,
									command_class: 113,
									dh:"V1event",
									valueHolder: "level",
									convert:function(value) { 
										if(value == 255) {
											return Characteristic.SmokeDetected.SMOKE_DETECTED;
										} else {
											return Characteristic.SmokeDetected.SMOKE_NOT_DETECTED;
										}
									}
								});
						}
					break;
					/*case 'LockMechanism':
						var service = new Service.LockMechanism(this.name);
						service.subtype = "node"+this.nodeId+"instance"+commandClass.instance+"LockMechanism";

						service.getCharacteristic(Characteristic.LockCurrentState).on('get', this.getSensorValue.bind(
							{
								dh: 6,
								valueHolder: "event",
								accessory:this,
								instance:commandClass.instance,
								commandClass: 113,
								convert:function(value) { 
									if(value == 3) {
										return Characteristic.LockCurrentState.SECURED;
									} else {
										return Characteristic.LockCurrentState.UNSECURED;
									}
								}
							}));
						this.serviceList.push(service);

						this.registerEventListener(Characteristic.LockCurrentState, 
							{
								service:service,
								node_id:this.nodeId,
								instance:commandClass.instance,
								command_class: 113,
								dh:6,
								valueHolder: "event",
								convert:function(value) { 
									if(value == 3) {
										return Characteristic.LockCurrentState.SECURED;
									} else {
										return Characteristic.LockCurrentState.UNSECURED;
									}
								}
							});
					break;*/

					/*case 'Water':
						var existingService = this.serviceList.find(function(element) {
							return typeof element === Service.LeakSensor;
						});

						if(typeof existingService === 'undefined') {
							var service = new Service.LeakSensor(this.name);
							service.subtype = "node"+this.nodeId+"instance"+commandClass.instance;
							service.getCharacteristic(this.NotificationForService[notification]).on('get', this.getSensorValue.bind(
								{
									dh: 5,
									valueHolder: "event",
									accessory:this,
									instance:commandClass.instance,
									commandClass: 113,
									convert:function(value) { 
										if(value == 2) {
											return Characteristic.LeakDetected.LEAK_DETECTED;
										} else {
											return Characteristic.LeakDetected.LEAK_NOT_DETECTED;
										}
									}
								}));
							this.serviceList.push(service);

							this.registerEventListener(this.NotificationForService[notification], 
								{
									service:service,
									node_id:this.nodeId,
									instance:commandClass.instance,
									command_class: 113,
									dh:5,
									valueHolder: "event",
									convert:function(value) { 
										if(value == 2) {
											return Characteristic.LeakDetected.LEAK_DETECTED;
										} else {
											return Characteristic.LeakDetected.LEAK_NOT_DETECTED;
										}
									}
								});
						}
					break;
					*/
				}
			}
		},
		createSensorAlarmAccessory : function(commandClass, sensorAlarmList) {
			for(var  index in sensorAlarmList) {
				var sensorAlarm = sensorAlarmList[index];
				if(typeof commandClass.dh[index] !== 'undefined') {
					this.log("Creating sensor alarm for " + sensorAlarm);
					switch(sensorAlarm) {
						case 'Tampered':
							this.onServiceCreatedCallback.push(function() { 
								this.serviceList.forEach(function(service) {
									this.registerEventListener(Characteristic.StatusTampered, 
										{
											service:service,
											node_id:this.nodeId,
											instance:commandClass.instance,
											command_class: commandClass.id,
											dh:0,
											valueHolder: "sensorState",
											convert:function(value) { 
												if(value == 255) {
													return Characteristic.StatusTampered.TAMPERED;
												} else {
													return Characteristic.StatusTampered.NOT_TAMPERED;
												}
											}
										});
								}.bind(this))}.bind(this));
							break;
						case 'LeakDetected':
							var service = new Service.LeakSensor(this.name);
							service.subtype = "node"+this.nodeId+"instance"+commandClass.instance;
							service.getCharacteristic(Characteristic[sensorAlarm]).on('get', this.getSensorValue.bind(
								{
									dh: 5,
									valueHolder: "sensorState",
									accessory:this,
									instance:commandClass.instance,
									commandClass: commandClass.id,
									convert:function(value) { 
										if(value == 255) {
											return Characteristic.LeakDetected.LEAK_DETECTED;
										} else {
											return Characteristic.LeakDetected.LEAK_NOT_DETECTED;
										}
									}
								}));
							this.serviceList.push(service);

							this.registerEventListener(Characteristic[sensorAlarm], 
								{
									service:service,
									node_id:this.nodeId,
									instance:commandClass.instance,
									command_class: commandClass.id,
									dh:5,
									valueHolder: "sensorState",
									convert:function(value) { 
										if(value == 255) {
											return Characteristic.LeakDetected.LEAK_DETECTED;
										} else {
											return Characteristic.LeakDetected.LEAK_NOT_DETECTED;
										}
									}
								});
							break;
					}
				}
			}
		},
		createMultiSensorAccessory : function(commandClass) {
			for(var index in commandClass.dh.parameters) {
				switch(commandClass.dh.parameters[index].data_holder) {
					case "1":
						console.log("Adding temperature sensor for " + this.nodeId);
						// Temperature...
						var service = new Service.TemperatureSensor(commandClass.dh.parameters[index].sensorTypeString);
						service.subtype = "node"+this.nodeId+"instance"+commandClass.instance;
						service.getCharacteristic(Characteristic.CurrentTemperature).on('get', this.getSensorValue.bind(
								{
									dh: 1,
									accessory:this,
									instance:commandClass.instance,
									commandClass:commandClass.id,
									convert:function(value) { return (value - 32) * 5/9; }
								}));
						this.serviceList.push(service);

						this.registerEventListener(Characteristic.CurrentTemperature, 
							{
								service:service,
								node_id:this.nodeId,
								instance:commandClass.instance,
								command_class:commandClass.id,
								dh:1,
								valueHolder:"val",
								convert:function(value) { return (value - 32) * 5/9; }
							});
						break;
					case "3":
						console.log("Adding luminiscense sensor for " + this.nodeId);
						// Luminiscense
						var service = new Service.LightSensor(commandClass.dh.parameters[index].sensorTypeString);
						service.subtype = "node"+this.nodeId+"instance"+commandClass.instance;
						service.getCharacteristic(Characteristic.CurrentAmbientLightLevel).on('get', this.getSensorValue.bind(
							{
								dh: 3,
								accessory:this,
								instance:commandClass.instance,
								commandClass:commandClass.id
							}));
						this.serviceList.push(service);
						this.registerEventListener(Characteristic.CurrentAmbientLightLevel, 
							{
								service:service,
								node_id:this.nodeId,
								instance:commandClass.instance,
								command_class:commandClass.id,
								dh:3,
								valueHolder:"val"
							});
						break;
					case "5":
						console.log("Adding humidity sensor for " + this.nodeId);
						// Humidity
						var service = new Service.HumiditySensor(commandClass.dh.parameters[index].sensorTypeString);
						service.subtype = "node"+this.nodeId+"instance"+commandClass.instance;
						service.getCharacteristic(Characteristic.CurrentRelativeHumidity).on('get', this.getSensorValue.bind(
							{
								dh: 5,
								instance:commandClass.instance,
								commandClass:commandClass.id,
								accessory:this
							}));
						this.serviceList.push(service);
						this.registerEventListener(Characteristic.CurrentRelativeHumidity, 
							{
								service:service,
								node_id:this.nodeId,
								instance:commandClass.instance,
								command_class:commandClass.id,
								dh:5,
								valueHolder:"val"
							});
						break;

				}
			}
		},
		getServices : function() {
			var that = this;
		
			/*var informationService = new Service.AccessoryInformation();
	
			informationService
			.setCharacteristic(Characteristic.Manufacturer, "Alex")
			.setCharacteristic(Characteristic.Model, "ZWAVE Automation Engine")
			.setCharacteristic(Characteristic.SerialNumber, "0");


			this.serviceList.push(informationService);*/

			return this.serviceList;
		},
		getBinaryState: function(callback) {
			var url = 'http://' + this.accessory.config.zae_host + ':' + this.accessory.config.zae_port + '/rest/v1/devices/'
				+ this.accessory.nodeId + '/' + this.instance + '/' + this.commandClass;

			if(this.dh) {
				url += '/' + this.dh;
			}

			this.accessory.log("Getting Binary state from " + url);
			
			this.accessory.platform.httpRequest(url, "", "GET", "", "", true, function(error, response, responseBody) {
				if (error) {
					this.accessory.log('HTTP get bonary state function failed: %s', error.message);
					callback(error);
				} else {
					var json = JSON.parse(responseBody);

					var binaryState = json.device_data.level == true || json.device_data.level > 0;

					this.accessory.log("Sensor " + this.accessory.nodeId + " CC " + this.commandClass + " binary state is currently %s", binaryState);
					callback(null, binaryState);
				}
			}.bind(this));
		},
		setBinaryState: function(isTrue, callback) {
			var url = 'http://' + this.accessory.config.zae_host + ':' + this.accessory.config.zae_port + '/rest/v1/devices/'
				+ this.accessory.nodeId + '/' + this.instance + '/' + this.commandClass;
			this.accessory.log("Setting Binary state at " + url);
			
			var body = "{\"command\":\"Set\",\"arguments\":[" + isTrue + "]}}";

			this.accessory.platform.httpRequest(url, body, "POST", "", "", true, function(error, response, responseBody) {
				if (error) {
					callback(error);
				} else {
					callback(null);
				}
			}.bind(this));
		},
		getSensorValue: function(callback) {
			//this.dh = 1;
			//console.log(JSON.stringify(this, null, 4));
			var url = 'http://' + this.accessory.config.zae_host + ':' + this.accessory.config.zae_port + '/rest/v1/devices/'
				+ this.accessory.nodeId + '/' + this.instance + '/' + this.commandClass + '/' + this.dh;
			this.accessory.log("Getting Sensor value: " + url);
			
			this.accessory.platform.httpRequest(url, "", "GET", "", "", true, function(error, response, responseBody) {
				if (error) {
					this.accessory.log('HTTP get sensor value function failed: %s', error.message);
					callback(error);
				} else {
					var json = JSON.parse(responseBody);

					var sensorValue = (this.valueHolder)? json.device_data[this.valueHolder] : json.device_data.val;
					if(typeof this.convert === 'function') {
						sensorValue = this.convert(sensorValue);
					}

					if(sensorValue != "alternative")
					{
						this.accessory.log("Sensor " + this.accessory.nodeId + " CC " + this.commandClass + " value is %s", sensorValue);
						callback(null, sensorValue);
					}
					else
					{
						this.accessory.getAlternativeSensorValue(this, callback);
					}
				}
			}.bind(this));
		},
		getAlternativeSensorValue : function(obj, callback) {
			var url = 'http://' + obj.accessory.config.zae_host + ':' + obj.accessory.config.zae_port + '/rest/v1/devices/'
				+ obj.accessory.nodeId + '/' + obj.instance + '/' + obj.alternativeBindings.commandClass + '/' + obj.alternativeBindings.dh;
				obj.accessory.log("Getting Sensor value: " + url);
			
				obj.accessory.platform.httpRequest(url, "", "GET", "", "", true, function(error, response, responseBody) {
				if (error) {
					obj.accessory.log('HTTP get sensor value function failed: %s', error.message);
					callback(error);
				} else {
					var json = JSON.parse(responseBody);

					var sensorValue = (obj.alternativeBindings.valueHolder)? json.device_data[obj.alternativeBindings.valueHolder] : json.device_data.val;
					if(typeof obj.alternativeBindings.convert === 'function') {
						sensorValue = obj.alternativeBindings.convert(sensorValue);
					}

					obj.accessory.log("Sensor " + obj.accessory.nodeId + " CC " + obj.alternativeBindings.commandClass + " value is %s", sensorValue);
					callback(null, sensorValue);
				}
			}.bind(obj));
		},
		getAlarmV1Value: function(callback) {
			//this.dh = 1;
			//console.log(JSON.stringify(this, null, 4));
			var url = 'http://' + this.accessory.config.zae_host + ':' + this.accessory.config.zae_port + '/rest/v1/devices/'
				+ this.accessory.nodeId + '/' + this.instance + '/' + this.commandClass + '/' + this.dh;
			this.accessory.log("Getting Sensor value: " + url);
			
			this.accessory.platform.httpRequest(url, "", "GET", "", "", true, function(error, response, responseBody) {
				if (error) {
					this.accessory.log('HTTP get alarm function failed: %s', error.message);
					callback(error);
				} else {
					var json = JSON.parse(responseBody);

					var sensorValue = (this.valueHolder)? json.device_data[this.valueHolder] : json.device_data.val;
					var alarmTypeValue = (this.alarmType)? json.device_data[this.alarmType] : '';
					if(typeof this.convert === 'function') {
						sensorValue = this.convert(sensorValue, alarmTypeValue);
					}

					this.accessory.log("Sensor " + this.accessory.nodeId + " CC " + this.commandClass + " value is %s", sensorValue);
					callback(null, sensorValue);
				}
			}.bind(this));
		},
		setSensorValue: function(value, callback) {
			var url = 'http://' + this.accessory.config.zae_host + ':' + this.accessory.config.zae_port + '/rest/v1/devices/'
				+ this.accessory.nodeId + '/' + this.instance + '/' + this.commandClass;

			var sensorValue = value;
			if(typeof this.convert === 'function') {
				sensorValue = this.convert(sensorValue);
			}
			this.accessory.log("Setting Sensor value at " + url + " to " + sensorValue);

			var body = "{\"command\":\"Set\",\"arguments\":[" + sensorValue + "]}}";

			this.accessory.platform.httpRequest(url, body, "POST", "", "", true, function(error, response, responseBody) {
				if (error) {
					callback(error);
				} else {
					callback(null);
				}
			}.bind(this));
		}
	}