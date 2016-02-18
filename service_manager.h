#pragma once

/*
    This is the library of built-in function classes and methods
    Built-in method has folloing syntax:
 
    [Function class].[method](args)
 
    Example:
 
    Scene.SetEnabled(True) -> Enables the scene
    DateTime.GetTimeSec -> returns number from 0 to 86400
    Timer.Delay(30, LightSwitch.0.SwitchBinary.Set(Off))
*/
#include <variant.h>
#include <service.h>
#include <event.h>

void                service_manager_init(const char* service_dir);
variant_t*          service_manager_eval_method(service_method_t* service, ...);
service_method_t*   service_manager_get_method(const char* service_class, const char* name);
service_t*          service_manager_get_class(const char* service_class);
service_t*          service_manager_get_class_by_id(int service_id);
bool                service_manager_is_class_exists(const char* service_class);
void                service_manager_for_each_class(void (*visitor)(service_t*, void*), void* arg);
void                service_manager_for_each_method(const char* service_class, void (*visitor)(service_method_t*, void*), void* arg);
void                service_manager_on_event(event_t* event);



