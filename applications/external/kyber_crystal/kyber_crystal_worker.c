#include "kyber_crystal_worker.h"

#include <nfc/nfc_listener.h>
#include <nfc/protocols/mf_classic/mf_classic_poller_sync.h>
#include <nfc/protocols/nfc_protocol.h>

#define TAG "KyberWorker"

// ---------------------------------------------------------------------------
// Key table
// All 16 sectors use the MIFARE factory-default key on genuine Kyber crystals.
// If a sector fails with this key the partial read is still forwarded — the
// caller can decide what to do (show a warning, run a dict attack later, etc.)
// ---------------------------------------------------------------------------
#define KYBER_KEY_SIZE    (6)
#define KYBER_NUM_SECTORS (16)

static const uint8_t KYBER_DEFAULT_KEY[KYBER_KEY_SIZE] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static void kyber_worker_build_keys(MfClassicDeviceKeys* keys) {
    memset(keys, 0, sizeof(MfClassicDeviceKeys));
    for(uint8_t i = 0; i < KYBER_NUM_SECTORS; i++) {
        keys->key_a_mask |= (1ULL << i);
        keys->key_b_mask |= (1ULL << i);
        memcpy(keys->key_a[i].data, KYBER_DEFAULT_KEY, KYBER_KEY_SIZE);
        memcpy(keys->key_b[i].data, KYBER_DEFAULT_KEY, KYBER_KEY_SIZE);
    }
}

// ---------------------------------------------------------------------------
// Listener callback — fires each time the hilt successfully reads a block
// ---------------------------------------------------------------------------
static NfcCommand kyber_listener_callback(NfcGenericEvent event, void* context) {
    UNUSED(event);
    KyberWorker* worker = context;
    if(worker->state != KyberWorkerStateEmulate) {
        return NfcCommandStop;
    }
    // Notify UI that an emulation contact occurred
    if(worker->callback) {
        worker->callback(KyberWorkerEventEmulated, worker->context);
    }
    return NfcCommandContinue;
}

// ---------------------------------------------------------------------------
// Worker thread
// ---------------------------------------------------------------------------
static int32_t kyber_worker_thread(void* ctx) {
    KyberWorker* worker = ctx;

    if(worker->state == KyberWorkerStateRead) {
        // --- READ ---------------------------------------------------------
        Nfc* nfc = nfc_alloc();
        MfClassicDeviceKeys keys;
        kyber_worker_build_keys(&keys);

        // Loop until card found or stopped
        MfClassicError err = MfClassicErrorNotPresent;
        while(worker->state == KyberWorkerStateRead) {
            // Detect card type first (also acts as a presence check)
            err = mf_classic_poller_sync_detect_type(nfc, &worker->mf_data->type);
            if(err == MfClassicErrorNone) {
                FURI_LOG_I(TAG, "Card detected, reading...");
                err = mf_classic_poller_sync_read(nfc, &keys, worker->mf_data);
                if(err == MfClassicErrorNone || err == MfClassicErrorPartialRead) {
                    FURI_LOG_I(
                        TAG,
                        "Read complete (err=%d, type=%d)",
                        err,
                        worker->mf_data->type);
                    if(worker->callback) {
                        worker->callback(KyberWorkerEventReadOk, worker->context);
                    }
                    break;
                }
                FURI_LOG_W(TAG, "Read failed (err=%d), retrying", err);
            }
            furi_delay_ms(300);
        }

        if(worker->state == KyberWorkerStateRead && err != MfClassicErrorNone &&
           err != MfClassicErrorPartialRead) {
            if(worker->callback) {
                worker->callback(KyberWorkerEventReadFail, worker->context);
            }
        }

        nfc_free(nfc);

    } else if(worker->state == KyberWorkerStateEmulate) {
        // --- EMULATE ------------------------------------------------------
        Nfc* nfc = nfc_alloc();
        NfcListener* listener = nfc_listener_alloc(
            nfc, NfcProtocolMfClassic, (const NfcDeviceData*)worker->mf_data);
        nfc_listener_start(listener, kyber_listener_callback, worker);

        // Keep the thread alive until stop is requested
        while(worker->state == KyberWorkerStateEmulate) {
            furi_delay_ms(50);
        }

        nfc_listener_stop(listener);
        nfc_listener_free(listener);
        nfc_free(nfc);
    }

    worker->state = KyberWorkerStateIdle;
    return 0;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
KyberWorker* kyber_worker_alloc(void) {
    KyberWorker* worker = malloc(sizeof(KyberWorker));
    worker->mf_data = mf_classic_alloc();
    worker->thread = furi_thread_alloc_ex("KyberWorker", 4096, kyber_worker_thread, worker);
    worker->state = KyberWorkerStateIdle;
    worker->callback = NULL;
    worker->context = NULL;
    return worker;
}

void kyber_worker_free(KyberWorker* worker) {
    furi_assert(worker);
    mf_classic_free(worker->mf_data);
    furi_thread_free(worker->thread);
    free(worker);
}

void kyber_worker_start_read(KyberWorker* worker, KyberWorkerCallback cb, void* ctx) {
    furi_assert(worker);
    furi_assert(worker->state == KyberWorkerStateIdle);
    worker->callback = cb;
    worker->context = ctx;
    mf_classic_reset(worker->mf_data);
    worker->state = KyberWorkerStateRead;
    furi_thread_start(worker->thread);
}

void kyber_worker_start_emulate(
    KyberWorker* worker,
    MfClassicData* data,
    KyberWorkerCallback cb,
    void* ctx) {
    furi_assert(worker);
    furi_assert(worker->state == KyberWorkerStateIdle);
    worker->callback = cb;
    worker->context = ctx;
    mf_classic_copy(worker->mf_data, data);
    worker->state = KyberWorkerStateEmulate;
    furi_thread_start(worker->thread);
}

void kyber_worker_stop(KyberWorker* worker) {
    furi_assert(worker);
    if(worker->state != KyberWorkerStateIdle) {
        worker->state = KyberWorkerStateStop;
        furi_thread_join(worker->thread);
        worker->state = KyberWorkerStateIdle;
    }
}
