#pragma once

#include <python3.7m/Python.h>

void python_manager_init(const char* python_dir);
void python_manager_stop();
const char* python_manager_name_from_id(int module_id);
const int python_manager_id_from_ptr(PyObject* pModule);
void** python_manager_get_context(int module_id);
