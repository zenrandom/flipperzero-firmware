#pragma once

#include "kyber_crystal_app.h"

// Crystal colour table (block 4, byte 0)
typedef struct {
    uint8_t id;
    const char* name;
} KyberColorEntry;

static const KyberColorEntry KYBER_COLORS[] = {
    {0x01, "Blue (Jedi)"},
    {0x02, "Green (Jedi)"},
    {0x03, "Red (Sith)"},
    {0x04, "Purple (Mace Windu)"},
    {0x05, "Yellow (Jedi Temple Guard)"},
    {0x06, "White (Ahsoka)"},
    {0x07, "Cyan (Ilum)"},
    {0x08, "Orange"},
};

static inline const char* kyber_color_name(uint8_t id) {
    for(size_t i = 0; i < sizeof(KYBER_COLORS) / sizeof(KYBER_COLORS[0]); i++) {
        if(KYBER_COLORS[i].id == id) return KYBER_COLORS[i].name;
    }
    return NULL;
}

struct KyberCrystalApp {
    // Services
    Gui* gui;
    NotificationApp* notifications;

    // Framework
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;

    // Views
    Submenu* submenu;
    Popup* popup;
    Widget* widget;

    // Worker
    KyberWorker* worker;

    // Card data — valid after a successful read
    MfClassicData* mf_data;
    bool data_ready;

    // Emulation contact counter (shown on emulate screen)
    uint32_t emulate_count;
};

// Called from scenes
void kyber_crystal_app_save(KyberCrystalApp* app);
