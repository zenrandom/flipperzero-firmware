#pragma once

#include <furi.h>
#include <nfc/nfc.h>
#include <nfc/protocols/mf_classic/mf_classic.h>

typedef enum {
    KyberWorkerStateIdle,
    KyberWorkerStateRead,
    KyberWorkerStateEmulate,
    KyberWorkerStateStop,
} KyberWorkerState;

typedef enum {
    KyberWorkerEventReadOk,
    KyberWorkerEventReadFail,
    KyberWorkerEventEmulated,
} KyberWorkerEvent;

typedef void (*KyberWorkerCallback)(KyberWorkerEvent event, void* context);

typedef struct {
    FuriThread* thread;
    KyberWorkerState state;
    KyberWorkerCallback callback;
    void* context;
    MfClassicData* mf_data; // filled on successful read; caller owns
} KyberWorker;

KyberWorker* kyber_worker_alloc(void);
void kyber_worker_free(KyberWorker* worker);

// Start reading — worker fills mf_data and fires ReadOk/ReadFail
void kyber_worker_start_read(KyberWorker* worker, KyberWorkerCallback cb, void* ctx);

// Start emulating — worker fires Emulated each time the hilt reads the card
void kyber_worker_start_emulate(
    KyberWorker* worker,
    MfClassicData* data,
    KyberWorkerCallback cb,
    void* ctx);

void kyber_worker_stop(KyberWorker* worker);
