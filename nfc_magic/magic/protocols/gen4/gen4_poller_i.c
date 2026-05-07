#include "gen4_poller_i.h"

#include "bit_buffer.h"
#include "magic/protocols/gen4/gen4_poller.h"
#include <nfc/protocols/iso14443_3a/iso14443_3a_poller.h>

#define GEN4_CMD_PREFIX (0xCF)

#define GEN4_CMD_SET_SHD_MODE (0x32)
#define GEN4_CMD_SET_ATS (0x34)
#define GEN4_CMD_SET_ATQA_SAK (0x35)
#define GEN4_CMD_SET_UL_PROTOCOL (0x69)
#define GEN4_CMD_SET_UL_MODE (0x6A)
#define GEN4_CMD_SET_MAX_RW_BLOCK (0x6B)
#define GEN4_CMD_GET_CFG (0xC6)
#define GEN4_CMD_GET_REVISION (0xCC)
#define GEN4_CMD_WRITE (0xCD)
#define GEN4_CMD_READ (0xCE)
#define GEN4_CMD_SET_DW_BLOCK_0 (0xCF)
#define GEN4_CMD_SET_CFG (0xF0)
#define GEN4_CMD_FUSE_CFG (0xF1)
#define GEN4_CMD_SET_PWD (0xFE)

#define GEN4_RESPONSE_SUCCESS (0x02)

static Gen4PollerError gen4_poller_process_error(Iso14443_3aError error) {
    Gen4PollerError ret = Gen4PollerErrorNone;

    if(error == Iso14443_3aErrorNone) {
        ret = Gen4PollerErrorNone;
    } else {
        ret = Gen4PollerErrorTimeout;
    }

    return ret;
}

Gen4PollerError
    gen4_poller_set_shadow_mode(Gen4Poller* instance, Gen4Password password, Gen4ShadowMode mode) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, password.bytes, GEN4_PASSWORD_LEN);
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_SET_SHD_MODE);
        bit_buffer_append_byte(instance->tx_buffer, mode);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t response = bit_buffer_get_size_bytes(instance->rx_buffer);

        FURI_LOG_D(TAG, "Card response: 0x%02X, Shadow mode set: 0x%02X", response, mode);

        if(response != GEN4_RESPONSE_SUCCESS) {
            ret = Gen4PollerErrorProtocol;
            break;
        }

    } while(false);

    return ret;
}

Gen4PollerError gen4_poller_set_direct_write_block_0_mode(
    Gen4Poller* instance,
    Gen4Password password,
    Gen4DirectWriteBlock0Mode mode) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, password.bytes, GEN4_PASSWORD_LEN);
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_SET_DW_BLOCK_0);
        bit_buffer_append_byte(instance->tx_buffer, mode);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }
        size_t response = bit_buffer_get_size_bytes(instance->rx_buffer);

        FURI_LOG_D(
            TAG, "Card response: 0x%02X, Direct write to block 0 mode set: 0x%02X", response, mode);

        if(response != GEN4_RESPONSE_SUCCESS) {
            ret = Gen4PollerErrorProtocol;
            break;
        }

    } while(false);

    return ret;
}

Gen4PollerError
    gen4_poller_get_config(Gen4Poller* instance, Gen4Password password, Gen4Config* config_result) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, password.bytes, GEN4_PASSWORD_LEN);
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_GET_CFG);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t rx_bytes = bit_buffer_get_size_bytes(instance->rx_buffer);

        if(rx_bytes != GEN4_CONFIG_SIZE) {
            ret = Gen4PollerErrorProtocol;
            break;
        }
        bit_buffer_write_bytes(instance->rx_buffer, config_result->data_raw, GEN4_CONFIG_SIZE);
    } while(false);

    return ret;
}

Gen4PollerError gen4_poller_get_revision(
    Gen4Poller* instance,
    Gen4Password password,
    Gen4Revision* revision_result) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, password.bytes, GEN4_PASSWORD_LEN);
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_GET_REVISION);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t rx_bytes = bit_buffer_get_size_bytes(instance->rx_buffer);
        if(rx_bytes != GEN4_REVISION_SIZE) {
            ret = Gen4PollerErrorProtocol;
            break;
        }
        bit_buffer_write_bytes(instance->rx_buffer, revision_result->data, GEN4_REVISION_SIZE);
    } while(false);

    return ret;
}

