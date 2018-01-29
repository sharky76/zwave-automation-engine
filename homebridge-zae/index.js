var Service, Characteristic;
var request = require("request");
//var pollingtoevent = require('polling-to-event');
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
	var eventUrl = 'http://' + zae_host + ':' + zae_port + '/rest/v1/sse';

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
				user: username,
				pass: password,
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
			accessoriesList.push(new ZAEAccessory(this.log, this.config, json.devices[index], this));
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
		this.name = deviceDescriptor.name;
		this.commands = deviceDescriptor.command_classes;
		this.config = config;
		this.additionalCharacteristics = [];
		this.serviceList = [];

		this.onServiceCreatedCallback = [];

		this.CharacteristicForService = {
			ContactSensor: Characteristic.ContactSensorState,
			MotionSensor: Characteristic.MotionDetected,
			LeakSensor:Characteristic.LeakDetected
		};

		this.NotificationForService = {
			MotionDetected: Characteristic.MotionDetected,
			LeakDetected: Characteristic.LeakDetected,
			Tampered: Characteristic.StatusTampered
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
					var service = new Service.Switch(this.name)
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
							dh:1
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
						.setCharacteristic(Characteristic.Model, deviceDescriptor.deviceTypeString || deviceDescriptor.name)
						.setCharacteristic(Characteristic.SerialNumber, serialNumber);
							
					this.serviceList.push(service);
					break;
				case 113:
					var notificationList = deviceDescriptor.descriptor.notifications;
					if(typeof notificationList !== 'undefined' && notificationList.length > 0) {
						this.createNotificationAccessory(deviceDescriptor.command_classes[index], notificationList);
					}
					break;
				case 156:
					var sensorAlarmList = deviceDescriptor.descriptor.alarmRoles;
					this.log("Sensor Alarm List:" + sensorAlarmList);
					if(typeof sensorAlarmList !== 'undefined') {
						this.log("Creating alarm roles for " + this.name);
						this.createSensorAlarmAccessory(deviceDescriptor.command_classes[index], sensorAlarmList);
					}

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
			this.platform.eventSource.addEventListener('EventLogAddedEvent', function(e) {
				//console.log(e.data);
    			var json = JSON.parse(e.data);
				var type = json.type;
				var instance = json.instance_id;
				var command_id = json.command_id;
				var dh = json.data.data_holder;
				if(command_id == this.command_class && json.node_id == this.node_id && dh == this.dh && instance == this.instance)  {
					//console.log("Event accepted");
					var sensorValue = json.data[this.valueHolder || "level"];
					if(typeof this.convert === 'function') {
						sensorValue = this.convert(sensorValue);
					}

					if(typeof this.force_value !== 'undefined') {
						//console.log("Event: Setting force value to " + this.force_value);
						if(typeof this.required_value === 'undefined' || (this.required_value == sensorValue)) {
							if(typeof this.callback === 'function') {
								this.callback(this.force_value);
							} else {
								this.service.getCharacteristic(Characteristics).setValue(this.force_value);
							}
						}
					} else {
						//console.log("Event: Setting Value to " + sensorValue);

						if(typeof this.required_value === 'undefined' || (this.required_value == sensorValue)) {
							if(typeof this.callback === 'function') {
								this.callback(sensorValue);
							} else {
								this.service.getCharacteristic(Characteristics).setValue(sensorValue);
							}
						}
					}
				}
			}.bind(data), false);
		},
		createBinarySensorAccessory : function(commandClass, serviceType) {
			if(serviceType != 'Disabled')
			{
				var service = new Service[serviceType](this.name);
				service.subtype = "instance" + commandClass.instance;
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
		createNotificationAccessory : function(commandClass, notificationList) {
			// Alert notification supported. Lets see what services and characteristics
			// we can create for this device
			for(var  index in notificationList) {
				var notification = notificationList[index];
				this.log("Creating notification for " + notification);
				switch(notification) {
					case 'MotionDetected':
						var existingService = this.serviceList.find(function(element) {
							return typeof element === Service.MotionSensor;
						});

						if(typeof existingService === 'undefined') {
							var service = new Service.MotionSensor(this.name);
							service.getCharacteristic(this.NotificationForService[notification]).on('get', this.getSensorValue.bind(
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

							this.registerEventListener(this.NotificationForService[notification], 
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
						}
					break;
					case 'Water':
						var existingService = this.serviceList.find(function(element) {
							return typeof element === Service.LeakSensor;
						});

						if(typeof existingService === 'undefined') {
							var service = new Service.LeakSensor(this.name);
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
							service.subtype = "instance"+commandClass.instance;
							service.getCharacteristic(this.NotificationForService[sensorAlarm]).on('get', this.getSensorValue.bind(
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

							this.registerEventListener(this.NotificationForService[sensorAlarm], 
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
						// Temperature...
						var service = new Service.TemperatureSensor(this.name);
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
								command_class:this.commandClass,
								dh:1,
								convert:function(value) { return (value - 32) * 5/9; }
							});
						break;
					case "3":
						// Luminiscense
						var service = new Service.LightSensor(this.name);
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
								command_class:this.commandClass,
								dh:3
							});
						break;
					case "5":
						// Humidity
						var service = new Service.HumiditySensor(this.name);
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
								command_class:this.commandClass,
								dh:5
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
					this.accessory.log('HTTP get power function failed: %s', error.message);
					callback(error);
				} else {
					var json = JSON.parse(responseBody);

					var binaryState = json.device_data.level;

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
					this.accessory.log('HTTP get power function failed: %s', error.message);
					callback(error);
				} else {
					var json = JSON.parse(responseBody);

					var sensorValue = (this.valueHolder)? json.device_data[this.valueHolder] : json.device_data.val;
					if(typeof this.convert === 'function') {
						sensorValue = this.convert(sensorValue);
					}

					this.accessory.log("Sensor " + this.accessory.nodeId + " CC " + this.commandClass + " value is %s", sensorValue);
					callback(null, sensorValue);
				}
			}.bind(this));
		}
	}