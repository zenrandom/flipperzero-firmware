#include "../kyber_crystal_app_i.h"

static bool kyber_emulate_worker_callback(KyberWorkerEvent event, void* context) {
    KyberCrystalApp* app = context;
    if(event == KyberWorkerEventEmulated) {
        view_dispatcher_send_custom_event(app->view_dispatcher, KyberCustomEventWorkerEmulated);
    }
    return true;
}

static void kyber_crystal_scene_emulate_refresh(KyberCrystalApp* app) {
    popup_reset(app->popup);
    popup_set_header(app->popup, "Emulating", 64, 10, AlignCenter, AlignTop);

    // Show crystal colour if known
    const char* color_str = NULL;
    bool block4_read = (app->mf_data->block_read_mask[0] >> 4) & 1;
    if(block4_read) {
        color_str = kyber_color_name(app->mf_data->block[4].data[0]);
    }

    FuriString* body = furi_string_alloc();
    if(color_str) {
        furi_string_printf(body, "%s\n\nContacts: %lu", color_str, app->emulate_count);
    } else {
        furi_string_printf(body, "Kyber Crystal\n\nContacts: %lu", app->emulate_count);
    }
    popup_set_text(app->popup, furi_string_get_cstr(body), 64, 36, AlignCenter, AlignCenter);
    furi_string_free(body);
}

void kyber_crystal_scene_emulate_on_enter(void* context) {
    KyberCrystalApp* app = context;
    furi_assert(app->data_ready);

    app->emulate_count = 0;
    kyber_crystal_scene_emulate_refresh(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, KyberViewPopup);

    kyber_worker_start_emulate(app->worker, app->mf_data, kyber_emulate_worker_callback, app);
}

bool kyber_crystal_scene_emulate_on_event(void* context, SceneManagerEvent event) {
    KyberCrystalApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       event.event == KyberCustomEventWorkerEmulated) {
        app->emulate_count++;
        kyber_crystal_scene_emulate_refresh(app);
        consumed = true;
    } else if(event.type == SceneManagerEventTypeBack) {
        kyber_worker_stop(app->worker);
        // Let scene_manager pop back
    }
    return consumed;
}

void kyber_crystal_scene_emulate_on_exit(void* context) {
    KyberCrystalApp* app = context;
    kyber_worker_stop(app->worker);
    popup_reset(app->popup);
}
