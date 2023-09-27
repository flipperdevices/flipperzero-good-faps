#pragma once

#include <stdint.h>
#include <stdbool.h>

#define RFAL_PICOPASS_UID_LEN 8
#define RFAL_PICOPASS_BLOCK_LEN 8

#define furi_hal_nfc_ll_ms2fc(x) (x)

#define FURI_HAL_NFC_LL_TXRX_FLAGS_CRC_TX_MANUAL 0
#define FURI_HAL_NFC_LL_TXRX_FLAGS_AGC_ON 0
#define FURI_HAL_NFC_LL_TXRX_FLAGS_PAR_RX_REMV 0
#define FURI_HAL_NFC_LL_TXRX_FLAGS_CRC_RX_KEEP 0

typedef enum {
    FuriHalNfcReturnOk = 0, /*!< no error occurred */
    FuriHalNfcReturnNomem = 1, /*!< not enough memory to perform the requested operation */
    FuriHalNfcReturnBusy = 2, /*!< device or resource busy */
    FuriHalNfcReturnIo = 3, /*!< generic IO error */
    FuriHalNfcReturnTimeout = 4, /*!< error due to timeout */
    FuriHalNfcReturnRequest =
        5, /*!< invalid request or requested function can't be executed at the moment */
    FuriHalNfcReturnNomsg = 6, /*!< No message of desired type */
    FuriHalNfcReturnParam = 7, /*!< Parameter error */
    FuriHalNfcReturnSystem = 8, /*!< System error */
    FuriHalNfcReturnFraming = 9, /*!< Framing error */
    FuriHalNfcReturnOverrun = 10, /*!< lost one or more received bytes */
    FuriHalNfcReturnProto = 11, /*!< protocol error */
    FuriHalNfcReturnInternal = 12, /*!< Internal Error */
    FuriHalNfcReturnAgain = 13, /*!< Call again */
    FuriHalNfcReturnMemCorrupt = 14, /*!< memory corruption */
    FuriHalNfcReturnNotImplemented = 15, /*!< not implemented */
    FuriHalNfcReturnPcCorrupt =
        16, /*!< Program Counter has been manipulated or spike/noise trigger illegal operation */
    FuriHalNfcReturnSend = 17, /*!< error sending*/
    FuriHalNfcReturnIgnore = 18, /*!< indicates error detected but to be ignored */
    FuriHalNfcReturnSemantic = 19, /*!< indicates error in state machine (unexpected cmd) */
    FuriHalNfcReturnSyntax = 20, /*!< indicates error in state machine (unknown cmd) */
    FuriHalNfcReturnCrc = 21, /*!< crc error */
    FuriHalNfcReturnNotfound = 22, /*!< transponder not found */
    FuriHalNfcReturnNotunique =
        23, /*!< transponder not unique - more than one transponder in field */
    FuriHalNfcReturnNotsupp = 24, /*!< requested operation not supported */
    FuriHalNfcReturnWrite = 25, /*!< write error */
    FuriHalNfcReturnFifo = 26, /*!< fifo over or underflow error */
    FuriHalNfcReturnPar = 27, /*!< parity error */
    FuriHalNfcReturnDone = 28, /*!< transfer has already finished */
    FuriHalNfcReturnRfCollision =
        29, /*!< collision error (Bit Collision or during RF Collision avoidance ) */
    FuriHalNfcReturnHwOverrun = 30, /*!< lost one or more received bytes */
    FuriHalNfcReturnReleaseReq = 31, /*!< device requested release */
    FuriHalNfcReturnSleepReq = 32, /*!< device requested sleep */
    FuriHalNfcReturnWrongState = 33, /*!< incorrent state for requested operation */
    FuriHalNfcReturnMaxReruns = 34, /*!< blocking procedure reached maximum runs */
    FuriHalNfcReturnDisabled = 35, /*!< operation aborted due to disabled configuration */
    FuriHalNfcReturnHwMismatch = 36, /*!< expected hw do not match  */
    FuriHalNfcReturnLinkLoss =
        37, /*!< Other device's field didn't behave as expected: turned off by Initiator in Passive mode, or AP2P did not turn on field */
    FuriHalNfcReturnInvalidHandle = 38, /*!< invalid or not initalized device handle */
    FuriHalNfcReturnIncompleteByte = 40, /*!< Incomplete byte rcvd         */
    FuriHalNfcReturnIncompleteByte01 = 41, /*!< Incomplete byte rcvd - 1 bit */
    FuriHalNfcReturnIncompleteByte02 = 42, /*!< Incomplete byte rcvd - 2 bit */
    FuriHalNfcReturnIncompleteByte03 = 43, /*!< Incomplete byte rcvd - 3 bit */
    FuriHalNfcReturnIncompleteByte04 = 44, /*!< Incomplete byte rcvd - 4 bit */
    FuriHalNfcReturnIncompleteByte05 = 45, /*!< Incomplete byte rcvd - 5 bit */
    FuriHalNfcReturnIncompleteByte06 = 46, /*!< Incomplete byte rcvd - 6 bit */
    FuriHalNfcReturnIncompleteByte07 = 47, /*!< Incomplete byte rcvd - 7 bit */
} FuriHalNfcReturn;

