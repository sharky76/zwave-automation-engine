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

    if(record->devtype == ZWAVE)
    {
        command_class_t* command_class = get_command_class_by_id(command_id);
        variant_t* ret = command_class_exec(command_class, "Set", record, variant_create_bool(value));
        variant_free(ret);
    }
    else if(record->devtype == VDEV)
    {
        vdev_t* vdev = vdev_manager_get_vdev(record->deviceName);
        command_class_t* command_class = vdev_manager_get_command_class(record->nodeId);
        variant_t* ret = command_class_exec(command_class, "Set", record, variant_create_bool(value));
        variant_free(ret);
    }
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

        //const char* resolver_name = resolver_name_from_id(node_id, instance_id, command_id);
        if(record->devtype == ZWAVE)
        {
            json_object_object_add(json_resp, "name", json_object_new_string(record->deviceName));
        
            zdata_acquire_lock(ZDataRoot(zway));
            ZDataHolder dh = zway_find_device_instance_cc_data(zway, device_id, instance_id, command_id, path);
            
            if(NULL == dh)
            {
                LOG_ERROR(PythonManager, "get_boolean: NULL data returned");
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
        const char* json_string = json_object_to_json_string(json_resp);
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