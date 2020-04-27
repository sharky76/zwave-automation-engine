#include "python_logging.h"
#include "logger.h"

USING_LOGGER(PythonManager)

static PyObject* log_info(PyObject *self, PyObject *args)
{
    int module_id;
    char* message;
     
    if(!PyArg_ParseTuple(args, "is:log_error", &module_id, &message))
    {
        LOG_ERROR(PythonManager, "Argument parse error");
        Py_RETURN_NONE;
    }

    LOG_INFO(module_id, message);    
    Py_RETURN_NONE;

}

static PyObject* log_error(PyObject *self, PyObject *args)
{
    int module_id;
    char* message;
     
    if(!PyArg_ParseTuple(args, "is:log_error", &module_id, &message))
    {
        LOG_ERROR(PythonManager, "Argument parse error");
        Py_RETURN_NONE;
    }

    LOG_ERROR(module_id, message);    
    Py_RETURN_NONE;
}

static PyObject* log_debug(PyObject *self, PyObject *args)
{
    int module_id;
    char* message;
     
    if(!PyArg_ParseTuple(args, "is:log_error", &module_id, &message))
    {
        LOG_ERROR(PythonManager, "Argument parse error");
        Py_RETURN_NONE;
    }

    LOG_DEBUG(module_id, message);    
    Py_RETURN_NONE;
}

static PyObject* log_advanced(PyObject *self, PyObject *args)
{
    int module_id;
    char* message;
     
    if(!PyArg_ParseTuple(args, "is:log_error", &module_id, &message))
    {
        LOG_ERROR(PythonManager, "Argument parse error");
        Py_RETURN_NONE;
    }

    LOG_ADVANCED(module_id, message);    
    Py_RETURN_NONE;
}

static PyMethodDef LogMethods[] = {
    {"log_info", log_info, METH_VARARGS, "Log with INFO priority"},
    {"log_advanced", log_advanced, METH_VARARGS, "Log with ADVANCED priority"},
    {"log_error", log_error, METH_VARARGS, "Log with ERROR priority"},
    {"log_debug", log_debug, METH_VARARGS, "Log with DEBUG priority"},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef LogModule = {
    PyModuleDef_HEAD_INIT, "logging", NULL, -1, LogMethods,
    NULL, NULL, NULL, NULL
};

PyObject* PyInit_logging(void)
{
    return PyModule_Create(&LogModule);
}