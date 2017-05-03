#pragma once

/*
    Manages multiple scenes
*/
#include <ZPlatform.h>
#include "resolver.h"
#include "data_callbacks.h"
#include "scene.h"
#include "builtin_service.h"
#include <event.h>
#include <hash.h>

void    scene_manager_init();
void    scene_manager_add_scene(const char* name);
void    scene_manager_remove_scene(const char* name);
void    scene_manager_free();
scene_t*    scene_manager_get_scene(const char* name);
void    scene_manager_for_each(void (*visitor)(scene_t*, void*), void* arg);
builtin_service_t*  scene_service_create(hash_table_t* service_table);
