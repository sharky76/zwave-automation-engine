#pragma once

/*
    Manages multiple scenes
*/
#include <ZPlatform.h>
#include "resolver.h"
#include "data_callbacks.h"
#include "scene.h"
#include <event.h>

void    scene_manager_init();
void    scene_manager_add_scene(const char* name);
void    scene_manager_remove_scene(const char* name);
void    scene_manager_free();
void    scene_manager_on_event(event_t* event);
scene_t*    scene_manager_get_scene(const char* name);
void    scene_manager_for_each(void (*visitor)(scene_t*, void*), void* arg);

