#include "picopass_protocol.h"

#include <furi/furi.h>

#include <mbedtls/des.h>
#include <lfrfid/protocols/lfrfid_protocols.h>
#include <lfrfid/lfrfid_dict_file.h>

#define TAG "PicopassProtocol"

static const uint8_t picopass_protocol_iclass_decryptionkey[] =
    {0xb4, 0x21, 0x2c, 0xca, 0xb7, 0xed, 0x21, 0x0f, 0x7b, 0x93, 0xd4, 0x59, 0x39, 0xc7, 0xdd, 0x36};

static void picopass_protocol_decrypt(uint8_t* enc_data, uint8_t* dec_data) {
    uint8_t key[32] = {0};
    memcpy(
        key,
        picopass_protocol_iclass_decryptionkey,
        sizeof(picopass_protocol_iclass_decryptionkey));
    mbedtls_des3_context ctx;
    mbedtls_des3_init(&ctx);
    mbedtls_des3_set2key_dec(&ctx, key);
    mbedtls_des3_crypt_ecb(&ctx, enc_data, dec_data);
    mbedtls_des3_free(&ctx);
}

PicopassData* picopass_protocol_alloc() {
    PicopassData* instance = malloc(sizeof(PicopassData));

    return instance;
}

void picopass_protocol_free(PicopassData* instance) {
    furi_assert(instance);

    free(instance);
}

void picopass_protocol_copy(PicopassData* data, const PicopassData* other) {
    furi_assert(data);
    furi_assert(other);

    *data = *other;
}

void picopass_protocol_reset(PicopassData* instance) {
    furi_assert(instance);

    memset(instance, 0, sizeof(PicopassData));
}

bool picopass_protocol_save(const PicopassData* instance, FlipperFormat* ff) {
    furi_assert(instance);
    furi_assert(ff);

    bool success = false;
    const PicopassPacs* pacs = &instance->pacs;
    const PicopassBlock* AA1 = instance->AA1;
    FuriString* temp_str = furi_string_alloc();

    do {
        if(!flipper_format_write_hex(ff, "Credential", pacs->credential, PICOPASS_BLOCK_LEN))
            break;

        // TODO: Add elite
        if(!flipper_format_write_comment_cstr(ff, "Picopass blocks")) break;

        size_t app_limit = AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[0] < PICOPASS_MAX_APP_LIMIT ?
                               AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[0] :
                               PICOPASS_MAX_APP_LIMIT;
        bool block_saved = true;
        for(size_t i = 0; i < app_limit; i++) {
            furi_string_printf(temp_str, "Block %d", i);
            if(!flipper_format_write_hex(
                   ff, furi_string_get_cstr(temp_str), AA1[i].data, PICOPASS_BLOCK_LEN)) {
                block_saved = false;
                break;
            }
        }
        if(!block_saved) break;

        success = true;
    } while(false);

    furi_string_free(temp_str);

    return success;
}

bool picopass_protocol_save_as_lfrfid(const PicopassData* instance, const char* path) {
    const PicopassPacs* pacs = &instance->pacs;
    ProtocolDict* dict = protocol_dict_alloc(lfrfid_protocols, LFRFIDProtocolMax);
    ProtocolId protocol = LFRFIDProtocolHidGeneric;

    bool result = false;
    uint64_t target = 0;
    uint64_t sentinel = 1ULL << pacs->bitLength;
    memcpy(&target, pacs->credential, PICOPASS_BLOCK_LEN);
    target = __builtin_bswap64(target);
    FURI_LOG_D(TAG, "Original (%d): %016llx", pacs->bitLength, target);

    if(pacs->bitLength == 26) {
        //3 bytes
        protocol = LFRFIDProtocolH10301;
        // Remove parity
        target = (target >> 1) & 0xFFFFFF;
        // Reverse order since it'll get reversed again
        target = __builtin_bswap64(target) >> (64 - 24);
    } else if(pacs->bitLength < 44) {
        // https://gist.github.com/blark/e8f125e402f576bdb7e2d7b3428bdba6
        protocol = LFRFIDProtocolHidGeneric;
        if(pacs->bitLength <= 36) {
            uint64_t header = 1ULL << 37;
            target = __builtin_bswap64((target | sentinel | header) << 4) >> (64 - 48);
        } else {
            target = __builtin_bswap64((target | sentinel) << 4) >> (64 - 48);
        }
    } else {
        //8 bytes
        protocol = LFRFIDProtocolHidExGeneric;
        target = __builtin_bswap64(target);
    }

    size_t data_size = protocol_dict_get_data_size(dict, protocol);
    uint8_t* data = malloc(data_size);
    if(data_size < 8) {
        memcpy(data, (void*)&target, data_size);
    } else {
        // data_size 12 for LFRFIDProtocolHidExGeneric
        memcpy(data + 4, (void*)&target, 8);
    }

    protocol_dict_set_data(dict, protocol, data, data_size);
    free(data);

    FuriString* briefStr;
    briefStr = furi_string_alloc();
    protocol_dict_render_brief_data(dict, briefStr, protocol);
    FURI_LOG_D(TAG, "LFRFID Brief: %s", furi_string_get_cstr(briefStr));
    furi_string_free(briefStr);

    result = lfrfid_dict_file_save(dict, protocol, path);
    if(result) {
        FURI_LOG_D(TAG, "Written: %d", result);
    } else {
        FURI_LOG_D(TAG, "Failed to write");
    }

    protocol_dict_free(dict);

    return result;
}