#define ERR_NONE FuriHalNfcReturnOk
#define ERR_RF_COLLISION FuriHalNfcReturnRfCollision
#define ERR_PARAM FuriHalNfcReturnParam
#define ERR_REQUEST FuriHalNfcReturnRequest

typedef FuriHalNfcReturn ReturnCode;

typedef enum {
    FuriHalNfcModeLegacyNone = 0, /*!< No mode selected/defined */
    FuriHalNfcModeLegacyPollNfca = 1, /*!< Mode to perform as NFCA (ISO14443A) Poller (PCD) */
    FuriHalNfcModeLegacyPollNfcaT1t = 2, /*!< Mode to perform as NFCA T1T (Topaz) Poller (PCD) */
    FuriHalNfcModeLegacyPollNfcb = 3, /*!< Mode to perform as NFCB (ISO14443B) Poller (PCD) */
    FuriHalNfcModeLegacyPollBPrime = 4, /*!< Mode to perform as B' Calypso (Innovatron) (PCD) */
    FuriHalNfcModeLegacyPollBCts = 5, /*!< Mode to perform as CTS Poller (PCD) */
    FuriHalNfcModeLegacyPollNfcf = 6, /*!< Mode to perform as NFCF (FeliCa) Poller (PCD) */
    FuriHalNfcModeLegacyPollNfcv = 7, /*!< Mode to perform as NFCV (ISO15963) Poller (PCD) */
    FuriHalNfcModeLegacyPollPicopass = 8, /*!< Mode to perform as PicoPass / iClass Poller (PCD) */
    FuriHalNfcModeLegacyPollActiveP2p =
        9, /*!< Mode to perform as Active P2P (ISO18092) Initiator  */
    FuriHalNfcModeLegacyListenNfca =
        10, /*!< Mode to perform as NFCA (ISO14443A) Listener (PICC) */
    FuriHalNfcModeLegacyListenNfcb =
        11, /*!< Mode to perform as NFCA (ISO14443B) Listener (PICC) */
    FuriHalNfcModeLegacyListenNfcf = 12, /*!< Mode to perform as NFCA (ISO15963) Listener (PICC) */
    FuriHalNfcModeLegacyListenActiveP2p =
        13 /*!< Mode to perform as Active P2P (ISO18092) Target  */
} FuriHalNfcModeLegacy;

