#include "../kyber_crystal_app_i.h"

static bool kyber_read_worker_callback(KyberWorkerEvent event, void* context) {
    KyberCrystalApp* app = context;
    if(event == KyberWorkerEventReadOk) {
        view_dispatcher_send_custom_event(app->view_dispatcher, KyberCustomEventWorkerRead);
    } else {
        view_dispatcher_send_custom_event(app->view_dispatcher, KyberCustomEventWorkerFail);
    }
    return true;
}

void kyber_crystal_scene_read_on_enter(void* context) {
    KyberCrystalApp* app = context;

    popup_reset(app->popup);
    popup_set_header(app->popup, "Reading", 64, 10, AlignCenter, AlignTop);
    popup_set_text(
        app->popup, "Hold crystal\nto Flipper", 64, 36, AlignCenter, AlignCenter);

    view_dispatcher_switch_to_view(app->view_dispatcher, KyberViewPopup);

    kyber_worker_start_read(app->worker, kyber_read_worker_callback, app);
}

bool kyber_crystal_scene_read_on_event(void* context, SceneManagerEvent event) {
    KyberCrystalApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == KyberCustomEventWorkerRead) {
            // Copy read data into app storage and advance to result scene
            mf_classic_copy(app->mf_data, app->worker->mf_data);
            app->data_ready = true;
            scene_manager_next_scene(app->scene_manager, KyberSceneResult);
            consumed = true;
        } else if(event.event == KyberCustomEventWorkerFail) {
            popup_set_header(app->popup, "No Crystal Found", 64, 10, AlignCenter, AlignTop);
            popup_set_text(
                app->popup,
                "Place crystal\nflat on back\nand try again",
                64,
                36,
                AlignCenter,
                AlignCenter);
            consumed = true;
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        kyber_worker_stop(app->worker);
        // consumed = false → scene_manager pops to start
    }
    return consumed;
}

void kyber_crystal_scene_read_on_exit(void* context) {
    KyberCrystalApp* app = context;
    kyber_worker_stop(app->worker);
    popup_reset(app->popup);
}
