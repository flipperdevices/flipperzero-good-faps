#include "gen4.h"
#include "core/check.h"

Gen4* gen4_alloc() {
    Gen4* instance = malloc(sizeof(Gen4));

    return instance;
}

void gen4_free(Gen4* instance) {
    furi_check(instance);

    free(instance);
}

void gen4_reset(Gen4* instance) {
    furi_check(instance);

    memset(&instance->config, 0, sizeof(Gen4Config));
    memset(&instance->revision, 0, sizeof(Gen4Revision));
}

void gen4_copy(Gen4* dest, const Gen4* source) {
    furi_check(dest);
    furi_check(source);

    memcpy(dest, source, sizeof(Gen4));
}

bool gen4_password_is_set(const Gen4Password* instance) {
    furi_check(instance);

    return (instance->bytes[0] || instance->bytes[1] || instance->bytes[2] || instance->bytes[3]);
}

void gen4_password_reset(Gen4Password* instance) {
    furi_check(instance);

    memset(instance->bytes, 0, GEN4_PASSWORD_LEN);
}

void gen4_password_copy(Gen4Password* dest, const Gen4Password* source) {
    furi_check(dest);
    furi_check(source);

    memcpy(dest->bytes, source->bytes, GEN4_PASSWORD_LEN);
}

const char* gen4_get_shadow_mode_name(Gen4ShadowMode mode) {
    switch(mode) {
    case Gen4ShadowModePreWrite:
        return "Pre-Write";
    case Gen4ShadowModeRestore:
        return "Restore";
    case Gen4ShadowModeDisabled:
        return "Disabled";
    case Gen4ShadowModeHighSpeedDisabled:
        return "Disabled (High-speed)";
    case Gen4ShadowModeSplit:
        return "Split";
    default:
        return "Unknown";
    }
}

const char* gen4_get_direct_write_mode_name(Gen4DirectWriteBlock0Mode mode) {
    switch(mode) {
    case Gen4DirectWriteBlock0ModeEnabled:
        return "Enabled";
    case Gen4DirectWriteBlock0ModeDisabled:
        return "Disabled";
    case Gen4DirectWriteBlock0ModeDefault:
        return "Default";
    default:
        return "Unknown";
    }
}

const char* gen4_get_uid_len_num(Gen4UIDLength code) {
    switch(code) {
    case Gen4UIDLengthSingle:
        return "4";
    case Gen4UIDLengthDouble:
        return "7";
    case Gen4UIDLengthTriple:
        return "10";
    default:
        return "Unknown";
    }
}

const char* gen4_get_configuration_name(const Gen4Config* config) {
    switch(config->data_parsed.protocol) {
    case Gen4ProtocolMfClassic: {
        switch(config->data_parsed.total_blocks) {
        case 255:
            return "MIFARE Classic 4K";
        case 63:
            return "MIFARE Classic 1K";
        case 19:
            return "MIFARE Classic Mini (0.3K)";
        default:
            return "Unknown";
        }
    } break;
    case Gen4ProtocolMfUltralight: {
        switch(config->data_parsed.total_blocks) {
        case 63:
            return "MIFARE Ultralight";
        case 127:
            return "NTAG 2XX";
        default:
            return "Unknown";
        }
    } break;
    default:
        return "Unknown";
        break;
    };
}

void gen4_calc_mfc_block0(const uint8_t* uid, uint8_t uid_len, uint8_t* block0) {
    block0[0] = uid[0];
    block0[1] = uid[1];
    block0[2] = uid[2];
    block0[3] = uid[3];
    uint8_t bcc = uid[0] ^ uid[1] ^ uid[2] ^ uid[3];
    for(uint8_t i = 4; i < uid_len; i++) {
        block0[i] = uid[i];
        bcc ^= uid[i];
    }
    block0[uid_len] = bcc;
}

void gen4_calc_mfu_bcc(const uint8_t* uid, uint8_t* bcc0, uint8_t* bcc1) {
    uint8_t bcc0_val = uid[0] ^ uid[1] ^ uid[2] ^ 0x88;
    uint8_t bcc1_val = uid[3] ^ uid[4] ^ uid[5] ^ uid[6];
    *bcc0 = bcc0_val;
    *bcc1 = bcc1_val;
}

bool gen4_hex_str_to_bytes(const char* hex_str, uint8_t* bytes, uint8_t len) {
    for(uint8_t i = 0; i < len; i++) {
        char h = hex_str[i * 2];
        char l = hex_str[i * 2 + 1];
        uint8_t val = 0;

        if(h >= '0' && h <= '9') {
            val = (h - '0') << 4;
        } else if(h >= 'A' && h <= 'F') {
            val = (h - 'A' + 10) << 4;
        } else if(h >= 'a' && h <= 'f') {
            val = (h - 'a' + 10) << 4;
        } else {
            return false;
        }

        if(l >= '0' && l <= '9') {
            val |= (l - '0');
        } else if(l >= 'A' && l <= 'F') {
            val |= (l - 'A' + 10);
        } else if(l >= 'a' && l <= 'f') {
            val |= (l - 'a' + 10);
        } else {
            return false;
        }

        bytes[i] = val;
    }
    return true;
}

void gen4_bytes_to_hex_str(const uint8_t* bytes, uint8_t len, char* hex_str) {
    for(uint8_t i = 0; i < len; i++) {
        hex_str[i * 2] = "0123456789ABCDEF"[bytes[i] >> 4];
        hex_str[i * 2 + 1] = "0123456789ABCDEF"[bytes[i] & 0x0F];
    }
    hex_str[len * 2] = '\0';
}
