#include "vauno_en8822c.h"

#define TAG "WSProtocolVaunoEN8822C"

/*
 * Help
 * https://github.com/merbanan/rtl_433/blob/master/src/devices/vauno_en8822c.c
 *
 *  Frame structure (42 bits):
 *
 *      Byte:      0        1        2        3        4
 *      Nibble:    1   2    3   4    5   6    7   8    9   10   11
 *      Type:      IIIIIIII B?CCTTTT TTTTTTTT HHHHHHHF FFFBXXXX XX
 *
 * - I: Random device ID
 * - C: Channel (1-3)
 * - T: Temperature (Little-endian)
 * - H: Humidity (Little-endian)
 * - F: Flags (unknown)
 * - B: Battery (1=low voltage ~<2.5V)
 * - X: Checksum (6 bit nibble sum)
 */

static const SubGhzBlockConst ws_protocol_vauno_en8822c_const = {
    .te_short = 500,
    .te_long = 2000,
    .te_delta = 150,
    .min_count_bit_for_found = 42,
};

struct WSProtocolDecoderVaunoEN8822C {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    WSBlockGeneric generic;

    uint16_t header_count;
};

struct WSProtocolEncoderVaunoEN8822C {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    WSBlockGeneric generic;
};

typedef enum {
    VaunoEN8822CDecoderStepReset = 0,
    VaunoEN8822CDecoderStepCheckPreambule,
    VaunoEN8822CDecoderStepSaveDuration,
    VaunoEN8822CDecoderStepCheckDuration,
} VaunoEN8822CDecoderStep;

const SubGhzProtocolDecoder ws_protocol_vauno_en8822c_decoder = {
    .alloc = ws_protocol_decoder_vauno_en8822c_alloc,
    .free = ws_protocol_decoder_vauno_en8822c_free,

    .feed = ws_protocol_decoder_vauno_en8822c_feed,
    .reset = ws_protocol_decoder_vauno_en8822c_reset,

    .get_hash_data = ws_protocol_decoder_vauno_en8822c_get_hash_data,
    .serialize = ws_protocol_decoder_vauno_en8822c_serialize,
    .deserialize = ws_protocol_decoder_vauno_en8822c_deserialize,
    .get_string = ws_protocol_decoder_vauno_en8822c_get_string,
};

const SubGhzProtocolEncoder ws_protocol_vauno_en8822c_encoder = {
    .alloc = NULL,
    .free = NULL,

    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};

const SubGhzProtocol ws_protocol_vauno_en8822c = {
    .name = WS_PROTOCOL_VAUNO_EN8822C_NAME,
    .type = SubGhzProtocolWeatherStation,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_315 | SubGhzProtocolFlag_868 |
            SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable,

    .decoder = &ws_protocol_vauno_en8822c_decoder,
    .encoder = &ws_protocol_vauno_en8822c_encoder,
};

void* ws_protocol_decoder_vauno_en8822c_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    WSProtocolDecoderVaunoEN8822C* instance = malloc(sizeof(WSProtocolDecoderVaunoEN8822C));
    instance->base.protocol = &ws_protocol_vauno_en8822c;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void ws_protocol_decoder_vauno_en8822c_free(void* context) {
    furi_assert(context);
    WSProtocolDecoderVaunoEN8822C* instance = context;
    free(instance);
}

void ws_protocol_decoder_vauno_en8822c_reset(void* context) {
    furi_assert(context);
    WSProtocolDecoderVaunoEN8822C* instance = context;
    instance->decoder.parser_step = VaunoEN8822CDecoderStepReset;
}

static bool ws_protocol_vauno_en8822c_check_crc(WSProtocolDecoderVaunoEN8822C* instance) {
    uint8_t msg[] = {
        instance->decoder.decode_data >> 32,
        instance->decoder.decode_data >> 24,
        instance->decoder.decode_data >> 16,
        instance->decoder.decode_data >> 8,
        instance->decoder.decode_data};

    uint8_t crc =
        subghz_protocol_blocks_crc4(msg, 4, 0x03, 0); // CRC-4 poly 0x3 init 0x0 xor last 4 bits
    crc ^= msg[4] >> 4; // last nibble is only XORed
    return (crc == (msg[4] & 0x0F));
}

/**
 * Analysis of received data
 * @param instance Pointer to a WSBlockGeneric* instance
 */
static void ws_protocol_vauno_en8822c_remote_controller(WSBlockGeneric* instance) {
    instance->id = instance->data >> 32;
    if((instance->data >> 30) & 0x3) {
        instance->battery_low = 0;
    } else {
        instance->battery_low = 1;
    }
    instance->channel = ((instance->data >> 28) & 0x3) + 1;
    instance->btn = WS_NO_BTN;
    uint16_t temp_raw = ((instance->data >> 16) & 0x0f) << 8 |
                        ((instance->data >> 20) & 0x0f) << 4 | ((instance->data >> 24) & 0x0f);
    instance->temp = locale_fahrenheit_to_celsius(((float)temp_raw - 900.0f) / 10.0f);
    instance->humidity = ((instance->data >> 8) & 0x0f) << 4 | ((instance->data >> 12) & 0x0f);
}