bool picopass_protocol_load(PicopassData* instance, FlipperFormat* ff, uint32_t version) {
    furi_assert(instance);
    furi_assert(ff);
    UNUSED(version);

    bool success = false;
    PicopassBlock* AA1 = instance->AA1;
    FuriString* temp_str = furi_string_alloc();

    do {
        // Parse header blocks
        bool block_read = true;
        for(size_t i = 0; i < 6; i++) {
            furi_string_printf(temp_str, "Block %d", i);
            if(!flipper_format_read_hex(
                   ff, furi_string_get_cstr(temp_str), AA1[i].data, PICOPASS_BLOCK_LEN)) {
                block_read = false;
                break;
            }
        }

        size_t app_limit = AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[0];
        // Fix for unpersonalized cards that have app_limit set to 0xFF
        if(app_limit > PICOPASS_MAX_APP_LIMIT) app_limit = PICOPASS_MAX_APP_LIMIT;
        for(size_t i = 6; i < app_limit; i++) {
            furi_string_printf(temp_str, "Block %d", i);
            if(!flipper_format_read_hex(
                   ff, furi_string_get_cstr(temp_str), AA1[i].data, PICOPASS_BLOCK_LEN)) {
                block_read = false;
                break;
            }
        }
        if(!block_read) break;

        picopass_protocol_parse_credential(instance);
        picopass_protocol_parse_wiegand(instance);

        success = true;
    } while(false);

    furi_string_free(temp_str);

    return success;
}

void picopass_protocol_parse_credential(PicopassData* instance) {
    furi_assert(instance);

    instance->pacs.biometrics = instance->AA1[6].data[4];
    instance->pacs.pin_length = instance->AA1[6].data[6] & 0x0F;
    instance->pacs.encryption = instance->AA1[6].data[7];

    if(instance->pacs.encryption == PicopassDeviceEncryption3DES) {
        FURI_LOG_D(TAG, "3DES Encrypted");
        picopass_protocol_decrypt(instance->AA1[7].data, instance->pacs.credential);
        picopass_protocol_decrypt(instance->AA1[8].data, instance->pacs.pin0);
        picopass_protocol_decrypt(instance->AA1[9].data, instance->pacs.pin1);
    } else if(instance->pacs.encryption == PicopassDeviceEncryptionNone) {
        FURI_LOG_D(TAG, "No Encryption");
        memcpy(instance->pacs.credential, instance->AA1[7].data, PICOPASS_BLOCK_LEN);
        memcpy(instance->pacs.pin0, instance->AA1[8].data, PICOPASS_BLOCK_LEN);
        memcpy(instance->pacs.pin1, instance->AA1[9].data, PICOPASS_BLOCK_LEN);
    } else if(instance->pacs.encryption == PicopassDeviceEncryptionDES) {
        FURI_LOG_D(TAG, "DES Encrypted");
    } else {
        FURI_LOG_D(TAG, "Unknown encryption");
    }

    instance->pacs.sio = (instance->AA1[10].data[0] == 0x30); // rough check
}

void picopass_protocol_parse_wiegand(PicopassData* instance) {
    furi_assert(instance);

    uint32_t* halves = (uint32_t*)instance->pacs.credential;
    if(halves[0] == 0) {
        uint8_t leading0s = __builtin_clz(REVERSE_BYTES_U32(halves[1]));
        instance->pacs.bitLength = 31 - leading0s;
    } else {
        uint8_t leading0s = __builtin_clz(REVERSE_BYTES_U32(halves[0]));
        instance->pacs.bitLength = 63 - leading0s;
    }

    // Remove sentinel bit from credential.  Byteswapping to handle array of bytes vs 64bit value
    uint64_t sentinel = __builtin_bswap64(1ULL << instance->pacs.bitLength);
    uint64_t swapped = 0;
    memcpy(&swapped, instance->pacs.credential, sizeof(uint64_t));
    swapped = swapped ^ sentinel;
    memcpy(instance->pacs.credential, &swapped, sizeof(uint64_t));
    FURI_LOG_D(TAG, "instance->pacs: (%d) %016llx", instance->pacs.bitLength, swapped);
}
