#include "../kyber_crystal_app.h"

// Generate on_enter handlers array
#define ADD_SCENE(module, name, id) module##_scene_##name##_on_enter,
void (*const kyber_crystal_on_enter_handlers[])(void*) = {
#include "kyber_crystal_scene_config.h"
};
#undef ADD_SCENE

// Generate on_event handlers array
#define ADD_SCENE(module, name, id) module##_scene_##name##_on_event,
bool (*const kyber_crystal_on_event_handlers[])(void*, SceneManagerEvent) = {
#include "kyber_crystal_scene_config.h"
};
#undef ADD_SCENE

// Generate on_exit handlers array
#define ADD_SCENE(module, name, id) module##_scene_##name##_on_exit,
void (*const kyber_crystal_on_exit_handlers[])(void*) = {
#include "kyber_crystal_scene_config.h"
};
#undef ADD_SCENE

const SceneManagerHandlers kyber_crystal_scene_handlers = {
    .on_enter_handlers = kyber_crystal_on_enter_handlers,
    .on_event_handlers = kyber_crystal_on_event_handlers,
    .on_exit_handlers = kyber_crystal_on_exit_handlers,
    .scene_num = KyberSceneNum,
};