Gen4PollerError gen4_poller_set_config(
    Gen4Poller* instance,
    Gen4Password password,
    const Gen4Config* config,
    size_t config_size,
    bool fuse) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, password.bytes, GEN4_PASSWORD_LEN);
        uint8_t fuse_config = fuse ? GEN4_CMD_FUSE_CFG : GEN4_CMD_SET_CFG;
        bit_buffer_append_byte(instance->tx_buffer, fuse_config);
        bit_buffer_append_bytes(instance->tx_buffer, config->data_raw, config_size);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t response = bit_buffer_get_size_bytes(instance->rx_buffer);

        FURI_LOG_D(TAG, "Card response to set default config command: 0x%02X", response);

        if(response != GEN4_RESPONSE_SUCCESS) {
            ret = Gen4PollerErrorProtocol;
            break;
        }
    } while(false);

    return ret;
}

Gen4PollerError gen4_poller_write_block(
    Gen4Poller* instance,
    Gen4Password password,
    uint8_t block_num,
    const uint8_t* data) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, password.bytes, GEN4_PASSWORD_LEN);
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_WRITE);
        bit_buffer_append_byte(instance->tx_buffer, block_num);
        bit_buffer_append_bytes(instance->tx_buffer, data, GEN4_POLLER_BLOCK_SIZE);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t rx_bytes = bit_buffer_get_size_bytes(instance->rx_buffer);
        if(rx_bytes != 2) {
            ret = Gen4PollerErrorProtocol;
            break;
        }
    } while(false);

    return ret;
}

Gen4PollerError gen4_poller_set_ats(
    Gen4Poller* instance,
    Gen4Password password,
    const uint8_t* ats_data,
    uint8_t ats_len) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, password.bytes, GEN4_PASSWORD_LEN);
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_SET_ATS);
        bit_buffer_append_byte(instance->tx_buffer, ats_len);
        if(ats_len > 0) {
            bit_buffer_append_bytes(instance->tx_buffer, ats_data, ats_len);
        }

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t response = bit_buffer_get_size_bytes(instance->rx_buffer);

        FURI_LOG_D(TAG, "Card response: 0x%02X, ATS set, len: %d", response, ats_len);

        if(response != GEN4_RESPONSE_SUCCESS) {
            ret = Gen4PollerErrorProtocol;
            break;
        }

    } while(false);

    return ret;
}

Gen4PollerError gen4_poller_set_atqa_sak(
    Gen4Poller* instance,
    Gen4Password password,
    uint8_t atqa0,
    uint8_t atqa1,
    uint8_t sak) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, password.bytes, GEN4_PASSWORD_LEN);
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_SET_ATQA_SAK);
        bit_buffer_append_byte(instance->tx_buffer, atqa1);
        bit_buffer_append_byte(instance->tx_buffer, atqa0);
        bit_buffer_append_byte(instance->tx_buffer, sak);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t response = bit_buffer_get_size_bytes(instance->rx_buffer);

        FURI_LOG_D(
            TAG,
            "Card response: 0x%02X, ATQA set: %02X %02X, SAK: %02X",
            response,
            atqa0,
            atqa1,
            sak);

        if(response != GEN4_RESPONSE_SUCCESS) {
            ret = Gen4PollerErrorProtocol;
            break;
        }

    } while(false);

    return ret;
}

Gen4PollerError gen4_poller_set_ul_protocol(
    Gen4Poller* instance,
    Gen4Password password,
    Gen4Protocol protocol) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, password.bytes, GEN4_PASSWORD_LEN);
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_SET_UL_PROTOCOL);
        bit_buffer_append_byte(instance->tx_buffer, protocol);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t response = bit_buffer_get_size_bytes(instance->rx_buffer);

        FURI_LOG_D(TAG, "Card response: 0x%02X, UL protocol set: 0x%02X", response, protocol);

        if(response != GEN4_RESPONSE_SUCCESS) {
            ret = Gen4PollerErrorProtocol;
            break;
        }

    } while(false);

    return ret;
}

Gen4PollerError gen4_poller_set_ul_mode(
    Gen4Poller* instance,
    Gen4Password password,
    Gen4UltralightMode mode) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, password.bytes, GEN4_PASSWORD_LEN);
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_SET_UL_MODE);
        bit_buffer_append_byte(instance->tx_buffer, mode);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t response = bit_buffer_get_size_bytes(instance->rx_buffer);

        FURI_LOG_D(TAG, "Card response: 0x%02X, UL mode set: 0x%02X", response, mode);

        if(response != GEN4_RESPONSE_SUCCESS) {
            ret = Gen4PollerErrorProtocol;
            break;
        }

    } while(false);

    return ret;
}

