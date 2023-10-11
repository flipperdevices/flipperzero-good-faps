#include "picopass_protocol.h"

#include <furi/furi.h>

#include <mbedtls/des.h>

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
    *data = *other;
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
