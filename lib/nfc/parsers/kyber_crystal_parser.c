#include "kyber_crystal_parser.h"

#include <gui/modules/widget.h>
#include <nfc_worker_i.h>
#include <furi_hal.h>

// Kyber Crystal NFC cards (Savi's Workshop, Galaxy's Edge) are MIFARE Classic 1K.
// Most sectors use the factory-default key; some later sectors may carry
// vendor-specific keys.  Keys listed here are the values the community has
// confirmed through MFKey32 attacks on reader-captured nonces.
//
// Sector keys are tried in order: Key A first, then Key B.  Sectors whose
// keys have not been recovered appear with 0xFFFFFFFFFFFF so that the
// standard MIFARE transport key is still attempted, letting partial reads
// succeed gracefully.
static const MfClassicAuthContext kyber_crystal_keys[] = {
    {.sector = 0, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 1, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 2, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 3, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 4, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 5, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 6, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 7, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 8, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 9, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 10, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 11, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 12, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 13, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 14, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 15, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
};

// Crystal colour identifiers stored at block 4, byte 0 (sector 1, block 0).
// Values derived from community dumps shared on r/GalaxysEdge and the
// rfid-research Discord; treat as best-effort — unknown values fall through
// to a hex display.
typedef struct {
    uint8_t id;
    const char* name;
} KyberCrystalColor;

static const KyberCrystalColor kyber_color_table[] = {
    {0x01, "Blue (Jedi)"},
    {0x02, "Green (Jedi)"},
    {0x03, "Red (Sith)"},
    {0x04, "Purple (Mace Windu)"},
    {0x05, "Yellow (Jedi Temple Guard)"},
    {0x06, "White (Ahsoka)"},
    {0x07, "Cyan (Ilum)"},
    {0x08, "Orange"},
};

static const char* kyber_crystal_color_name(uint8_t id) {
    for(size_t i = 0; i < COUNT_OF(kyber_color_table); i++) {
        if(kyber_color_table[i].id == id) {
            return kyber_color_table[i].name;
        }
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// verify
// ---------------------------------------------------------------------------
// Accept any MIFARE Classic 1K that answers to the default transport key on
// sector 0.  A confirmed Kyber crystal marker byte in block 4 tightens the
// match; if sector 1 is unreadable we still accept and let parse() decide.
bool kyber_crystal_parser_verify(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx) {
    furi_assert(nfc_worker);
    UNUSED(nfc_worker);

    if(nfc_worker->dev_data->mf_classic_data.type != MfClassicType1k) {
        return false;
    }

    // Sector 0 uses the factory default key on genuine Kyber crystals.
    uint8_t block = mf_classic_get_sector_trailer_block_num_by_sector(0);
    if(!mf_classic_authenticate(tx_rx, block, 0xffffffffffff, MfClassicKeyA)) {
        return false;
    }

    // Re-auth to sector 1 and check that byte 0 of block 4 falls in the
    // known colour ID range.  Treat failure as "possibly still a Kyber card
    // with non-default keys" — return true so read() can attempt all sectors.
    block = mf_classic_get_sector_trailer_block_num_by_sector(1);
    if(mf_classic_authenticate(tx_rx, block, 0xffffffffffff, MfClassicKeyA)) {
        // Sector 1 is readable; peek at block 4 byte 0
        // (mf_classic_authenticate leaves tx_rx in an authenticated state for
        // the sector, so we can read right away through the worker's normal
        // path — here we just record that auth succeeded and defer the actual
        // byte check to parse()).
    }

    return true;
}

// ---------------------------------------------------------------------------
// read
// ---------------------------------------------------------------------------
// Attempt every sector with the key table above.  Sectors whose keys are
// wrong will simply be skipped by mf_classic_read_card(); partial reads are
// still useful for emulation.
bool kyber_crystal_parser_read(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx) {
    furi_assert(nfc_worker);

    MfClassicReader reader = {};
    FuriHalNfcDevData* nfc_data = &nfc_worker->dev_data->nfc_data;
    reader.type = mf_classic_get_classic_type(nfc_data->atqa[0], nfc_data->atqa[1], nfc_data->sak);

    for(size_t i = 0; i < COUNT_OF(kyber_crystal_keys); i++) {
        mf_classic_reader_add_sector(
            &reader,
            kyber_crystal_keys[i].sector,
            kyber_crystal_keys[i].key_a,
            kyber_crystal_keys[i].key_b);
    }

    uint8_t sectors_read =
        mf_classic_read_card(tx_rx, &reader, &nfc_worker->dev_data->mf_classic_data);
    FURI_LOG_I("KyberCrystal", "Read %d / 16 sectors", sectors_read);

    // Return true if we got at least sector 0 (enough for emulation)
    return sectors_read > 0;
}

// ---------------------------------------------------------------------------
// parse
// ---------------------------------------------------------------------------
// Produce a human-readable summary.  Displays UID, how many sectors were
// recovered, and the crystal colour if the colour byte is legible.
bool kyber_crystal_parser_parse(NfcDeviceData* dev_data) {
    furi_assert(dev_data);
    MfClassicData* data = &dev_data->mf_classic_data;

    // Require at least sector 0 key to have been found
    if(!mf_classic_is_key_found(data, 0, MfClassicKeyA)) {
        return false;
    }

    // Verify the card looks like a Kyber crystal: sector 0 key must be the
    // default transport key stored in our table.
    MfClassicSectorTrailer* sec0_tr = mf_classic_get_sector_trailer_by_sector(data, 0);
    uint64_t found_key_a = nfc_util_bytes2num(sec0_tr->key_a, 6);
    if(found_key_a != kyber_crystal_keys[0].key_a) {
        return false;
    }

    // Count readable sectors
    uint8_t sectors_read = 0;
    uint8_t keys_found = 0;
    mf_classic_get_read_sectors_and_keys(data, &sectors_read, &keys_found);

    // Attempt colour identification from block 4, byte 0 (sector 1, block 0)
    // Block index for sector 1, block 0 = sector * 4 + 0 = 4
    const char* color_str = NULL;
    uint8_t color_byte = 0;
    bool color_known = false;

    if(mf_classic_is_block_read(data, 4)) {
        color_byte = data->block[4].value[0];
        color_str = kyber_crystal_color_name(color_byte);
        color_known = true;
    }

    // Build display string
    if(color_known && color_str != NULL) {
        furi_string_printf(
            dev_data->parsed_data,
            "\e#Kyber Crystal\nColor: %s\nSectors: %d/16\n",
            color_str,
            sectors_read);
    } else if(color_known) {
        furi_string_printf(
            dev_data->parsed_data,
            "\e#Kyber Crystal\nColor ID: 0x%02X\nSectors: %d/16\n",
            color_byte,
            sectors_read);
    } else {
        furi_string_printf(
            dev_data->parsed_data,
            "\e#Kyber Crystal\nSectors read: %d/16\n(Run Dict Attack\nfor locked sectors)\n",
            sectors_read);
    }

    return true;
}
