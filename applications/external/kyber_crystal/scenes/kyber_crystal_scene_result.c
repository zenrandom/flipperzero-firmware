#include "../kyber_crystal_app_i.h"

typedef enum {
    ResultMenuEmulate,
    ResultMenuSave,
    ResultMenuReadAgain,
} ResultMenuItem;

static void kyber_crystal_scene_result_submenu_callback(void* context, uint32_t index) {
    KyberCrystalApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

// Count how many sectors were fully read
static uint8_t kyber_count_sectors_read(const MfClassicData* data) {
    uint8_t total = mf_classic_get_total_sectors_num(data->type);
    uint8_t count = 0;
    for(uint8_t s = 0; s < total; s++) {
        uint8_t trailer = mf_classic_get_sector_trailer_num_by_sector(s);
        // A sector is considered read if the trailer block bit is set
        if((data->block_read_mask[trailer / 32] >> (trailer % 32)) & 1) {
            count++;
        }
    }
    return count;
}

void kyber_crystal_scene_result_on_enter(void* context) {
    KyberCrystalApp* app = context;
    furi_assert(app->data_ready);

    const MfClassicData* data = app->mf_data;
    uint8_t total = mf_classic_get_total_sectors_num(data->type);
    uint8_t read = kyber_count_sectors_read(data);

    // Try to get the crystal colour from block 4, byte 0
    // block 4 = sector 1 block 0; it is read if its bit is set
    const char* color_str = NULL;
    bool block4_read = (data->block_read_mask[0] >> 4) & 1;
    uint8_t color_byte = 0;
    if(block4_read) {
        color_byte = data->block[4].data[0];
        color_str = kyber_color_name(color_byte);
    }

    submenu_reset(app->submenu);

    // Header line — build into a FuriString so we can format it
    FuriString* header = furi_string_alloc();
    if(color_str) {
        furi_string_printf(header, "Crystal: %s", color_str);
    } else if(block4_read) {
        furi_string_printf(header, "Crystal ID: 0x%02X", color_byte);
    } else {
        furi_string_set(header, "Crystal Read");
    }
    // Append sector count as a sub-line via the submenu header
    FuriString* full_header = furi_string_alloc();
    furi_string_printf(
        full_header, "%s\n%d/%d sectors", furi_string_get_cstr(header), read, total);
    submenu_set_header(app->submenu, furi_string_get_cstr(full_header));
    furi_string_free(full_header);
    furi_string_free(header);

    submenu_add_item(
        app->submenu,
        "Emulate",
        ResultMenuEmulate,
        kyber_crystal_scene_result_submenu_callback,
        app);
    submenu_add_item(
        app->submenu,
        "Save (.nfc)",
        ResultMenuSave,
        kyber_crystal_scene_result_submenu_callback,
        app);
    submenu_add_item(
        app->submenu,
        "Read Again",
        ResultMenuReadAgain,
        kyber_crystal_scene_result_submenu_callback,
        app);

    view_dispatcher_switch_to_view(app->view_dispatcher, KyberViewMenu);
}

bool kyber_crystal_scene_result_on_event(void* context, SceneManagerEvent event) {
    KyberCrystalApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == ResultMenuEmulate) {
            scene_manager_next_scene(app->scene_manager, KyberSceneEmulate);
            consumed = true;
        } else if(event.event == ResultMenuSave) {
            kyber_crystal_app_save(app);
            // Brief feedback via popup
            popup_reset(app->popup);
            popup_set_header(app->popup, "Saved!", 64, 28, AlignCenter, AlignCenter);
            popup_set_timeout(app->popup, 1200);
            popup_enable_timeout(app->popup);
            popup_set_context(app->popup, app);
            popup_set_callback(app->popup, NULL);
            view_dispatcher_switch_to_view(app->view_dispatcher, KyberViewPopup);
            consumed = true;
        } else if(event.event == ResultMenuReadAgain) {
            scene_manager_next_scene(app->scene_manager, KyberSceneRead);
            consumed = true;
        }
    }
    return consumed;
}

void kyber_crystal_scene_result_on_exit(void* context) {
    KyberCrystalApp* app = context;
    submenu_reset(app->submenu);
    popup_reset(app->popup);
}