typedef enum {
    FuriHalNfcBitrate106 = 0, /*!< Bit Rate 106 kbit/s (fc/128) */
    FuriHalNfcBitrate212 = 1, /*!< Bit Rate 212 kbit/s (fc/64) */
    FuriHalNfcBitrate424 = 2, /*!< Bit Rate 424 kbit/s (fc/32) */
    FuriHalNfcBitrate848 = 3, /*!< Bit Rate 848 kbit/s (fc/16) */
    FuriHalNfcBitrate1695 = 4, /*!< Bit Rate 1695 kbit/s (fc/8) */
    FuriHalNfcBitrate3390 = 5, /*!< Bit Rate 3390 kbit/s (fc/4) */
    FuriHalNfcBitrate6780 = 6, /*!< Bit Rate 6780 kbit/s (fc/2) */
    FuriHalNfcBitrate13560 = 7, /*!< Bit Rate 13560 kbit/s (fc) */
    FuriHalNfcBitrate52p97 = 0xEB, /*!< Bit Rate 52.97 kbit/s (fc/256) Fast Mode VICC->VCD */
    FuriHalNfcBitrate26p48 =
        0xEC, /*!< Bit Rate 26,48 kbit/s (fc/512) NFCV VICC->VCD & VCD->VICC 1of4 */
    FuriHalNfcBitrate1p66 = 0xED, /*!< Bit Rate 1,66 kbit/s (fc/8192) NFCV VCD->VICC 1of256 */
    FuriHalNfcBitrateKeep = 0xFF /*!< Value indicating to keep the same previous bit rate */
} FuriHalNfcBitrate;

#define FURI_HAL_NFC_LL_GT_NFCA furi_hal_nfc_ll_ms2fc(5U) /*!< GTA  Digital 2.0  6.10.4.1 & B.2 */
#define FURI_HAL_NFC_LL_GT_NFCB furi_hal_nfc_ll_ms2fc(5U) /*!< GTB  Digital 2.0  7.9.4.1  & B.3 */
#define FURI_HAL_NFC_LL_GT_NFCF furi_hal_nfc_ll_ms2fc(20U) /*!< GTF  Digital 2.0  8.7.4.1  & B.4 */
#define FURI_HAL_NFC_LL_GT_NFCV furi_hal_nfc_ll_ms2fc(5U) /*!< GTV  Digital 2.0  9.7.5.1  & B.5 */
#define FURI_HAL_NFC_LL_GT_PICOPASS furi_hal_nfc_ll_ms2fc(1U) /*!< GT Picopass */
#define FURI_HAL_NFC_LL_GT_AP2P furi_hal_nfc_ll_ms2fc(5U) /*!< TIRFG  Ecma 340  11.1.1 */
#define FURI_HAL_NFC_LL_GT_AP2P_ADJUSTED \
    furi_hal_nfc_ll_ms2fc(               \
        5U +                             \
        25U) /*!< Adjusted GT for greater interoperability (Sony XPERIA P, Nokia N9, Huawei P2) */

typedef enum {
    FuriHalNfcErrorHandlingNone = 0, /*!< No special error handling will be performed */
    FuriHalNfcErrorHandlingNfc = 1, /*!< Error handling set to perform as NFC compliant device */
    FuriHalNfcErrorHandlingEmvco =
        2 /*!< Error handling set to perform as EMVCo compliant device */
} FuriHalNfcErrorHandling;

/* RFAL Frame Delay Time (FDT) Listen default values   */
#define FURI_HAL_NFC_LL_FDT_LISTEN_NFCA_POLLER \
    1172U /*!< FDTA,LISTEN,MIN (n=9) Last bit: Logic "1" - tnn,min/2 Digital 1.1  6.10 ;  EMV CCP Spec Book D v2.01  4.8.1.3 */
#define FURI_HAL_NFC_LL_FDT_LISTEN_NFCB_POLLER \
    1008U /*!< TR0B,MIN         Digital 1.1  7.1.3 & A.3  ; EMV CCP Spec Book D v2.01  4.8.1.3 & Table A.5 */
#define FURI_HAL_NFC_LL_FDT_LISTEN_NFCF_POLLER \
    2672U /*!< TR0F,LISTEN,MIN  Digital 1.1  8.7.1.1 & A.4 */
#define FURI_HAL_NFC_LL_FDT_LISTEN_NFCV_POLLER \
    4310U /*!< FDTV,LISTEN,MIN  t1 min       Digital 2.1  B.5  ;  ISO15693-3 2009  9.1 */
