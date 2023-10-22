#include "gen4_poller_i.h"

#include <nfc/protocols/iso14443_3a/iso14443_3a_poller.h>
#include <nfc/helpers/nfc_util.h>

#define GEN4_CMD_PREFIX (0xCF)

#define GEN4_CMD_GET_CFG (0xC6)
#define GEN4_CMD_WRITE (0xCD)
#define GEN4_CMD_READ (0xCE)
#define GEN4_CMD_SET_CFG (0xF0)
#define GEN4_CMD_FUSE_CFG (0xF1)
#define GEN4_CMD_SET_PWD (0xFE)

static const uint8_t gen4_poller_default_config[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
                                                     0x00, 0x09, 0x78, 0x00, 0x91, 0x02, 0xDA,
                                                     0xBC, 0x19, 0x10, 0x10, 0x11, 0x12, 0x13,
                                                     0x14, 0x15, 0x16, 0x04, 0x00, 0x08, 0x00};

static Gen4PollerError gen4_poller_process_error(Iso14443_3aError error) {
    Gen4PollerError ret = Gen4PollerErrorNone;

    if(error == Iso14443_3aErrorNone) {
        ret = Gen4PollerErrorNone;
    } else {
        ret = Gen4PollerErrorTimeout;
    }

    return ret;
}

Gen4PollerError gen4_poller_set_config(Gen4Poller* instance) {
    Gen4PollerError ret = Gen4PollerErrorNone;
    bit_buffer_reset(instance->tx_buffer);

    do {
        uint8_t password_arr[4] = {};
        nfc_util_num2bytes(instance->password, COUNT_OF(password_arr), password_arr);
        bit_buffer_append_byte(instance->tx_buffer, GEN4_CMD_PREFIX);
        bit_buffer_append_bytes(instance->tx_buffer, password_arr, COUNT_OF(password_arr));
        bit_buffer_append_bytes(
            instance->tx_buffer, gen4_poller_default_config, COUNT_OF(gen4_poller_default_config));

        Iso14443_3aError error = iso14443_3a_poller_txrx(
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
