#include "python_command.h"
#include "logger.h"
#include <resolver.h>
#include <ZWayLib.h>
#include <ZData.h>
#include <ZPlatform.h>
#include <json-c/json.h>
#include <zway_json.h>
#include "command_class.h"
#include "vdev_manager.h"

USING_LOGGER(PythonManager)
extern ZWay zway;

static PyObject* set_boolean(PyObject *self, PyObject *args)
{
    int module_id;
    int device_id;
    int instance_id;
    int command_id;
    bool value;
     
    if(!PyArg_ParseTuple(args, "iiiib:set_boolean", &module_id, &device_id, &instance_id, &command_id, &value))
    {
        LOG_ERROR(PythonManager, "set_boolean: Argument parse error");
        Py_RETURN_NONE;
    }

    device_record_t* record = resolver_resolve_id(device_id, instance_id, command_id);
    if(NULL == record)
    {
        Py_RETURN_NONE;
    }

    command_class_t* command_class = NULL;

    if(record->devtype == ZWAVE)
    {
        command_class = get_command_class_by_id(command_id);
    }
    else if(record->devtype == VDEV)
    {
        vdev_t* vdev = vdev_manager_get_vdev(record->deviceName);
        command_class = vdev_manager_get_command_class(record->nodeId);
    }

    variant_t* ret = command_class_exec(command_class, "Set", record, variant_create_bool(value));
    variant_free(ret);

    Py_RETURN_NONE;
}

static PyObject* set_value(PyObject *self, PyObject *args)
{
    int module_id;
    int device_id;
    int instance_id;
    int command_id;
    int value;
     
    if(!PyArg_ParseTuple(args, "iiiii:set_value", &module_id, &device_id, &instance_id, &command_id, &value))
    {
        LOG_ERROR(PythonManager, "set_value: Argument parse error");
        Py_RETURN_NONE;
    }

    // TODO: Integer Value set
    Py_RETURN_NONE;
}

static PyObject* get_value(PyObject *self, PyObject *args)
{
    int module_id;
    int device_id;
    int instance_id;
    int command_id;
    char* path = NULL;
     
    if(!PyArg_ParseTuple(args, "iiii|s:get_value", &module_id, &device_id, &instance_id, &command_id, &path))
    {
        LOG_ERROR(PythonManager, "get_value: Argument parse error");
        Py_RETURN_NONE;
    }

    // TODO: Boolean value get
    device_record_t* record = resolver_resolve_id(device_id, instance_id, command_id);

    if(NULL != record)
    {   
        json_object* json_resp = json_object_new_object();
        json_object_object_add(json_resp, "name", json_object_new_string(record->deviceName));

        //const char* resolver_name = resolver_name_from_id(node_id, instance_id, command_id);
        if(record->devtype == ZWAVE)
        {
            zdata_acquire_lock(ZDataRoot(zway));
            ZDataHolder dh = zway_find_device_instance_cc_data(zway, device_id, instance_id, command_id, path);
            
            if(NULL == dh)
            {
                LOG_ERROR(PythonManager, "get_value: NULL data returned");
                Py_RETURN_NONE;
            }
            else
            {
                json_object* device_data = json_object_new_object();
                zway_json_data_holder_to_json(device_data, dh);
                json_object_object_add(json_resp, "device_data", device_data);
            }
            zdata_release_lock(ZDataRoot(zway));
        }
        else
        {
            // Either get a specific device + command class
            vdev_t* vdev = vdev_manager_get_vdev(record->deviceName);

            if(NULL == vdev)
            {
                // Lets try to get a root device...
                const char* device_name = resolver_name_from_id(device_id, -1, -1);
                if(NULL != device_name)
                {
                    vdev = vdev_manager_get_vdev(device_name);
                }
            }

            if(NULL != vdev)
            {
                stack_for_each(vdev->supported_method_list, vdev_command_variant)
                {
                    vdev_command_t* vdev_command = VARIANT_GET_PTR(vdev_command_t, vdev_command_variant);
                    if(vdev_command->is_command_class)
                    {
                        command_class_t* vdev_cmd_class = vdev_manager_get_command_class(device_id);
                        
                        if(NULL != vdev_cmd_class)
                        {
                            if(vdev_command->nargs != 0 || !vdev_command->is_get)
                            {
                                continue;
                            }
                            LOG_DEBUG(PythonManager, "Calling command %s on device %s, node_id: %d, instance_id: %d, command_id: %d",
                                            vdev->name, vdev_command->name, record->nodeId, record->instanceId, record->commandId);
                            variant_t* value = command_class_exec(vdev_cmd_class, vdev_command->name, record);
                            if(NULL == value)
                            {
                                LOG_ERROR(PythonManager, "Unable to call method %s on VDEV: device not found", vdev_command->name);
                                continue;
                            }

                            json_object* cmd_value = json_object_new_object();
                            char* string_value;
                            variant_to_string(value, &string_value);
            
                            if(NULL == vdev_command->data_holder)
                            {
                                // The string value is json string to be appended directly under "dh" key
                                json_object_put(cmd_value);
                                cmd_value = json_tokener_parse(string_value);
                            }
                            else if(strchr(vdev_command->data_holder, '.') != NULL)
                            {
                                json_object* parameter_array = json_object_new_array();
                                char* data_holder = strdup(vdev_command->data_holder);
                                char* saveptr;
                                char* tok = strtok_r(data_holder, ".", &saveptr);
                                json_object* dh_item = json_object_new_object();
            
                                if(NULL != tok)
                                {
                                    json_object_object_add(dh_item, "data_holder", json_object_new_string(tok));
                                }
            
                                tok = strtok_r(NULL, ".", &saveptr);
            
                                if(NULL != tok)
                                {
                                    json_object_object_add(dh_item, tok, variant_to_json_object(value));
                                }
                                
                                json_object_array_add(parameter_array, dh_item);
                                json_object_object_add(cmd_value, "parameters", parameter_array);
                                free(data_holder);
                            }
                            else
                            {
                                json_object_object_add(cmd_value, vdev_command->data_holder, variant_to_json_object(value));
                            }
                            
                            free(string_value);
                            variant_free(value);
        
                            json_object_object_add(json_resp, "device_data", cmd_value);
                            break;
                        }
                    }
                }
            }
        }
        const char* json_string = json_object_to_json_string(json_resp);
        LOG_DEBUG(PythonManager, "JSON response: %s", json_string);
        PyObject* ret = PyUnicode_FromString(json_string);
        json_object_put(json_resp);;

        return ret;
    }
    
    Py_RETURN_NONE;
}

static PyMethodDef CommandMethods[] = {
    {"set_boolean", set_boolean, METH_VARARGS, "Set boolean value"},
    {"set_value", set_value, METH_VARARGS, "Set integer value"},
    {"get_value", get_value, METH_VARARGS, "Get device value"},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef CommandModule = {
    PyModuleDef_HEAD_INIT, "command", NULL, -1, CommandMethods,
    NULL, NULL, NULL, NULL
};

PyObject* PyInit_command(void)
{
    return PyModule_Create(&CommandModule);
}