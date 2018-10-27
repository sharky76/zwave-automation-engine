var Accessory, hap, UUIDGen;
var request = require("request");

var FFMPEG = require('./ffmpeg').FFMPEG;

module.exports = function(homebridge) {
  Accessory = homebridge.platformAccessory;
  hap = homebridge.hap;
  UUIDGen = homebridge.hap.uuid;

  homebridge.registerPlatform("homebridge-zae-camera", "ZAE-camera", ffmpegPlatform, false);

  console.log("ZAE-camera registered with homebridge");
}

function ffmpegPlatform(log, config, api) {
  var self = this;

  self.log = log;
  self.config = config || {};

  if (api) {
    log("API is valid");
    this.api = api;
    self.api = api;

    if (api.version < 2.1) {
      throw new Error("Unexpected API version.");
    }

    self.api.on('didFinishLaunching', self.didFinishLaunching.bind(this));
  }
}

ffmpegPlatform.prototype.configureAccessory = function(accessory) {
  // Won't be invoked
}

ffmpegPlatform.prototype = {
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
    callback([]);
  },
	getAccessories: function(callback) {
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

    //this.log("Resp: " + response);

    var json = JSON.parse(response);
    var videoProcessor = this.config.videoProcessor || 'ffmpeg';

		for(var index in json.devices) {
      //this.log("Index: " + index + " " + json.devices[index].node_id);
      for(var cmdClassIndex in json.devices[index].command_classes)
      {
        this.log("Command class index: " + cmdClassIndex + " ID " + json.devices[index].command_classes[cmdClassIndex].id);
        if(json.devices[index].command_classes[cmdClassIndex].id == 242)
        {
          var cameraConfig = [];

          var cameraName = json.devices[index].name;
          var videoConfig = json.devices[index].command_classes[cmdClassIndex].dh.cameras[0].videoConfig;
          
          videoConfig["vcodec"] = "h264_omx";
          videoConfig["debug"] = true;
          videoConfig["audio"] = false;

          this.log("[1]Video config: " + videoConfig.source);
          // -rtsp_transport tcp
          videoConfig.source = "-rtsp_transport tcp -c:v h264_mmal " + videoConfig.source;

          cameraConfig["name"] = cameraName;
          cameraConfig["videoConfig"] = videoConfig;

          this.log("[2]Video config: " + cameraConfig.videoConfig.source);

          this.log("Create Camera " + json.devices[index].node_id);
          // Attempt to create accessory for each device entry...
          var uuid = UUIDGen.generate(cameraName);
          var cameraAccessory = new Accessory(cameraName, uuid, hap.Accessory.Categories.CAMERA);
          var cameraSource = new FFMPEG(hap, cameraConfig, this.log, videoProcessor);
          cameraAccessory.configureCameraSource(cameraSource);
          accessoriesList.push(cameraAccessory);
        }
      }
		}

		return accessoriesList;
  },
  didFinishLaunching : function() {
      this.getAccessories(function(configuredAccessories) {
        this.api.publishCameraAccessories("ZAE-camera", configuredAccessories);
      }.bind(this));
    }
}
