#pragma once

// Kyber Crystal NFC Reader / Emulator
// Targets: RogueMaster / Unleashed firmware (NFC poller API, ~0.90+)
//
// Build:  ./fbt fap_kyber_crystal  (from the RogueMaster firmware root)
// Install: copy .fap to /ext/apps/NFC/ via qFlipper or SD card

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <notification/notification_messages.h>
#include <gui/modules/submenu.h>
#include <gui/modules/popup.h>
#include <gui/modules/widget.h>
#include <storage/storage.h>

#include <nfc/nfc.h>
#include <nfc/nfc_device.h>
#include <nfc/protocols/mf_classic/mf_classic.h>
#include <nfc/protocols/mf_classic/mf_classic_poller_sync.h>
#include <nfc/nfc_listener.h>

#include "scenes/kyber_crystal_scene.h"
#include "kyber_crystal_worker.h"

#define KYBER_NFC_SAVE_FOLDER EXT_PATH("nfc")
#define KYBER_SAVE_EXTENSION  ".nfc"

typedef enum {
    KyberViewMenu,
    KyberViewPopup,
    KyberViewWidget,
} KyberView;

typedef enum {
    KyberCustomEventWorkerRead,
    KyberCustomEventWorkerFail,
    KyberCustomEventWorkerEmulated,
} KyberCustomEvent;

typedef struct KyberCrystalApp KyberCrystalApp;

KyberCrystalApp* kyber_crystal_app_alloc(void);
void kyber_crystal_app_free(KyberCrystalApp* app);
