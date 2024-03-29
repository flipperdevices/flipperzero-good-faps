#pragma once

#include <gui/scene_manager.h>

// Generate scene id and total number
#define ADD_SCENE(name, id) Scene##id,
typedef enum {
#include "scene_config.h"
    SceneNum,
} Scene;
#undef ADD_SCENE

extern const SceneManagerHandlers scene_handlers;

// Generate scene on_enter handlers declaration
#define ADD_SCENE(name, id) void scene_##name##_on_enter(void*);
#include "scene_config.h"
#undef ADD_SCENE

// Generate scene on_event handlers declaration
#define ADD_SCENE(name, id) bool scene_##name##_on_event(void* context, SceneManagerEvent event);
#include "scene_config.h"
#undef ADD_SCENE

// Generate scene on_exit handlers declaration
#define ADD_SCENE(name, id) void scene_##name##_on_exit(void* context);
#include "scene_config.h"
#undef ADD_SCENE
