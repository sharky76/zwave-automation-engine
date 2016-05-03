#include "config.h"
#include <json-c/json.h>
#include <string.h>
#include <logger.h>
#include <wordexp.h>

/*
 * Config stored in json format: 
 *  
  { 
    "device": "/dev/ttyAMA0",
    "api_prefix": "/home/osmc/z-way-server",
    "services_prefix": "services",
    "config_location": "~/.zae/",
    "client_port":    9231
  }
 *  
 * 
 * @author alex (2/24/2016)
 * 
 * @param file 
 */

DECLARE_LOGGER(Config)
config_t    global_config;

bool    config_load(const char* file)
{
    struct json_object* config_object = json_object_from_file(file);

    if(NULL == config_object)
    {
        LOG_ERROR(Config, "Error loading global config file %s", file);
        return false;
    }

    struct json_object* device_object;
    json_object_object_get_ex(config_object, "device", &device_object);
    if(NULL == device_object)
    {
        return false;
    }
    global_config.device = strdup(json_object_get_string(device_object));

    struct json_object* api_object;
    json_object_object_get_ex(config_object, "api_prefix", &api_object);
    if(NULL == api_object)
    {
        return false;
    }
    global_config.api_prefix = strdup(json_object_get_string(api_object));

    struct json_object* services_object;
    json_object_object_get_ex(config_object, "services", &services_object);
    if(NULL == services_object)
    {
        return false;
    }
    global_config.services_prefix = strdup(json_object_get_string(services_object));

    struct json_object* vdev_object;
    json_object_object_get_ex(config_object, "virtual_devices", &vdev_object);
    if(NULL == vdev_object)
    {
        return false;
    }
    global_config.vdev_prefix = strdup(json_object_get_string(vdev_object));

    struct json_object* config_loc_object;
    json_object_object_get_ex(config_object, "config_location", &config_loc_object);
    if(NULL == config_loc_object)
    {
        return false;
    }
    wordexp_t exp_result;
    wordexp(json_object_get_string(config_loc_object), &exp_result, 0);
    global_config.config_location = strdup(exp_result.we_wordv[0]);

    struct json_object* port_object;
    json_object_object_get_ex(config_object, "client_port", &port_object);
    if(NULL == port_object)
    {
        return false;
    }
    global_config.client_port = json_object_get_int(port_object);

    json_object_put(config_object);
    return true;
}
