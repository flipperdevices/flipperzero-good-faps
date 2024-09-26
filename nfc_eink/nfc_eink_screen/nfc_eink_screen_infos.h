#pragma once
#include <furi.h>
#include <m-array.h>

typedef enum {
    NfcEinkScreenSizeUnknown,
    NfcEinkScreenSize2n13inch,
    NfcEinkScreenSize2n9inch,
    NfcEinkScreenSize1n54inch,
    NfcEinkScreenSize3n71inch,
    NfcEinkScreenSize2n7inch,
    NfcEinkScreenSize4n2inch,
    NfcEinkScreenSize7n5inch,

    NfcEinkScreenSizeNum
} NfcEinkScreenSize;

typedef enum {
    NfcEinkManufacturerWaveshare,
    NfcEinkManufacturerGoodisplay,

    NfcEinkManufacturerNum,
    NfcEinkManufacturerUnknown
} NfcEinkManufacturer;

/// TODO: Align all screens here by manufacturers before release
typedef enum {
    NfcEinkScreenTypeUnknown,
    NfcEinkScreenTypeGoodisplay2n13inch,
    NfcEinkScreenTypeGoodisplay2n9inch,

    //NfcEinkTypeGoodisplay4n2inch,

    NfcEinkScreenTypeWaveshare2n13inch,
    NfcEinkScreenTypeWaveshare2n9inch,
    NfcEinkScreenTypeGoodisplay1n54inch,
    NfcEinkScreenTypeGoodisplay3n71inch,

    NfcEinkScreenTypeWaveshare2n7inch,
    NfcEinkScreenTypeWaveshare4n2inch,
    NfcEinkScreenTypeWaveshare7n5inch,

    NfcEinkScreenTypeNum
} NfcEinkScreenType;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t data_block_size;
    NfcEinkScreenType screen_type;
    NfcEinkScreenSize screen_size;
    NfcEinkManufacturer screen_manufacturer;
    const char* name;
} NfcEinkScreenInfo;

#define M_ARRAY_SIZE (sizeof(NfcEinkScreenInfo*) * NfcEinkScreenTypeNum)
#define M_INIT(a)    ((a) = malloc(M_ARRAY_SIZE))

#define M_INIT_SET(new, old)                          \
    do {                                              \
        M_INIT(new);                                  \
        memcpy((void*)new, (void*)old, M_ARRAY_SIZE); \
        free((void*)old);                             \
    } while(false)

//#define M_SET(a, b) (M_INIT(a); memcpy(a, b, M_ARRAY_SIZE))
#define M_CLEAR(a) (free((void*)a))

#define M_DESCRIPTOR_ARRAY_OPLIST \
    (INIT(M_INIT), INIT_SET(M_INIT_SET), CLEAR(M_CLEAR), TYPE(const NfcEinkScreenInfo*))

ARRAY_DEF(EinkScreenInfoArray, const NfcEinkScreenInfo*, M_DESCRIPTOR_ARRAY_OPLIST);

const NfcEinkScreenInfo* nfc_eink_descriptor_get_by_type(NfcEinkScreenType type);

uint8_t nfc_eink_descriptor_get_all_usable(EinkScreenInfoArray_t result);

uint8_t nfc_eink_descriptor_filter_by_manufacturer(
    EinkScreenInfoArray_t result,
    NfcEinkManufacturer manufacturer);

uint8_t nfc_eink_descriptor_filter_by_screen_size(
    EinkScreenInfoArray_t result,
    NfcEinkScreenSize screen_size);

uint8_t nfc_eink_descriptor_filter_by_screen_type(
    EinkScreenInfoArray_t result,
    NfcEinkScreenType screen_type);
