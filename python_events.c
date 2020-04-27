#include "python_events.h"
#include "python_manager.h"
#include "logger.h"
#include <hash.h>

USING_LOGGER(PythonManager)

static variant_stack_t* device_events_module_list;
static hash_table_t* data_events_module_table;

static PyObject* register_device_events(PyObject *self, PyObject *args)
{
    int module_id;
     
    if(!PyArg_ParseTuple(args, "i:register_device_events", &module_id))
    {
        LOG_ERROR(PythonManager, "Argument parse error");
        Py_RETURN_NONE;
    }

    LOG_ADVANCED(PythonManager, "Registering module \"%s\" with DeviceAddedEvent", python_manager_name_from_id(module_id));
    stack_push_back(device_events_module_list, variant_create_int32(DT_INT32, module_id));

    Py_RETURN_NONE;
}

static PyObject* register_data_events(PyObject *self, PyObject *args)
{
    int module_id;
    int device_id;
     
    if(!PyArg_ParseTuple(args, "ii:register_device_events", &module_id, &device_id))
    {
        LOG_ERROR(PythonManager, "Argument parse error");
        Py_RETURN_NONE;
    }

    LOG_ADVANCED(PythonManager, "Registering module \"%s\" with EventLogEvent", python_manager_name_from_id(module_id));
    
    if(data_events_module_table->count == 0)
    {
        variant_stack_t* new_module_list = stack_create();
        variant_hash_insert(data_events_module_table, device_id, variant_create_list(new_module_list));
    }

    variant_t* module_list_variant = variant_hash_get(data_events_module_table, device_id);
    variant_stack_t* module_list = VARIANT_GET_PTR(variant_stack_t, module_list_variant);
    stack_push_back(module_list, variant_create_int32(DT_INT32, module_id));

    Py_RETURN_NONE;
}

static PyMethodDef EventsMethods[] = {
    {"register_device_events", register_device_events, METH_VARARGS, "Register to receive Device Added / Device Removed events"},
    {"register_data_events", register_data_events, METH_VARARGS, "Register to receive EventLogEvent events"},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef EventsModule = {
    PyModuleDef_HEAD_INIT, "events", NULL, -1, EventsMethods,
    NULL, NULL, NULL, NULL
};

PyObject* PyInit_events(void)
{
    return PyModule_Create(&EventsModule);
}

void python_events_init()
{
    device_events_module_list = stack_create();
    data_events_module_table = variant_hash_init();
}

const variant_stack_t* python_events_modules_for_device_event()
{
    return device_events_module_list;
}

const variant_stack_t* python_events_modules_for_data_event(int device_id)
{
    variant_t* module_list = variant_hash_get(data_events_module_table, device_id);
    return VARIANT_GET_PTR(variant_stack_t, module_list);
}