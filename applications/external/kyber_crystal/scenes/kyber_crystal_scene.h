#pragma once

#include <gui/scene_manager.h>

// Generate scene id enum
#define ADD_SCENE(module, name, id) KyberScene##id,
typedef enum {
#include "kyber_crystal_scene_config.h"
    KyberSceneNum,
} KyberScene;
#undef ADD_SCENE

extern const SceneManagerHandlers kyber_crystal_scene_handlers;

// Generate scene handler declarations
#define ADD_SCENE(module, name, id)                                                \
    void module##_scene_##name##_on_enter(void*);                                  \
    bool module##_scene_##name##_on_event(void*, SceneManagerEvent);               \
    void module##_scene_##name##_on_exit(void*);
#include "kyber_crystal_scene_config.h"
#undef ADD_SCENE