void ws_protocol_decoder_vauno_en8822c_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    WSProtocolDecoderVaunoEN8822C* instance = context;

    switch(instance->decoder.parser_step) {
    case VaunoEN8822CDecoderStepReset:
        if((level) && (DURATION_DIFF(duration, ws_protocol_vauno_en8822c_const.te_short) <
                       ws_protocol_vauno_en8822c_const.te_delta)) {
            instance->decoder.parser_step = VaunoEN8822CDecoderStepCheckPreambule;
            instance->decoder.te_last = duration;
            instance->header_count = 0;
        }
        break;

    case VaunoEN8822CDecoderStepCheckPreambule:
        if(level) {
            instance->decoder.te_last = duration;
        } else {
            if((DURATION_DIFF(instance->decoder.te_last, ws_protocol_vauno_en8822c_const.te_short) <
                ws_protocol_vauno_en8822c_const.te_delta) &&
               (DURATION_DIFF(duration, ws_protocol_vauno_en8822c_const.te_long * 4) <
                ws_protocol_vauno_en8822c_const.te_delta * 4)) {
                //Found preambule
                instance->header_count++;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, ws_protocol_vauno_en8822c_const.te_short) <
                 ws_protocol_vauno_en8822c_const.te_delta) &&
                (duration < (ws_protocol_vauno_en8822c_const.te_long * 2 +
                             ws_protocol_vauno_en8822c_const.te_delta * 2))) {
                //Found syncPrefix
                if(instance->header_count > 0) {
                    instance->decoder.parser_step = VaunoEN8822CDecoderStepSaveDuration;
                    instance->decoder.decode_data = 0;
                    instance->decoder.decode_count_bit = 0;
                    if((DURATION_DIFF(
                            instance->decoder.te_last, ws_protocol_vauno_en8822c_const.te_short) <
                        ws_protocol_vauno_en8822c_const.te_delta) &&
                       (DURATION_DIFF(duration, ws_protocol_vauno_en8822c_const.te_long) <
                        ws_protocol_vauno_en8822c_const.te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                        instance->decoder.parser_step = VaunoEN8822CDecoderStepSaveDuration;
                    } else if(
                        (DURATION_DIFF(
                             instance->decoder.te_last, ws_protocol_vauno_en8822c_const.te_short) <
                         ws_protocol_vauno_en8822c_const.te_delta) &&
                        (DURATION_DIFF(duration, ws_protocol_vauno_en8822c_const.te_long * 2) <
                         ws_protocol_vauno_en8822c_const.te_delta * 4)) {
                        subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                        instance->decoder.parser_step = VaunoEN8822CDecoderStepSaveDuration;
                    } else {
                        instance->decoder.parser_step = VaunoEN8822CDecoderStepReset;
                    }
                }
            } else {
                instance->decoder.parser_step = VaunoEN8822CDecoderStepReset;
            }
        }
        break;

    case VaunoEN8822CDecoderStepSaveDuration:
        if(level) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = VaunoEN8822CDecoderStepCheckDuration;
        } else {
            instance->decoder.parser_step = VaunoEN8822CDecoderStepReset;
        }
        break;

    case VaunoEN8822CDecoderStepCheckDuration:
        if(!level) {
            if(DURATION_DIFF(duration, ws_protocol_vauno_en8822c_const.te_long * 4) <
               ws_protocol_vauno_en8822c_const.te_delta * 4) {
                //Found syncPostfix
                if((instance->decoder.decode_count_bit ==
                    ws_protocol_vauno_en8822c_const.min_count_bit_for_found) &&
                   ws_protocol_vauno_en8822c_check_crc(instance)) {
                    instance->generic.data = instance->decoder.decode_data;
                    instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                    ws_protocol_vauno_en8822c_remote_controller(&instance->generic);
                    if(instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                instance->decoder.parser_step = VaunoEN8822CDecoderStepReset;
                break;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, ws_protocol_vauno_en8822c_const.te_short) <
                 ws_protocol_vauno_en8822c_const.te_delta) &&
                (DURATION_DIFF(duration, ws_protocol_vauno_en8822c_const.te_long) <
                 ws_protocol_vauno_en8822c_const.te_delta * 2)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                instance->decoder.parser_step = VaunoEN8822CDecoderStepSaveDuration;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, ws_protocol_vauno_en8822c_const.te_short) <
                 ws_protocol_vauno_en8822c_const.te_delta) &&
                (DURATION_DIFF(duration, ws_protocol_vauno_en8822c_const.te_long * 2) <
                 ws_protocol_vauno_en8822c_const.te_delta * 4)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                instance->decoder.parser_step = VaunoEN8822CDecoderStepSaveDuration;
            } else {
                instance->decoder.parser_step = VaunoEN8822CDecoderStepReset;
            }
        } else {
            instance->decoder.parser_step = VaunoEN8822CDecoderStepReset;
        }
        break;
    }
}

uint8_t ws_protocol_decoder_vauno_en8822c_get_hash_data(void* context) {
    furi_assert(context);
    WSProtocolDecoderVaunoEN8822C* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus ws_protocol_decoder_vauno_en8822c_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    WSProtocolDecoderVaunoEN8822C* instance = context;
    return ws_block_generic_serialize(&instance->generic, flipper_format, preset);
}

SubGhzProtocolStatus
    ws_protocol_decoder_vauno_en8822c_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    WSProtocolDecoderVaunoEN8822C* instance = context;
    return ws_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, ws_protocol_vauno_en8822c_const.min_count_bit_for_found);
}

void ws_protocol_decoder_vauno_en8822c_get_string(void* context, FuriString* output) {
    furi_assert(context);
    WSProtocolDecoderVaunoEN8822C* instance = context;
    furi_string_printf(
        output,
        "%s %dbit\r\n"
        "Key:0x%lX%08lX\r\n"
        "Sn:0x%lX Ch:%d  Bat:%d\r\n"
        "Temp:%3.1f C Hum:%d%%",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        (uint32_t)(instance->generic.data >> 32),
        (uint32_t)(instance->generic.data),
        instance->generic.id,
        instance->generic.channel,
        instance->generic.battery_low,
        (double)instance->generic.temp,
        instance->generic.humidity);
}