#define FURI_HAL_NFC_LL_FDT_LISTEN_PICOPASS_POLLER \
    3400U /*!< ISO15693 t1 min - observed adjustment */
#define FURI_HAL_NFC_LL_FDT_LISTEN_AP2P_POLLER \
    64U /*!< FDT AP2P No actual FDTListen is required as fields switch and collision avoidance */
#define FURI_HAL_NFC_LL_FDT_LISTEN_NFCA_LISTENER 1172U /*!< FDTA,LISTEN,MIN  Digital 1.1  6.10 */
#define FURI_HAL_NFC_LL_FDT_LISTEN_NFCB_LISTENER \
    1024U /*!< TR0B,MIN         Digital 1.1  7.1.3 & A.3  ;  EMV CCP Spec Book D v2.01  4.8.1.3 & Table A.5 */
#define FURI_HAL_NFC_LL_FDT_LISTEN_NFCF_LISTENER \
    2688U /*!< TR0F,LISTEN,MIN  Digital 2.1  8.7.1.1 & B.4 */
#define FURI_HAL_NFC_LL_FDT_LISTEN_AP2P_LISTENER \
    64U /*!< FDT AP2P No actual FDTListen exists as fields switch and collision avoidance */

/*  RFAL Frame Delay Time (FDT) Poll default values    */
#define FURI_HAL_NFC_LL_FDT_POLL_NFCA_POLLER \
    6780U /*!< FDTA,POLL,MIN   Digital 1.1  6.10.3.1 & A.2 */
#define FURI_HAL_NFC_LL_FDT_POLL_NFCA_T1T_POLLER \
    384U /*!< RRDDT1T,MIN,B1  Digital 1.1  10.7.1 & A.5 */
#define FURI_HAL_NFC_LL_FDT_POLL_NFCB_POLLER \
    6780U /*!< FDTB,POLL,MIN = TR2B,MIN,DEFAULT Digital 1.1 7.9.3 & A.3  ;  EMVCo 3.0 FDTB,PCD,MIN  Table A.5 */
#define FURI_HAL_NFC_LL_FDT_POLL_NFCF_POLLER 6800U /*!< FDTF,POLL,MIN   Digital 2.1  8.7.3 & B.4 */
#define FURI_HAL_NFC_LL_FDT_POLL_NFCV_POLLER 4192U /*!< FDTV,POLL  Digital 2.1  9.7.3.1  & B.5 */
#define FURI_HAL_NFC_LL_FDT_POLL_PICOPASS_POLLER 1790U /*!< FDT Max */
#define FURI_HAL_NFC_LL_FDT_POLL_AP2P_POLLER \
    0U /*!< FDT AP2P No actual FDTPoll exists as fields switch and collision avoidance */

FuriHalNfcReturn furi_hal_nfc_ll_set_mode(
    FuriHalNfcModeLegacy mode,
    FuriHalNfcBitrate txBR,
    FuriHalNfcBitrate rxBR);

void furi_hal_nfc_ll_set_guard_time(uint32_t cycles);

void furi_hal_nfc_ll_set_error_handling(FuriHalNfcErrorHandling eHandling);

void furi_hal_nfc_ll_set_fdt_listen(uint32_t cycles);

void furi_hal_nfc_ll_set_fdt_poll(uint32_t FDTPoll);

void furi_hal_nfc_ll_txrx_on();

void furi_hal_nfc_ll_txrx_off();

void furi_hal_nfc_exit_sleep();

void furi_hal_nfc_start_sleep();

FuriHalNfcReturn furi_hal_nfc_ll_txrx(
    uint8_t* txBuf,
    uint16_t txBufLen,
    uint8_t* rxBuf,
    uint16_t rxBufLen,
    uint16_t* actLen,
    uint32_t flags,
    uint32_t fwt);

FuriHalNfcReturn furi_hal_nfc_ll_txrx_bits(
    uint8_t* txBuf,
    uint16_t txBufLen,
    uint8_t* rxBuf,
    uint16_t rxBufLen,
    uint16_t* actLen,
    uint32_t flags,
    uint32_t fwt);

