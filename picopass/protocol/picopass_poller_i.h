#pragma once

#include "picopass_poller.h"
#include "picopass_protocol.h"

#include <nfc/helpers/iso13239_crc.h>

#define PICOPASS_POLLER_BUFFER_SIZE (255)
#define PICOPASS_CRC_SIZE (2)

typedef enum {
    PicopassPollerSessionStateIdle,
    PicopassPollerSessionStateActive,
    PicopassPollerSessionStateStopRequest,
} PicopassPollerSessionState;

typedef enum {
    PicopassPollerStateDetect,
    PicopassPollerStatePreAuth,
    PicopassPollerStateCheckSecurity,
    PicopassPollerStateAuth,
    PicopassPollerStateSuccess,
    PicopassPollerStateFail,

    PicopassPollerStateNum,
} PicopassPollerState;

struct PicopassPoller {
    Nfc* nfc;
    PicopassPollerSessionState session_state;
    PicopassPollerState state;

    PicopassColResSerialNum col_res_serial_num;
    PicopassSerialNum serial_num;
    PicopassData data;

    BitBuffer* tx_buffer;
    BitBuffer* rx_buffer;
    BitBuffer* tmp_buffer;

    PicopassPollerEvent event;
    PicopassPollerCallback callback;
    void* context;
};

PicopassError picopass_poller_actall(PicopassPoller* instance);

PicopassError
    picopass_poller_identify(PicopassPoller* instance, PicopassColResSerialNum* col_res_serial_num);

PicopassError picopass_poller_select(
    PicopassPoller* instance,
    PicopassColResSerialNum* col_res_serial_num,
    PicopassSerialNum* serial_num);

PicopassError
    picopass_poller_read_block(PicopassPoller* instance, uint8_t block_num, PicopassBlock* block);

PicopassError
    picopass_poller_read_check(PicopassPoller* instance, PicopassReadCheckResp* read_check_resp);

PicopassError picopass_poller_check(
    PicopassPoller* instance,
    PicopassMac* mac,
    PicopassCheckResp* check_resp);
