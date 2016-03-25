#include "builtin_service.h"

void              builtin_service_manager_init();
service_method_t* builtin_service_manager_get_method(const char* service_class, const char* method_name);
bool              builtin_service_manager_is_class_exists(const char* service_class);
builtin_service_t* builtin_service_manager_get_class(const char* service_class);
void              builtin_service_manager_for_each_class(void (*visitor)(builtin_service_t*, void*), void* arg);
void              builtin_service_manager_for_each_method(const char* service_class, void (*visitor)(service_method_t*, void*), void* arg);