Gen4PollerError gen4_poller_set_max_rw_block(
    Gen4Poller* instance,
    Gen4Password password,
    uint8_t max_block) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, password.bytes, GEN4_PASSWORD_LEN);
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_SET_MAX_RW_BLOCK);
        bit_buffer_append_byte(instance->tx_buffer, max_block);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t response = bit_buffer_get_size_bytes(instance->rx_buffer);

        FURI_LOG_D(TAG, "Card response: 0x%02X, Max R/W block set: 0x%02X", response, max_block);

        if(response != GEN4_RESPONSE_SUCCESS) {
            ret = Gen4PollerErrorProtocol;
            break;
        }

    } while(false);

    return ret;
}

Gen4PollerError gen4_poller_read_memory(
    Gen4Poller* instance,
    Gen4Password password,
    uint8_t block_num,
    uint8_t* data,
    uint8_t data_len) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, password.bytes, GEN4_PASSWORD_LEN);
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_READ);
        bit_buffer_append_byte(instance->tx_buffer, block_num);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t rx_bytes = bit_buffer_get_size_bytes(instance->rx_buffer);
        if(rx_bytes < data_len) {
            ret = Gen4PollerErrorProtocol;
            break;
        }
        bit_buffer_write_bytes(instance->rx_buffer, data, data_len);
    } while(false);

    return ret;
}

Gen4PollerError
    gen4_poller_mfu_read_block(Gen4Poller* instance, uint8_t block_num, uint8_t* data) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, 0x30);
        bit_buffer_append_byte(instance->tx_buffer, block_num);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t rx_bytes = bit_buffer_get_size_bytes(instance->rx_buffer);
        if(rx_bytes < 16) {
            ret = Gen4PollerErrorProtocol;
            break;
        }
        bit_buffer_write_bytes(instance->rx_buffer, data, 16);
    } while(false);

    return ret;
}

Gen4PollerError gen4_poller_mfu_write_block(
    Gen4Poller* instance,
    uint8_t block_num,
    const uint8_t* data) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, 0xA2);
        bit_buffer_append_byte(instance->tx_buffer, block_num);
        bit_buffer_append_bytes(instance->tx_buffer, data, 4);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t response = bit_buffer_get_size_bytes(instance->rx_buffer);

        FURI_LOG_D(
            TAG, "MFU write block %d response: 0x%02X", block_num, response);

        if(response != 0x0A) {
            ret = Gen4PollerErrorProtocol;
            break;
        }

    } while(false);

    return ret;
}

Gen4PollerError
    gen4_poller_mfu_auth_pwd(Gen4Poller* instance, const uint8_t* password) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, 0x1B);
        bit_buffer_append_bytes(instance->tx_buffer, password, 4);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t rx_bytes = bit_buffer_get_size_bytes(instance->rx_buffer);

        FURI_LOG_D(TAG, "MFU PWD AUTH response, %d bytes", rx_bytes);

    } while(false);

    return ret;
}

Gen4PollerError gen4_poller_change_password(
    Gen4Poller* instance,
    Gen4Password pwd_current,
    Gen4Password pwd_new) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, pwd_current.bytes, GEN4_PASSWORD_LEN);

        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_SET_PWD);
        bit_buffer_append_bytes(instance->tx_buffer, pwd_new.bytes, GEN4_PASSWORD_LEN);

        Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
            instance->iso3_poller, instance->tx_buffer, instance->rx_buffer, GEN4_POLLER_MAX_FWT);

        if(error != Iso14443_3aErrorNone) {
            ret = gen4_poller_process_error(error);
            break;
        }

        size_t response = bit_buffer_get_size_bytes(instance->rx_buffer);

        FURI_LOG_D(
            TAG,
            "Trying to change password from 0x%02X %02X %02X %02X to "
            "0x%02X %02X %02X %02X. Card response: 0x%02X",
            pwd_current.bytes[0],
            pwd_current.bytes[1],
            pwd_current.bytes[2],
            pwd_current.bytes[3],
            pwd_new.bytes[0],
            pwd_new.bytes[1],
            pwd_new.bytes[2],
            pwd_new.bytes[3],
            response);

        if(response != GEN4_RESPONSE_SUCCESS) {
            ret = Gen4PollerErrorProtocol;
            break;
        }
    } while(false);

    return ret;
}
