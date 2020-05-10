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
        LOG_ERROR(PythonManager, "register_device_events: Argument parse error");
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
        LOG_ERROR(PythonManager, "register_data_events: Argument parse error");
        Py_RETURN_NONE;
    }

    LOG_ADVANCED(PythonManager, "Registering module \"%s\" with EventLogEvent for device %d", python_manager_name_from_id(module_id), device_id);
    
    variant_t* module_variant = variant_hash_get(data_events_module_table, device_id);
    if(NULL == module_variant)
    {
        variant_stack_t* module_list = stack_create();
        variant_hash_insert(data_events_module_table, device_id, variant_create_list(module_list));
    }

    variant_t* module_list_variant = variant_hash_get(data_events_module_table, device_id);
    variant_stack_t* module_list = VARIANT_GET_PTR(variant_stack_t, module_list_variant);
    stack_push_back(module_list, variant_create_int32(DT_INT32, module_id));

    Py_RETURN_NONE;
}

static PyObject* register_context(PyObject *self, PyObject *args)
{
    int module_id;
    PyObject* context;
     
    if(!PyArg_ParseTuple(args, "iO:register_context", &module_id, &context))
    {
        LOG_ERROR(PythonManager, "register_context: Argument parse error");
        Py_RETURN_NONE;
    }

    LOG_ADVANCED(PythonManager, "Registering context for module \"%s\"", python_manager_name_from_id(module_id));
    PyObject** context_ptr = python_manager_get_context(module_id);
    Py_INCREF(context);
    *context_ptr = context;

    Py_RETURN_NONE;
}


static PyMethodDef EventsMethods[] = {
    {"register_device_events", register_device_events, METH_VARARGS, "Register to receive Device Added / Device Removed events"},
    {"register_data_events", register_data_events, METH_VARARGS, "Register to receive EventLogEvent events"},
    {"register_context", register_context, METH_VARARGS, "Register context object to pass to event callbacks"},
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