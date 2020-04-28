/** 
 * Python plugin manager 
 * 
 * Defines an API for python scripts to integrate with zae
 * 
 * Events:
 * 
 * Register interest in device_id events:
 * register_device_events()
 * register_data_event(device_id, instance_id, command_id)
 * 
 * On event the following python method will be invoked:
 * on_device_event(device_id, device_type)
 * on_data_event(device_id, instance_id, command_id, device_type)
 * 
 * Device query:
 * 
 * get_name_from_id(device_id, instance, command_class)
 * get_name_from_node_id(device_id)
 * get_device_tree(device_id) -> return device command instances, command classes and methods
 * get_value(device_id, instance, command_class, args...) -> calls command_class.Get(args...)
 * set_value(device_id, instance, command_class, args...) -> calls command_class.Set(args...)
 * send_command(device_id, instance, command_class, command, args...) -> calls command_class.command(args...)
 * 
 * Scene command handler:
 * 
 * on_scene_event(name, source, device_id, instance, command_class, context)
 * 
 * Logging:
 * 
 * log(string)
 */

#include "python_manager.h"
#include <dirent.h>
#include "logger.h"
#include "cli_logger.h"
#include <variant.h>
#include "python_logging.h"
#include "python_events.h"
#include <event_dispatcher.h>
#include <event.h>
#include <hash.h>
#include <event_log.h>
#include <variant.h>

DECLARE_LOGGER(PythonManager)

void    python_manager_on_device_event(event_pump_t* pump, int event_id, void* data, void* context);
void    python_manager_on_data_event(event_pump_t* pump, int event_id, void* data, void* context);

typedef struct module_entry_t
{
    char* name;
    int   module_id;
    PyObject* module;
} module_entry_t;

void module_entry_free(void* arg)
{
    module_entry_t* entry = (module_entry_t*)arg;
    free(entry->name);
    Py_DECREF(entry->module);
    free(entry);
}

static hash_table_t* registered_module_table;

void python_manager_init(const char* python_dir)
{
    PyObject* pInt;

    LOG_ADVANCED(PythonManager, "Initializing python manager...");

    event_pump_t* pump = event_dispatcher_get_pump("EVENT_PUMP");
    event_dispatcher_register_handler(pump, EventLogEvent, python_manager_on_data_event, NULL);
    event_dispatcher_register_handler(pump, DeviceAddedEvent, python_manager_on_device_event, NULL);

    registered_module_table = variant_hash_init();
    python_events_init();

    DIR *dp;
    struct dirent *ep;     
    dp = opendir (python_dir);

    PyImport_AppendInittab("logging", &PyInit_logging);
    PyImport_AppendInittab("events", &PyInit_events);

	Py_Initialize();
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("import os");
    char path_cmd[257] = {0};
    snprintf(path_cmd, 256, "sys.path.append(\"%s\")", python_dir);
    PyRun_SimpleString(path_cmd);

    if(dp != NULL)
    {
        while (ep = readdir (dp))
        {
            // Remove .py extension
            char* ext = strstr(ep->d_name, ".py");
            if(NULL == ext) continue;
            *ext = 0;
            // Import module
            PyObject* pName = PyUnicode_DecodeFSDefault(ep->d_name);
            PyObject* pModule = PyImport_Import(pName);
            Py_DECREF(pName);
            if(NULL == pModule)
            {
                LOG_ERROR(PythonManager, "Failed to import module %s", ep->d_name);
            }
            else
            {
                PyObject* pFunc = PyObject_GetAttrString(pModule, "on_init");
                if (pFunc && PyCallable_Check(pFunc)) 
                {
                    int module_id = variant_get_next_user_type();
                    logger_register_service_with_id(module_id, ep->d_name);
                    cli_add_logging_class(ep->d_name);

                    module_entry_t* new_entry = calloc(1, sizeof(module_entry_t));
                    new_entry->name = strdup(ep->d_name);
                    new_entry->module = pModule;
                    new_entry->module_id = module_id;
                    variant_hash_insert(registered_module_table, module_id, variant_create_ptr(DT_PTR, new_entry, &module_entry_free));

                    PyObject* pArgs = PyTuple_New(1);
                    PyObject* pValue = PyLong_FromLong(module_id);
                    /* pValue reference stolen here: */
                    PyTuple_SetItem(pArgs, 0, pValue);

                    PyObject* pRetValue = PyObject_CallObject(pFunc, pArgs);

                    bool retVal;
                    if(NULL == pRetValue || !PyArg_Parse(pRetValue, "b:on_init", &retVal))
                    {
                        printf("Module %s failed to initialize\n", ep->d_name);
                        PyErr_Print();
                        LOG_ERROR(PythonManager, "Module %s failed to initialize", ep->d_name);
                        variant_hash_remove(registered_module_table, module_id);
                    }
                    else
                    {
                        Py_DECREF(pRetValue);
                    }
                    
                    Py_DECREF(pValue);
                    Py_DECREF(pArgs);
                    Py_DECREF(pFunc);
                }
                else
                {
                    Py_DECREF(pModule);
                }
            }
        }
    }
}

