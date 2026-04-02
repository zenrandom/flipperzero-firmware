#include "../kyber_crystal_app_i.h"

typedef enum {
    StartMenuRead,
    StartMenuEmulate,
} StartMenuItem;

static void kyber_crystal_scene_start_submenu_callback(void* context, uint32_t index) {
    KyberCrystalApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void kyber_crystal_scene_start_on_enter(void* context) {
    KyberCrystalApp* app = context;

    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Kyber Crystal");
    submenu_add_item(
        app->submenu,
        "Read Crystal",
        StartMenuRead,
        kyber_crystal_scene_start_submenu_callback,
        app);
    if(app->data_ready) {
        submenu_add_item(
            app->submenu,
            "Emulate Crystal",
            StartMenuEmulate,
            kyber_crystal_scene_start_submenu_callback,
            app);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, KyberViewMenu);
}

bool kyber_crystal_scene_start_on_event(void* context, SceneManagerEvent event) {
    KyberCrystalApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == StartMenuRead) {
            scene_manager_next_scene(app->scene_manager, KyberSceneRead);
            consumed = true;
        } else if(event.event == StartMenuEmulate && app->data_ready) {
            scene_manager_next_scene(app->scene_manager, KyberSceneEmulate);
            consumed = true;
        }
    }
    return consumed;
}

void kyber_crystal_scene_start_on_exit(void* context) {
    KyberCrystalApp* app = context;
    submenu_reset(app->submenu);
}
