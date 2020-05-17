#include "python_events.h"
#include "python_manager.h"
#include "logger.h"
#include "event_dispatcher.h"
#include <hash.h>

USING_LOGGER(PythonManager)

static variant_stack_t* device_events_module_list;
static hash_table_t* data_events_module_table;
static variant_stack_t* timer_events_module_list;

void on_timer_event_callback(event_pump_t*,int,void*);

void free_timer_event_entry(void* arg)
{
    timer_event_entry_t* e = (timer_event_entry_t*)arg;
    free(e->callback);
    free(e);
}

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
     
    if(!PyArg_ParseTuple(args, "ii:register_data_events", &module_id, &device_id))
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

static PyObject* register_timer_events(PyObject *self, PyObject *args)
{
    int module_id;
    int timeout_msec;
    char* callback;
    bool single_shot = false;
     
    if(!PyArg_ParseTuple(args, "iis|b:register_timer_events", &module_id, &timeout_msec, &callback, &single_shot))
    {
        LOG_ERROR(PythonManager, "register_timer_events: Argument parse error");
        Py_RETURN_NONE;
    }

    LOG_ADVANCED(PythonManager, "Registering module \"%s\" with Timer events with timeout %d", python_manager_name_from_id(module_id), timeout_msec);
    
    timer_event_entry_t* event_entry = (timer_event_entry_t*)malloc(sizeof(timer_event_entry_t));
    event_entry->module_id = module_id;
    event_entry->timeout_msec = timeout_msec;
    event_entry->callback = strdup(callback);

    event_pump_t* timer_event_pump = event_dispatcher_get_pump("TIMER_PUMP");
    event_entry->timer_id = event_dispatcher_register_handler(timer_event_pump, event_entry->timeout_msec, single_shot, on_timer_event_callback, (void*)event_entry);
    timer_event_pump->start(timer_event_pump, event_entry->timer_id);
    stack_push_back(timer_events_module_list, variant_create_ptr(DT_PTR, event_entry, &free_timer_event_entry));

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
    {"register_timer_events", register_timer_events, METH_VARARGS, "Register to receive timer events every X msec"},
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
    timer_events_module_list = stack_create();
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

variant_stack_t* python_events_modules_for_timer_event()
{
    return timer_events_module_list;
}

void on_timer_event_callback(event_pump_t* pump, int timer_id, void* arg)
{
    timer_event_entry_t* event_entry = (timer_event_entry_t*)arg;

    module_entry_t* mod_entry = python_manager_get_module(event_entry->module_id);
    PyObject* pFunc = PyObject_GetAttrString(mod_entry->module, event_entry->callback);
    if (pFunc && PyCallable_Check(pFunc)) 
    {
        PyObject* pArgs = PyTuple_New(1);
        PyObject* pContext = mod_entry->context;
        PyTuple_SetItem(pArgs, 0, pContext);

        PyObject* pRetValue = PyObject_CallObject(pFunc, pArgs);
        if(NULL == pRetValue)
        {
            PyErr_Print();
            LOG_ERROR(PythonManager, "Error callong device event handle for module %s", python_manager_name_from_id(event_entry->module_id));
        }
        else
        {
            Py_DECREF(pRetValue);
        }

        Py_DECREF(pFunc);
    }
}
