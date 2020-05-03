#include "python_command.h"
#include "logger.h"

USING_LOGGER(PythonManager)

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

    // TODO: Boolean value set
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

static PyMethodDef CommandMethods[] = {
    {"set_boolean", set_boolean, METH_VARARGS, "Set boolean value"},
    {"set_value", set_value, METH_VARARGS, "Set integer value"},
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