void furi_hal_nfc_ll_poll();

enum {
    // PicoPass command bytes:
    // Low nibble used for command
    // High nibble used for options and checksum (MSB)
    // The only option we care about in 15693 mode is the key
    // which is only used by READCHECK, so for simplicity we
    // don't bother breaking down the command and flags into parts

    // READ: ADDRESS(1) CRC16(2) -> DATA(8) CRC16(2)
    // IDENTIFY: No args -> ASNB(8) CRC16(2)
    RFAL_PICOPASS_CMD_READ_OR_IDENTIFY = 0x0C,
    // ADDRESS(1) CRC16(2) -> DATA(32) CRC16(2)
    RFAL_PICOPASS_CMD_READ4 = 0x06,
    // ADDRESS(1) DATA(8) SIGN(4)|CRC16(2) -> DATA(8) CRC16(2)
    RFAL_PICOPASS_CMD_UPDATE = 0x87,
    // ADDRESS(1) -> DATA(8)
    RFAL_PICOPASS_CMD_READCHECK_KD = 0x88,
    // ADDRESS(1) -> DATA(8)
    RFAL_PICOPASS_CMD_READCHECK_KC = 0x18,
    // CHALLENGE(4) READERSIGNATURE(4) -> CHIPRESPONSE(4)
    RFAL_PICOPASS_CMD_CHECK = 0x05,
    // No args -> SOF
    RFAL_PICOPASS_CMD_ACTALL = 0x0A,
    // No args -> SOF
    RFAL_PICOPASS_CMD_ACT = 0x8E,
    // ASNB(8)|SERIALNB(8) -> SERIALNB(8) CRC16(2)
    RFAL_PICOPASS_CMD_SELECT = 0x81,
    // No args -> SERIALNB(8) CRC16(2)
    RFAL_PICOPASS_CMD_DETECT = 0x0F,
    // No args -> SOF
    RFAL_PICOPASS_CMD_HALT = 0x00,
    // PAGE(1) CRC16(2) -> BLOCK1(8) CRC16(2)
    RFAL_PICOPASS_CMD_PAGESEL = 0x84,
};

typedef struct {
    uint8_t CSN[RFAL_PICOPASS_UID_LEN]; // Anti-collision CSN
    uint8_t crc[2];
} rfalPicoPassIdentifyRes;

typedef struct {
    uint8_t CSN[RFAL_PICOPASS_UID_LEN]; // Real CSN
    uint8_t crc[2];
} rfalPicoPassSelectRes;

typedef struct {
    uint8_t CCNR[8];
} rfalPicoPassReadCheckRes;

typedef struct {
    uint8_t mac[4];
} rfalPicoPassCheckRes;

typedef struct {
    uint8_t data[RFAL_PICOPASS_BLOCK_LEN];
    uint8_t crc[2];
} rfalPicoPassReadBlockRes;

uint16_t rfalPicoPassCalculateCcitt(uint16_t preloadValue, const uint8_t* buf, uint16_t length);

FuriHalNfcReturn rfalPicoPassPollerInitialize(void);
FuriHalNfcReturn rfalPicoPassPollerCheckPresence(void);
FuriHalNfcReturn rfalPicoPassPollerIdentify(rfalPicoPassIdentifyRes* idRes);
FuriHalNfcReturn rfalPicoPassPollerSelect(uint8_t* csn, rfalPicoPassSelectRes* selRes);
FuriHalNfcReturn rfalPicoPassPollerReadCheck(rfalPicoPassReadCheckRes* rcRes);
FuriHalNfcReturn rfalPicoPassPollerCheck(uint8_t* mac, rfalPicoPassCheckRes* chkRes);
FuriHalNfcReturn rfalPicoPassPollerReadBlock(uint8_t blockNum, rfalPicoPassReadBlockRes* readRes);
FuriHalNfcReturn rfalPicoPassPollerWriteBlock(uint8_t blockNum, uint8_t data[8], uint8_t mac[4]);
