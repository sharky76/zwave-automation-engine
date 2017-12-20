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
		//homebridge.registerAccessory("homebridge-http", "ZAE", ZAEAccessory);

		//homebridge.registerAccessory("homebridge-http", "Http", HttpAccessory);

		console.log("ZAE registered with homebridge");

		// Load config path to read event URL
		/*var config = JSON.parse(fs.readFileSync(homebridge.user.configPath()));
		zae_host = config.zae_host;
		zae_port = config.zae_port;
		var eventUrl = 'http://' + zae_host + ':' + zae_port + '/rest/v1/sse';

		console.log("EventURL:", eventUrl);
		eventSource = new EventSource(eventUrl);
		eventSource.onopen = function () {
		  console.log("Connection opened")
		};

		eventSource.onerror = function () {
		  console.log("Connection error");
		};
*/
		
	}

	function ZAEPlatform(log, config, api) {
		var platform = this;
		this.log = log;
		this.config = config;
		this.api = api;
		// Load config path to read event URL
		//var config = JSON.parse(fs.readFileSync(homebridge.user.configPath()));
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

		var CharacteristicForService = {
			ContactSensor: Characteristic.ContactSensorState,
			MotionSensor:Characteristic.MotionDetected
		};

		for(var index in deviceDescriptor.command_classes) {
			switch(deviceDescriptor.command_classes[index].id) {
				case 48: // Binary Sensor
					var serviceType = deviceDescriptor.role || "ContactSensor";
					var service = new Service[serviceType](this.name);

					service.getCharacteristic(CharacteristicForService[serviceType]).on('get', this.getBinaryState.bind(
							{
								dh: deviceDescriptor.command_classes[index].dh.parameters[0].data_holder,
								accessory:this,
								commandClass:deviceDescriptor.command_classes[index].id
							}));
					this.serviceList.push(service);

					this.registerEventListener(CharacteristicForService[serviceType], 
						{
							service:service,
							node_id:this.nodeId,
							command_class:deviceDescriptor.command_classes[index].id,
							dh:1
						});

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
								commandClass:deviceDescriptor.command_classes[index].id
							}))
						.on('set', this.setBinaryState.bind(
							{
								accessory:this,
								commandClass:deviceDescriptor.command_classes[index].id
							}));
					this.serviceList.push(service);

					this.registerEventListener(Characteristic.On, 
						{
							service:service,
							node_id:this.nodeId,
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
								command_class:deviceDescriptor.command_classes[index].id,
								dh:"currentScene",
								valueHolder:"currentScene",	// Read event value from this data holder
								force_value:0,				// Force Set this value to device (SINGLE_PRESS)
								required_value:sceneIndex	// Only react to the event if value equal to required_value
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
								commandClass:deviceDescriptor.command_classes[index].id,
								convert:function(value) { return (value < 20)? Characteristic.StatusLowBattery.BATTERY_LEVEL_LOW : Characteristic.StatusLowBattery.BATTERY_LEVEL_NORMAL }
							}));

					service.getCharacteristic(Characteristic.BatteryLevel).on('get', this.getSensorValue.bind(
							{
								dh: "last",
								valueHolder:"last",
								accessory:this,
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
			}
		}

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
			this.log("Add event listener for device", this.name);
			this.platform.eventSource.addEventListener('EventLogAddedEvent', function(e) {
				//console.log(e.data);
    			var json = JSON.parse(e.data);
				var type = json.type;
				var command_id = json.command_id;
				var dh = json.data.data_holder;
				if(command_id == this.command_class && json.node_id == this.node_id && dh == this.dh) {

					var sensorValue = json.data[this.valueHolder || "level"];
					if(typeof this.convert === 'function') {
						sensorValue = this.convert(sensorValue);
					}

					console.log(this.force_value);
					if(typeof this.force_value !== 'undefined') {
						//console.log("Event: Setting force value to " + this.force_value);
						if(typeof this.required_value === 'undefined' || (this.required_value == sensorValue)) {
							this.service.getCharacteristic(Characteristics).setValue(this.force_value);
						}
					} else {
						//console.log("Event: Setting Value to " + sensorValue);

						if(typeof this.required_value === 'undefined' || (this.required_value == sensorValue)) {
							this.service.getCharacteristic(Characteristics).setValue(sensorValue);
						}
					}
				}
			}.bind(data), false);
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
									commandClass:commandClass.id,
									convert:function(value) { return (value - 32) * 5/9; }
								}));
						this.serviceList.push(service);

						this.registerEventListener(Characteristic.CurrentTemperature, 
							{
								service:service,
								node_id:this.nodeId,
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
								commandClass:commandClass.id
							}));
						this.serviceList.push(service);
						this.registerEventListener(Characteristic.CurrentAmbientLightLevel, 
							{
								service:service,
								node_id:this.nodeId,
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
								commandClass:commandClass.id,
								accessory:this
							}));
						this.serviceList.push(service);
						this.registerEventListener(Characteristic.CurrentRelativeHumidity, 
							{
								service:service,
								node_id:this.nodeId,
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
				+ this.accessory.nodeId + '/0/' + this.commandClass;

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
				+ this.accessory.nodeId + '/0/' + this.commandClass;
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
			var url = 'http://' + this.accessory.config.zae_host + ':' + this.accessory.config.zae_port + '/rest/v1/devices/'
				+ this.accessory.nodeId + '/0/' + this.commandClass + '/' + this.dh;
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


	function HttpAccessory(log, config) {
		this.log = log;

		// url info
		this.on_url                 = config["on_url"];
		this.on_body                = config["on_body"];
		this.off_url                = config["off_url"];
		this.off_body               = config["off_body"];
		this.status_url             = config["status_url"];
		this.brightness_url         = config["brightness_url"];
		this.brightnesslvl_url      = config["brightnesslvl_url"];
		this.http_method            = config["http_method"] 	  	 	|| "GET";;
		this.http_set_method 	    = config["http_set_method"]  		|| this.http_method;
		this.username               = config["username"] 	  	 	 	|| "";
		this.password               = config["password"] 	  	 	 	|| "";
		this.sendimmediately        = config["sendimmediately"] 	 	|| "";
		this.service                = config["service"] 	  	 	 	|| "Switch";
		this.name                   = config["name"];
		this.brightnessHandling     = config["brightnessHandling"] 	 	|| "no";
		this.switchHandling 		= config["switchHandling"] 		 	|| "no";
		this.pollingFrequency       = config["pollingFrequency"]		|| 1000;
		this.status_on				= config["status_on"]				|| true;
		this.status_off				= config["status_off"]				|| false;
		this.node_id				= config["node_id"];

		//realtime polling info
		this.state = false;
		this.currentlevel = 0;
		var that = this;
		
		this.log("Add event listener for device", this.name);
		eventSource.addEventListener('EventLogAddedEvent', function(e) {
			console.log(that.name + " -> " + e.data);
  	      	var json = JSON.parse(e.data);
			var type = json.type;

			if(type == "ZWAVE") {
				var node_id = json.node_id;
				if(node_id == that.node_id) {
			  		switch (that.service) {
					case "SensorBinary":
						if(that.sensorBinaryService)
						{	var command_id = json.command_id;
							if(command_id == 48) {
								that.state = json.data.level;
								that.sensorBinaryService.getCharacteristic(Characteristic.ContactSensorState)
									.setValue(that.state);
							}
						}
						break;
					case "MotionSensor":
						if(that.motionSensorService)
						{
							var command_id = json.command_id;
							if(command_id == 48) {
								that.state = json.data.level;
								that.motionSensorService.getCharacteristic(Characteristic.MotionDetected)
									.setValue(that.state);
							}
						}
						break;
					case "Switch":
						if(that.switchService) {
							var command_id = json.command_id;
							if(command_id == 37) {
								that.state = json.data.level;
								that.switchService.getCharacteristic(Characteristic.On)
									.setValue(that.state);
							}
						}
					case "TemperatureSensor":
						if(that.temperatureService) {
							var command_id = json.command_id;
							var dh_name = json.data.data_holder;

							if(command_id == 49 && dh_name == 1) {
								that.state = (json.data.val - 32) * 5/9;
								that.temperatureService.getCharacteristic(Characteristic.CurrentTemperature)
									.setValue(that.state);
							}
						}
						break;
					case "HumiditySensor":
						if(that.humidityService) {
							var command_id = json.command_id;
							var dh_name = json.data.data_holder;

							if(command_id == 49 && dh_name == 5) {
								that.state = json.data.val;
								that.humidityService.getCharacteristic(Characteristic.CurrentRelativeHumidity)
									.setValue(that.state);
							}
						}
						break;
					case "LightSensor":
						if(that.lightService) {
							var command_id = json.command_id;
							var dh_name = json.data.data_holder;

							if(command_id == 49 && dh_name == 3) {
								that.state = json.data.val;
								that.lightService.getCharacteristic(Characteristic.CurrentAmbientLightLevel)
									.setValue(that.state);
							}
						}
						break;
					case "StatelessProgrammableSwitch":
						if(that.sceneControllerService) {
							var command_id = json.command_id;

							if(command_id == 91) {
								that.state = 0;
								that.sceneControllerService.getCharacteristic(Characteristic.ProgrammableSwitchEvent)
									.setValue(that.state);
							}
						}
						break;
					}
				}        
			}
		}, false);

		// Status Polling, if you want to add additional services that don't use switch handling you can add something like this || (this.service=="Smoke" || this.service=="Motion"))
		/*if (this.status_url) {
			var powerurl = this.status_url;
			var statusemitter = pollingtoevent(function(done) {
	        	that.httpRequest(powerurl, "", "GET", that.username, that.password, that.sendimmediately, function(error, response, body) {
            		if (error) {
                		that.log('HTTP get power function failed: %s', error.message);
		                callback(error);
            		} else {               				    
						done(null, body);
            		}
        		})
			}, {longpolling:false,interval:this.pollingFrequency,longpollEventName:"statuspoll"});

		statusemitter.on("statuspoll", function(data) {       
        	var binaryState = parseInt(data);
        	that.log(that.service, "Data is", data);
	    	that.state = binaryState > 0 || data == this.status_on;
			that.log(that.service, "received status",that.status_url, "state is currently", this.state); 
			// switch used to easily add additonal services
			switch (that.service) {
				case "Switch":
					if (that.switchService ) {
						that.switchService .getCharacteristic(Characteristic.On)
						.setValue(that.state);
					}
					break;
				case "Light":
					if (that.lightbulbService) {
						that.lightbulbService.getCharacteristic(Characteristic.On)
						.setValue(that.state);
					}		
					break;	
				case "SensorBinary":
					if(that.sensorBinaryService)
					{
						var json = JSON.parse(data);
						that.state = json.device_data[1].level;
						that.sensorBinaryService.getCharacteristic(Characteristic.ContactSensorState)
						.setValue(that.state);
					}
				case "MotionSensor":
					if(that.motionSensorService)
					{
						var json = JSON.parse(data);
						that.state = json.device_data[1].level;
						that.motionSensorService.getCharacteristic(Characteristic.MotionDetected)
						.setValue(that.state);
					}
					break;

				}        
		});

	} */
	// Brightness Polling
	/*if (this.brightnesslvl_url && this.brightnessHandling =="realtime") {
		var brightnessurl = this.brightnesslvl_url;
		var levelemitter = pollingtoevent(function(done) {
	        	that.httpRequest(brightnessurl, "", "GET", that.username, that.password, that.sendimmediately, function(error, response, responseBody) {
            		if (error) {
                			that.log('HTTP get power function failed: %s', error.message);
							return;
            		} else {               				    
						done(null, responseBody);
            		}
        		}) // set longer polling as slider takes longer to set value
    	}, {longpolling:true,interval:2000,longpollEventName:"levelpoll"});

		levelemitter.on("levelpoll", function(data) {  
			that.currentlevel = parseInt(data);

			if (that.lightbulbService) {				
				that.log(that.service, "received brightness",that.brightnesslvl_url, "level is currently", that.currentlevel); 		        
				that.lightbulbService.getCharacteristic(Characteristic.Brightness)
				.setValue(that.currentlevel);
			}        
    	});
	}*/
	}

	HttpAccessory.prototype = {

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

	setBinaryState: function(isTrue, callback) {
		var url;
		var body;
		
		if (!this.on_url || !this.off_url) {
				this.log.warn("Ignoring request; No power url defined.");
				callback(new Error("No power url defined."));
				return;
		}
		
		if (isTrue) {
			url = this.on_url;
			body = this.on_body;
			this.log("Setting power state to on");
		} else {
			url = this.off_url;
			body = this.off_body;
			this.log("Setting power state to off");
		}
		
		this.httpRequest(url, body, this.http_set_method, this.username, this.password, this.sendimmediately, function(error, response, responseBody) {
			if (error) {
			this.log('HTTP set power function failed: %s', error.message);
			callback(error);
			} else {
			this.log('HTTP set power function succeeded!');
			callback();
			}
		}.bind(this));
	},
  
  getBinaryState: function(callback) {
	if (!this.status_url) {
		this.log.warn("Ignoring request; No status url defined.");
		callback(new Error("No status url defined."));
		return;
	}
	
	var url = this.status_url;
	this.log("Getting Binary state");
	
	this.httpRequest(url, "", this.http_method, this.username, this.password, this.sendimmediately, function(error, response, responseBody) {
	if (error) {
		this.log('HTTP get power function failed: %s', error.message);
		callback(error);
	} else {
		var json = JSON.parse(responseBody);

		var binaryState = json.device_data.level;

		this.log("Binary state is currently %s", binaryState);
		callback(null, binaryState);
	}
	}.bind(this));
  },
  getSensorValue: function(callback) {
	if (!this.status_url) {
		this.log.warn("Ignoring request; No status url defined.");
		callback(new Error("No status url defined."));
		return;
	}
	
	var url = this.status_url;
	this.log("Getting Sensor value");
	
	this.httpRequest(url, "", this.http_method, this.username, this.password, this.sendimmediately, function(error, response, responseBody) {
	if (error) {
		this.log('HTTP get power function failed: %s', error.message);
		callback(error);
	} else {
		var json = JSON.parse(responseBody);

		var sensorValue = json.device_data.val;
		if(typeof this.convert === 'function') {
			sensorValue = this.convert(sensorValue);
		}

		this.log("Sensor value is %s", sensorValue);
		callback(null, sensorValue);
	}
	}.bind(this));
  },
  getLowBattery: function(callback) {
	var url = 'http://' + zae_host + ':' + zae_port + '/rest/v1/devices/' + this.node_id + '/0/128';
	this.log("Getting Battery value");
	
	this.httpRequest(url, "", this.http_method, this.username, this.password, this.sendimmediately, function(error, response, responseBody) {
	if (error) {
		this.log('HTTP get power function failed: %s', error.message);
		callback(error);
	} else {
		var json = JSON.parse(responseBody);

		var batteryValue = json.device_data.last;
		this.log("Battery value is %s", batteryValue);
		callback(null, batteryValue < 20);
	}
	}.bind(this));
  },
  getSceneControllerValue: function(callback) {
  	var url = 'http://' + zae_host + ':' + zae_port + '/rest/v1/devices/' + this.node_id + '/0/91';
	this.log("Getting Scene Controller value");
	
	this.httpRequest(url, "", this.http_method, this.username, this.password, this.sendimmediately, function(error, response, responseBody) {
	if (error) {
		this.log('HTTP get power function failed: %s', error.message);
		callback(error);
	} else {
		var json = JSON.parse(responseBody);

		var currentScene = json.device_data.currentScene;
		this.log("Current scene is %s", currentScene);
		callback(null, currentScene);
	}
	}.bind(this));
  },
  	getBrightness: function(callback) {
		if (!this.brightnesslvl_url) {
			this.log.warn("Ignoring request; No brightness level url defined.");
			callback(new Error("No brightness level url defined."));
			return;
		}		
			var url = this.brightnesslvl_url;
			this.log("Getting Brightness level");
	
			this.httpRequest(url, "", "GET", this.username, this.password, this.sendimmediately, function(error, response, responseBody) {
			if (error) {
				this.log('HTTP get brightness function failed: %s', error.message);
				callback(error);
			} else {			
				var binaryState = parseInt(responseBody);
				var level = binaryState;
				this.log("brightness state is currently %s", binaryState);
				callback(null, level);
			}
			}.bind(this));
	  },

	setBrightness: function(level, callback) {
		
		if (!this.brightness_url) {
			this.log.warn("Ignoring request; No brightness url defined.");
			callback(new Error("No brightness url defined."));
			return;
		}    
	
		var url = this.brightness_url.replace("%b", level)
	
		this.log("Setting brightness to %s", level);
	
		this.httpRequest(url, "", this.http_brightness_method, this.username, this.password, this.sendimmediately, function(error, response, body) {
		if (error) {
			this.log('HTTP brightness function failed: %s', error);
			callback(error);
		} else {
			this.log('HTTP brightness function succeeded!');
			callback();
		}
		}.bind(this));
	},

	identify: function(callback) {
		this.log("Identify requested!");
		callback(); // success
	},

	getServices: function() {
		
		var that = this;
		
		// you can OPTIONALLY create an information service if you wish to override
		// the default values for things like serial number, model, etc.
		var informationService = new Service.AccessoryInformation();
	
		informationService
		.setCharacteristic(Characteristic.Manufacturer, "Alex")
		.setCharacteristic(Characteristic.Model, "ZWAVE Automation Engine")
		.setCharacteristic(Characteristic.SerialNumber, "0");
	
		switch (this.service) {
			case "Switch": 
				this.switchService = new Service.Switch(this.name);
				this.switchService
					.getCharacteristic(Characteristic.On)
						.on('get', this.getBinaryState.bind(this))
						.on('set', this.setBinaryState.bind(this));						
				// case "realtime":				
				// 	this.switchService
				// 	.getCharacteristic(Characteristic.On)
				// 	.on('get', function(callback) {callback(null, that.state)})
				// 	.on('set', this.setPowerState.bind(this));
				// 	break;
				// default	:	
				// 	this.switchService
				// 	.getCharacteristic(Characteristic.On)	
				// 	.on('set', this.setPowerState.bind(this));					
				// 	break;}
					return [this.switchService];
					break;
		case "Light":	
			this.lightbulbService = new Service.Lightbulb(this.name);			
			switch (this.switchHandling) {
			//Power Polling
			case "yes" :
				this.lightbulbService
				.getCharacteristic(Characteristic.On)
				.on('get', this.getBinaryState.bind(this))
				.on('set', this.setBinaryState.bind(this));
				break;
			case "realtime":
				this.lightbulbService
				.getCharacteristic(Characteristic.On)
				.on('get', function(callback) {callback(null, that.state)})
				.on('set', this.setBinaryState.bind(this));
				break;
			default:		
				this.lightbulbService
				.getCharacteristic(Characteristic.On)	
				.on('set', this.setBinaryState.bind(this));
				break;
			}
			// Brightness Polling 
			if (this.brightnessHandling == "realtime") {
				this.lightbulbService 
				.addCharacteristic(new Characteristic.Brightness())
				.on('get', function(callback) {callback(null, that.currentlevel)})
				.on('set', this.setBrightness.bind(this));
			} else if (this.brightnessHandling == "yes") {
				this.lightbulbService
				.addCharacteristic(new Characteristic.Brightness())
				.on('get', this.getBrightness.bind(this))
				.on('set', this.setBrightness.bind(this));							
			}
	
			return [informationService, this.lightbulbService];
			break;
			case "SensorBinary":
			{
				this.sensorBinaryService = new Service.ContactSensor(this.name);
				this.sensorBinaryService
					.getCharacteristic(Characteristic.ContactSensorState)
						.on('get', this.getBinaryState.bind(this));

				this.sensorBinaryService
					.getCharacteristic(Characteristic.StatusLowBattery)
						.on('get', this.getLowBattery.bind(this));
			}
			return [informationService, this.sensorBinaryService];
			break;		
			case "MotionSensor":
			{
				this.motionSensorService = new Service.MotionSensor(this.name);
				this.motionSensorService
					.getCharacteristic(Characteristic.MotionDetected)
						.on('get', this.getBinaryState.bind(this));

				this.motionSensorService
					.getCharacteristic(Characteristic.StatusLowBattery)
						.on('get', this.getLowBattery.bind(this));
			}
			return [informationService, this.motionSensorService];
			break;
			case "TemperatureSensor":
			{
				this.temperatureService = new Service.TemperatureSensor(this.name);
				this.convert = function(value) { return (value - 32) * 5/9; }
				this.temperatureService
					.getCharacteristic(Characteristic.CurrentTemperature)
						.on('get', this.getSensorValue.bind(this));

				this.temperatureService
					.getCharacteristic(Characteristic.StatusLowBattery)
						.on('get', this.getLowBattery.bind(this));
			}
			return [informationService, this.temperatureService];
			break;
			case "HumiditySensor":
			{
				this.humidityService = new Service.HumiditySensor(this.name);
				this.humidityService
					.getCharacteristic(Characteristic.CurrentRelativeHumidity)
						.on('get', this.getSensorValue.bind(this));

				this.humidityService
					.getCharacteristic(Characteristic.StatusLowBattery)
						.on('get', this.getLowBattery.bind(this));
			}
			return [informationService, this.humidityService];
			break;
			case "LightSensor":
			{
				this.lightService = new Service.LightSensor(this.name);
				this.lightService
					.getCharacteristic(Characteristic.CurrentAmbientLightLevel)
						.on('get', this.getSensorValue.bind(this));

				this.lightService
					.getCharacteristic(Characteristic.StatusLowBattery)
						.on('get', this.getLowBattery.bind(this));
			}
			return [informationService, this.lightService];
			break;
			case "StatelessProgrammableSwitch":
			{
				this.sceneControllerService = new Service.StatelessProgrammableSwitch(this.name);

				this.sceneControllerService
					.getCharacteristic(Characteristic.ProgrammableSwitchEvent)
						.on('get', function(callback) {callback(null, that.state)});

				this.sceneControllerService
					.getCharacteristic(Characteristic.ServiceLabelIndex)
						.on('get', this.getSceneControllerValue.bind(this));

				this.sceneControllerService
					.getCharacteristic(Characteristic.StatusLowBattery)
						.on('get', this.getLowBattery.bind(this));
			}
			return [informationService, this.sceneControllerService];
			break;
		}
	}
};