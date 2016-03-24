#include "builtin_service.h"

void              builtin_service_manager_init();
service_method_t* builtin_service_manager_get_method(const char* service_class, const char* method_name);
bool              builtin_service_manager_is_class_exists(const char* service_class);

