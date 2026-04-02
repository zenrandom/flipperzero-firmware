#include "kyber_crystal_app_i.h"

#include <nfc/nfc_device.h>
#include <nfc/protocols/nfc_protocol.h>

#define TAG "KyberApp"

// ---------------------------------------------------------------------------
// Worker callback — runs on the worker thread, posts custom event to UI
// ---------------------------------------------------------------------------
static bool kyber_worker_event_callback(KyberWorkerEvent event, void* context) {
    furi_assert(context);
    KyberCrystalApp* app = context;
    switch(event) {
    case KyberWorkerEventReadOk:
        view_dispatcher_send_custom_event(app->view_dispatcher, KyberCustomEventWorkerRead);
        break;
    case KyberWorkerEventReadFail:
        view_dispatcher_send_custom_event(app->view_dispatcher, KyberCustomEventWorkerFail);
        break;
    case KyberWorkerEventEmulated:
        view_dispatcher_send_custom_event(app->view_dispatcher, KyberCustomEventWorkerEmulated);
        break;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Navigation callback
// ---------------------------------------------------------------------------
static bool kyber_navigation_callback(void* context) {
    KyberCrystalApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

// ---------------------------------------------------------------------------
// Custom event callback
// ---------------------------------------------------------------------------
static bool kyber_custom_event_callback(void* context, uint32_t event) {
    KyberCrystalApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

// ---------------------------------------------------------------------------
// Save helper — writes a .nfc file compatible with the standard NFC app
// ---------------------------------------------------------------------------
void kyber_crystal_app_save(KyberCrystalApp* app) {
    furi_assert(app);
    furi_assert(app->data_ready);

    NfcDevice* device = nfc_device_alloc();
    nfc_device_set_data(device, NfcProtocolMfClassic, (const NfcDeviceData*)app->mf_data);
    nfc_device_save(device, KYBER_NFC_SAVE_FOLDER "/Kyber_Crystal" KYBER_SAVE_EXTENSION);
    nfc_device_free(device);
}

// ---------------------------------------------------------------------------
// App alloc / free
// ---------------------------------------------------------------------------
KyberCrystalApp* kyber_crystal_app_alloc(void) {
    KyberCrystalApp* app = malloc(sizeof(KyberCrystalApp));
    app->mf_data = mf_classic_alloc();
    app->data_ready = false;
    app->emulate_count = 0;

    // Open services
    app->gui = furi_record_open(RECORD_GUI);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    // View dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, kyber_navigation_callback);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, kyber_custom_event_callback);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Scene manager
    app->scene_manager = scene_manager_alloc(&kyber_crystal_scene_handlers, app);

    // Views
    app->submenu = submenu_alloc();
    view_dispatcher_add_view(app->view_dispatcher, KyberViewMenu, submenu_get_view(app->submenu));

    app->popup = popup_alloc();
    view_dispatcher_add_view(app->view_dispatcher, KyberViewPopup, popup_get_view(app->popup));

    app->widget = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, KyberViewWidget, widget_get_view(app->widget));

    // Worker
    app->worker = kyber_worker_alloc();

    return app;
}

void kyber_crystal_app_free(KyberCrystalApp* app) {
    furi_assert(app);

    kyber_worker_stop(app->worker);
    kyber_worker_free(app->worker);

    view_dispatcher_remove_view(app->view_dispatcher, KyberViewWidget);
    view_dispatcher_remove_view(app->view_dispatcher, KyberViewPopup);
    view_dispatcher_remove_view(app->view_dispatcher, KyberViewMenu);

    widget_free(app->widget);
    popup_free(app->popup);
    submenu_free(app->submenu);

    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);

    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_GUI);

    mf_classic_free(app->mf_data);
    free(app);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int32_t kyber_crystal_app(void* p) {
    UNUSED(p);
    KyberCrystalApp* app = kyber_crystal_app_alloc();

    scene_manager_next_scene(app->scene_manager, KyberSceneStart);
    view_dispatcher_run(app->view_dispatcher);

    kyber_crystal_app_free(app);
    return 0;
}
