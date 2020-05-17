#pragma once

#include <python3.7m/Python.h>

typedef struct module_entry_t
{
    char* name;
    int   module_id;
    PyObject* module;
    PyObject* context;
} module_entry_t;

void python_manager_init(const char* python_dir);
void python_manager_stop();
const char* python_manager_name_from_id(int module_id);
const int python_manager_id_from_ptr(PyObject* pModule);
PyObject** python_manager_get_context(int module_id);
module_entry_t* python_manager_get_module(int module_id);