void python_manager_stop()
{
    LOG_ADVANCED(PythonManager, "Stopping python manager...");

    Py_Finalize();
    event_pump_t* pump = event_dispatcher_get_pump("EVENT_PUMP");
    event_dispatcher_unregister_handler(pump, EventLogEvent, python_manager_on_data_event, NULL);
    event_dispatcher_unregister_handler(pump, DeviceAddedEvent, python_manager_on_device_event, NULL);

    variant_hash_free(registered_module_table);
}

const char* python_manager_name_from_id(int module_id)
{
    variant_t* mod_entry_variant = variant_hash_get(registered_module_table, module_id);
    if(NULL == mod_entry_variant)
    {
        return NULL;
    }

    module_entry_t* mod_entry = VARIANT_GET_PTR(module_entry_t, mod_entry_variant);
    return mod_entry->name;
}

void    python_manager_on_device_event(event_pump_t* pump, int event_id, void* data, void* context)
{
    int device_id = (int)data;
    const variant_stack_t* mod_id_list = python_events_modules_for_device_event();
    stack_for_each(mod_id_list, module_id_variant)
    {
        int module_id = variant_get_int(module_id_variant);

        variant_t* mod_entry_variant = variant_hash_get(registered_module_table, module_id);
        if(NULL == mod_entry_variant)
        {
            LOG_ERROR(PythonManager, "Module ID %d not registered", module_id);
            continue;
        }

        module_entry_t* mod_entry = VARIANT_GET_PTR(module_entry_t, mod_entry_variant);
        PyObject* pFunc = PyObject_GetAttrString(mod_entry->module, "on_device_event");
        if (pFunc && PyCallable_Check(pFunc)) 
        {
            PyObject* pArgs = PyTuple_New(1);
            PyObject* pValue = PyLong_FromLong(device_id);
            PyTuple_SetItem(pArgs, 0, pValue);

            PyObject* pRetValue = PyObject_CallObject(pFunc, pArgs);
            if(NULL == pRetValue)
            {
                LOG_ERROR(PythonManager, "Error callong device event handle for module %s", python_manager_name_from_id(module_id));
            }
            else
            {
                Py_DECREF(pRetValue);
            }

            Py_DECREF(pValue);
            //Py_DECREF(pArgs);
            Py_DECREF(pFunc);
        }
    }
}

void    python_manager_on_data_event(event_pump_t* pump, int event_id, void* data, void* context)
{
    event_log_entry_t* event_log_entry = (event_log_entry_t*)data;

    const variant_stack_t* mod_id_list = python_events_modules_for_data_event(event_log_entry->node_id);
    if(NULL == mod_id_list) return;

    stack_for_each(mod_id_list, module_id_variant)
    {
        int module_id = variant_get_int(module_id_variant);
        variant_t* mod_entry_variant = variant_hash_get(registered_module_table, module_id);
        if(NULL == mod_entry_variant)
        {
            LOG_ERROR(PythonManager, "Module ID %d not registered", module_id);
            continue;
        }

        module_entry_t* mod_entry = VARIANT_GET_PTR(module_entry_t, mod_entry_variant);
        PyObject* pFunc = PyObject_GetAttrString(mod_entry->module, "on_data_event");
        if (pFunc && PyCallable_Check(pFunc)) 
        {
            PyObject* pArgs = PyTuple_New(5);
            PyObject* devType = PyLong_FromLong(event_log_entry->device_type);
            PyObject* nodeId = PyLong_FromLong(event_log_entry->node_id);
            PyObject* instanceId = PyLong_FromLong(event_log_entry->instance_id);
            PyObject* commandId = PyLong_FromLong(event_log_entry->command_id);
            PyObject* dataHolder = PyUnicode_FromString(event_log_entry->event_data);

            if(PyTuple_SetItem(pArgs, 0, devType) == -1 ||
               PyTuple_SetItem(pArgs, 1, nodeId) == -1 ||
               PyTuple_SetItem(pArgs, 2, instanceId) == -1 ||
               PyTuple_SetItem(pArgs, 3, commandId) == -1 ||
               PyTuple_SetItem(pArgs, 4, dataHolder) == -1)
            {
                LOG_ERROR(PythonManager, "Error setting script arguments");
            }
            else
            {
                PyObject* pRetValue = PyObject_CallObject(pFunc, pArgs);
                if(NULL == pRetValue)
                {
                    LOG_ERROR(PythonManager, "Error callong device event handle for module %s", python_manager_name_from_id(module_id));
                }
                else
                {
                    Py_DECREF(pRetValue);
                }
            }

            Py_DECREF(devType);
            Py_DECREF(nodeId);
            Py_DECREF(instanceId);
            Py_DECREF(commandId);
            Py_DECREF(dataHolder);
            Py_DECREF(pFunc);
        }
    }
}