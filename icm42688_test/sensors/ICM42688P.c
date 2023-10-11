#include "ICM42688P_regs.h"
#include "ICM42688P.h"

#define TAG "ICM42688P"

#define ICM42688P_TIMEOUT 100

struct ICM42688P {
    FuriHalSpiBusHandle* spi_bus;
    const GpioPin* irq_pin;
    float accel_scale;
    float gyro_scale;
};

static const struct AccelFullScale {
    float value;
    uint8_t reg_mask;
} accel_fs_modes[] = {
    [AccelFullScale16G] = {16.f, ICM42688_AFS_16G},
    [AccelFullScale8G] = {8.f, ICM42688_AFS_8G},
    [AccelFullScale4G] = {4.f, ICM42688_AFS_4G},
    [AccelFullScale2G] = {2.f, ICM42688_AFS_2G},
};

static const struct GyroFullScale {
    float value;
    uint8_t reg_mask;
} gyro_fs_modes[] = {
    [GyroFullScale2000DPS] = {2000.f, ICM42688_GFS_2000DPS},
    [GyroFullScale1000DPS] = {1000.f, ICM42688_GFS_1000DPS},
    [GyroFullScale500DPS] = {500.f, ICM42688_GFS_500DPS},
    [GyroFullScale250DPS] = {250.f, ICM42688_GFS_250DPS},
    [GyroFullScale125DPS] = {125.f, ICM42688_GFS_125DPS},
    [GyroFullScale62_5DPS] = {62.5f, ICM42688_GFS_62_5DPS},
    [GyroFullScale31_25DPS] = {31.25f, ICM42688_GFS_31_25DPS},
    [GyroFullScale15_625DPS] = {15.625f, ICM42688_GFS_15_625DPS},
};

static bool icm42688p_write_reg(FuriHalSpiBusHandle* spi_bus, uint8_t addr, uint8_t value) {
    bool res = false;
    furi_hal_spi_acquire(spi_bus);
    do {
        uint8_t cmd_data[2] = {addr & 0x7F, value};
        if(!furi_hal_spi_bus_tx(spi_bus, cmd_data, 2, ICM42688P_TIMEOUT)) break;
        res = true;
    } while(0);
    furi_hal_spi_release(spi_bus);
    return res;
}

static bool icm42688p_read_reg(FuriHalSpiBusHandle* spi_bus, uint8_t addr, uint8_t* value) {
    bool res = false;
    furi_hal_spi_acquire(spi_bus);
    do {
        uint8_t cmd_byte = addr | (1 << 7);
        if(!furi_hal_spi_bus_tx(spi_bus, &cmd_byte, 1, ICM42688P_TIMEOUT)) break;
        if(!furi_hal_spi_bus_rx(spi_bus, value, 1, ICM42688P_TIMEOUT)) break;
        res = true;
    } while(0);
    furi_hal_spi_release(spi_bus);
    return res;
}

static bool
    icm42688p_read_mem(FuriHalSpiBusHandle* spi_bus, uint8_t addr, uint8_t* data, uint8_t len) {
    bool res = false;
    furi_hal_spi_acquire(spi_bus);
    do {
        uint8_t cmd_byte = addr | (1 << 7);
        if(!furi_hal_spi_bus_tx(spi_bus, &cmd_byte, 1, ICM42688P_TIMEOUT)) break;
        if(!furi_hal_spi_bus_rx(spi_bus, data, len, ICM42688P_TIMEOUT)) break;
        res = true;
    } while(0);
    furi_hal_spi_release(spi_bus);
    return res;
}

bool icm42688p_accel_config(
    ICM42688P* icm42688p,
    ICM42688PAccelFullScale full_scale,
    ICM42688PDataRate rate) {
    icm42688p->accel_scale = accel_fs_modes[full_scale].value;
    uint8_t reg_value = accel_fs_modes[full_scale].reg_mask | rate;
    return icm42688p_write_reg(icm42688p->spi_bus, ICM42688_ACCEL_CONFIG0, reg_value);
}

float icm42688p_accel_get_full_scale(ICM42688P* icm42688p) {
    return icm42688p->accel_scale;
}

bool icm42688p_gyro_config(
    ICM42688P* icm42688p,
    ICM42688PGyroFullScale full_scale,
    ICM42688PDataRate rate) {
    icm42688p->gyro_scale = gyro_fs_modes[full_scale].value;
    uint8_t reg_value = gyro_fs_modes[full_scale].reg_mask | rate;
    return icm42688p_write_reg(icm42688p->spi_bus, ICM42688_GYRO_CONFIG0, reg_value);
}

float icm42688p_gyro_get_full_scale(ICM42688P* icm42688p) {
    return icm42688p->gyro_scale;
}

bool icm42688p_read_accel_raw(ICM42688P* icm42688p, ICM42688PRawData* data) {
    bool ret = icm42688p_read_mem(
        icm42688p->spi_bus, ICM42688_ACCEL_DATA_X1, (uint8_t*)data, sizeof(ICM42688PRawData));
    return ret;
}

bool icm42688p_read_gyro_raw(ICM42688P* icm42688p, ICM42688PRawData* data) {
    bool ret = icm42688p_read_mem(
        icm42688p->spi_bus, ICM42688_GYRO_DATA_X1, (uint8_t*)data, sizeof(ICM42688PRawData));
    return ret;
}

bool icm42688p_write_gyro_offset(ICM42688P* icm42688p, ICM42688PScaledData* scaled_data) {
    if((fabsf(scaled_data->x) > 64.f) || (fabsf(scaled_data->y) > 64.f) ||
       (fabsf(scaled_data->z) > 64.f)) {
        return false;
    }

    uint16_t offset_x = (uint16_t)(-(int16_t)(scaled_data->x * 32.f) * 16) >> 4;
    uint16_t offset_y = (uint16_t)(-(int16_t)(scaled_data->y * 32.f) * 16) >> 4;
    uint16_t offset_z = (uint16_t)(-(int16_t)(scaled_data->z * 32.f) * 16) >> 4;

    uint8_t offset_regs[9];
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_REG_BANK_SEL, 4);
    icm42688p_read_mem(icm42688p->spi_bus, ICM42688_OFFSET_USER0, offset_regs, 5);

    offset_regs[0] = offset_x & 0xFF;
    offset_regs[1] = (offset_x & 0xF00) >> 8;
    offset_regs[1] |= (offset_y & 0xF00) >> 4;
    offset_regs[2] = offset_y & 0xFF;
    offset_regs[3] = offset_z & 0xFF;
    offset_regs[4] &= 0xF0;
    offset_regs[4] |= (offset_z & 0x0F00) >> 8;

    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_OFFSET_USER0, offset_regs[0]);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_OFFSET_USER1, offset_regs[1]);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_OFFSET_USER2, offset_regs[2]);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_OFFSET_USER3, offset_regs[3]);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_OFFSET_USER4, offset_regs[4]);

    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_REG_BANK_SEL, 0);
    return true;
}

void icm42688p_apply_scale(ICM42688PRawData* raw_data, float full_scale, ICM42688PScaledData* data) {
    data->x = ((float)(raw_data->x)) / 32768.f * full_scale;
    data->y = ((float)(raw_data->y)) / 32768.f * full_scale;
    data->z = ((float)(raw_data->z)) / 32768.f * full_scale;
}

void icm42688p_apply_scale_fifo(
    ICM42688P* icm42688p,
    ICM42688PFifoPacket* fifo_data,
    ICM42688PScaledData* accel_data,
    ICM42688PScaledData* gyro_data) {
    float full_scale = icm42688p->accel_scale;
    accel_data->x = ((float)(fifo_data->a_x)) / 32768.f * full_scale;
    accel_data->y = ((float)(fifo_data->a_y)) / 32768.f * full_scale;
    accel_data->z = ((float)(fifo_data->a_z)) / 32768.f * full_scale;

    full_scale = icm42688p->gyro_scale;
    gyro_data->x = ((float)(fifo_data->g_x)) / 32768.f * full_scale;
    gyro_data->y = ((float)(fifo_data->g_y)) / 32768.f * full_scale;
    gyro_data->z = ((float)(fifo_data->g_z)) / 32768.f * full_scale;
}

float icm42688p_read_temp(ICM42688P* icm42688p) {
    uint8_t reg_val[2];

    // TODO: liitle/big endian check
    icm42688p_read_mem(icm42688p->spi_bus, ICM42688_TEMP_DATA1, reg_val, 2);
    int16_t temp_int = (reg_val[0] << 8) | reg_val[1];
    return ((float)temp_int / 132.48f) + 25.f;
}

void icm42688_fifo_enable(
    ICM42688P* icm42688p,
    ICM42688PIrqCallback irq_callback,
    void* irq_context) {
    // FIFO mode: stream
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_FIFO_CONFIG, (1 << 6));
    // Little-endian data, FIFO count in records
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_INTF_CONFIG0, (1 << 7) | (1 << 6));
    // FIFO partial read, FIFO packet: gyro + accel TODO: 20bit
    icm42688p_write_reg(
        icm42688p->spi_bus, ICM42688_FIFO_CONFIG1, (1 << 6) | (1 << 5) | (1 << 1) | (1 << 0));
    // FIFO irq watermark
    uint16_t fifo_watermark = 1;
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_FIFO_CONFIG2, fifo_watermark & 0xFF);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_FIFO_CONFIG3, fifo_watermark >> 8);

    // IRQ1: push-pull, active high
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_INT_CONFIG, (1 << 1) | (1 << 0));
    // Clear IRQ on status read
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_INT_CONFIG0, 0);
    // IRQ pulse duration
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_INT_CONFIG1, (1 << 6) | (1 << 5));

    uint8_t reg_data = 0;
    icm42688p_read_reg(icm42688p->spi_bus, ICM42688_INT_STATUS, &reg_data);

    furi_hal_gpio_init(icm42688p->irq_pin, GpioModeInterruptRise, GpioPullDown, GpioSpeedVeryHigh);
    furi_hal_gpio_remove_int_callback(icm42688p->irq_pin);
    furi_hal_gpio_add_int_callback(icm42688p->irq_pin, irq_callback, irq_context);

    // IRQ1 source: FIFO threshold
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_INT_SOURCE0, (1 << 2));
}

void icm42688_fifo_disable(ICM42688P* icm42688p) {
    furi_hal_gpio_remove_int_callback(icm42688p->irq_pin);
    furi_hal_gpio_init(icm42688p->irq_pin, GpioModeAnalog, GpioPullNo, GpioSpeedLow);

    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_INT_SOURCE0, 0);

    // FIFO mode: bypass
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_FIFO_CONFIG, 0);
}

uint16_t icm42688_fifo_get_count(ICM42688P* icm42688p) {
    uint16_t reg_val = 0;
    icm42688p_read_mem(icm42688p->spi_bus, ICM42688_FIFO_COUNTH, (uint8_t*)&reg_val, 2);
    return reg_val;
}

bool icm42688_fifo_read(ICM42688P* icm42688p, ICM42688PFifoPacket* data) {
    icm42688p_read_mem(
        icm42688p->spi_bus, ICM42688_FIFO_DATA, (uint8_t*)data, sizeof(ICM42688PFifoPacket));
    return (data->header) & (1 << 7);
}

static void read_awg_gyro(ICM42688P* icm42688p, int32_t* data) {
    int32_t sample_cnt = 0;
    size_t timeout = 300;

    do {
        furi_delay_ms(1);
        uint8_t int_status = 0;
        icm42688p_read_reg(icm42688p->spi_bus, ICM42688_INT_STATUS, &int_status);

        if(int_status & (1 << 3)) {
            ICM42688PRawData raw_data;
            icm42688p_read_gyro_raw(icm42688p, &raw_data);
            data[0] += (int32_t)raw_data.x;
            data[1] += (int32_t)raw_data.y;
            data[2] += (int32_t)raw_data.z;
            sample_cnt++;
        }
        timeout--;
    } while((sample_cnt < 200) && (timeout > 0));

    data[0] /= sample_cnt;
    data[1] /= sample_cnt;
    data[2] /= sample_cnt;
}

static void read_awg_accel(ICM42688P* icm42688p, int32_t* data) {
    int32_t sample_cnt = 0;
    size_t timeout = 300;

    do {
        furi_delay_ms(1);
        uint8_t int_status = 0;
        icm42688p_read_reg(icm42688p->spi_bus, ICM42688_INT_STATUS, &int_status);

        if(int_status & (1 << 3)) {
            ICM42688PRawData raw_data;
            icm42688p_read_accel_raw(icm42688p, &raw_data);
            data[0] += (int32_t)raw_data.x;
            data[1] += (int32_t)raw_data.y;
            data[2] += (int32_t)raw_data.z;
            sample_cnt++;
        }
        timeout--;
    } while((sample_cnt < 200) && (timeout > 0));

    data[0] /= sample_cnt;
    data[1] /= sample_cnt;
    data[2] /= sample_cnt;
}

#define INV_ST_OTP_EQUATION(FS, ST_code) \
    (uint32_t)((2620.f / powf(2.f, 3.f - FS)) * powf(1.01f, ST_code - 1.f) + 0.5f)

/* Pass/Fail criteria */
#define MIN_RATIO 0.5f /* expected ratio greater than 0.5 */
#define MAX_RATIO 1.5f /* expected ratio lower than 1.5 */

#define MIN_ST_GYRO_DPS 60 /* expected values greater than 60dps */
#define MAX_ST_GYRO_OFFSET_DPS 20 /* expected offset less than 20 dps */

#define MIN_ST_ACCEL_MG 50 /* expected values in [50mgee;1200mgee] */
#define MAX_ST_ACCEL_MG 1200

static bool run_gyro_self_test(ICM42688P* icm42688p, int32_t* st_bias) {
    int32_t values_normal[3] = {0};
    int32_t values_selftest[3] = {0};
    uint32_t selftest_response[3];

    uint8_t selftest_data[3];
    uint32_t selftest_otp[3];

    bool result = true;

    icm42688p_gyro_config(icm42688p, GyroFullScale250DPS, DataRate1kHz);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_GYRO_CONFIG1, 0x1A);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_GYRO_ACCEL_CONFIG0, 0x14);

    read_awg_gyro(icm42688p, values_normal);

    icm42688p_write_reg(
        icm42688p->spi_bus, ICM42688_SELF_TEST_CONFIG, (1 << 0) | (1 << 1) | (1 << 2));
    furi_delay_ms(200);

    read_awg_gyro(icm42688p, values_selftest);

    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_SELF_TEST_CONFIG, 0);

    for(uint8_t i = 0; i < 3; i++) {
        selftest_response[i] = abs(values_selftest[i] - values_normal[i]);
    }

    /* Read ST_DATA */
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_REG_BANK_SEL, 1);
    icm42688p_read_mem(icm42688p->spi_bus, ICM42688_XG_ST_DATA, selftest_data, 3);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_REG_BANK_SEL, 0);

    /* If ST_DATA = 0 for any axis */
    if(selftest_data[0] == 0 || selftest_data[1] == 0 || selftest_data[2] == 0) {
        /* compare the Self-Test response to the ST absolute limits */
        for(uint8_t i = 0; i < 3; i++) {
            if(selftest_response[i] < (MIN_ST_GYRO_DPS * 32768 / 250)) {
                FURI_LOG_E(TAG, "ST resp limits");
                result = false;
            }
        }
    } else { /* If ST_DATA != 0 for all axis */
        /* compare the Self-Test response to the factory OTP values */
        for(uint8_t i = 0; i < 3; i++) {
            selftest_otp[i] = INV_ST_OTP_EQUATION(GyroFullScale250DPS, selftest_data[i]);
            if(selftest_otp[i] == 0) {
                FURI_LOG_E(TAG, "ST resp otp 0");
                result = false;
            } else {
                float ratio = ((float)selftest_response[i]) / ((float)selftest_otp[i]);
                if((ratio >= MAX_RATIO) || (ratio <= MIN_RATIO)) {
                    FURI_LOG_E(TAG, "ST resp otp ratio");
                    result = false;
                }
            }
        }
    }

    /* stored the computed bias (checking GST and GOFFSET values) */
    for(uint8_t i = 0; i < 3; i++) {
        if((abs(values_normal[i]) > (int32_t)(MAX_ST_GYRO_OFFSET_DPS * 32768 / 250))) {
            result = false;
            FURI_LOG_E(TAG, "ST resp bias");
        }

        st_bias[i] = values_normal[i];
    }

    return result;
}

static bool run_accel_self_test(ICM42688P* icm42688p, int32_t* st_bias) {
    int32_t values_normal[3] = {0};
    int32_t values_selftest[3] = {0};
    uint32_t selftest_response[3];

    uint8_t selftest_data[3];
    uint32_t selftest_otp[3];

    bool result = true;

    uint32_t accel_sensitivity_1g = 32768 / 2;

    icm42688p_accel_config(icm42688p, AccelFullScale2G, DataRate1kHz);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_ACCEL_CONFIG1, 0x15);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_GYRO_ACCEL_CONFIG0, 0x41);

    read_awg_accel(icm42688p, values_normal);

    icm42688p_write_reg(
        icm42688p->spi_bus, ICM42688_SELF_TEST_CONFIG, (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6));
    furi_delay_ms(25);

    read_awg_accel(icm42688p, values_selftest);

    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_SELF_TEST_CONFIG, 0);

    for(uint8_t i = 0; i < 3; i++) {
        selftest_response[i] = abs(values_selftest[i] - values_normal[i]);
    }

    /* Read ST_DATA */
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_REG_BANK_SEL, 2);
    icm42688p_read_mem(icm42688p->spi_bus, ICM42688_XA_ST_DATA, selftest_data, 3);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_REG_BANK_SEL, 0);

    /* If ST_DATA = 0 for any axis */
    if(selftest_data[0] == 0 || selftest_data[1] == 0 || selftest_data[2] == 0) {
        /* compare the Self-Test response to the ST absolute limits */
        for(uint8_t i = 0; i < 3; i++) {
            if((selftest_response[i] < ((MIN_ST_ACCEL_MG * accel_sensitivity_1g) / 1000)) ||
               (selftest_response[i] > ((MAX_ST_ACCEL_MG * accel_sensitivity_1g) / 1000))) {
                FURI_LOG_E(TAG, "ST resp limits");
                result = false;
            }
        }
    } else { /* If ST_DATA != 0 for all axis */
        /* compare the Self-Test response to the factory OTP values */
        for(uint8_t i = 0; i < 3; i++) {
            selftest_otp[i] = INV_ST_OTP_EQUATION(AccelFullScale2G, selftest_data[i]);
            if(selftest_otp[i] == 0) {
                FURI_LOG_E(TAG, "ST resp otp 0");
                result = false;
            } else {
                float ratio = ((float)selftest_response[i]) / ((float)selftest_otp[i]);
                if((ratio >= MAX_RATIO) || (ratio <= MIN_RATIO)) {
                    FURI_LOG_E(TAG, "ST resp otp ratio");
                    result = false;
                }
            }
        }
    }

    /* stored the computed offset */
    for(uint8_t i = 0; i < 3; i++) {
        st_bias[i] = values_normal[i];
    }

    /* assume the largest data axis shows +1 or -1 G for gravity */
    uint8_t axis = 0;
    int8_t axis_sign = 1;
    if(abs(st_bias[1]) > abs(st_bias[0])) {
        axis = 1;
    }
    if(abs(st_bias[2]) > abs(st_bias[axis])) {
        axis = 2;
    }
    if(st_bias[axis] < 0) {
        axis_sign = -1;
    }

    st_bias[axis] -= accel_sensitivity_1g * axis_sign;

    return result;
}

bool icm42688_selftest(ICM42688P* icm42688p, bool* gyro_ok, bool* accel_ok) {
    int32_t gyro_raw_bias[3] = {0};
    int32_t accel_raw_bias[3] = {0};

    *gyro_ok = run_gyro_self_test(icm42688p, gyro_raw_bias);
    if(*gyro_ok) {
        FURI_LOG_I(TAG, "Gyro Selftest PASS");
    } else {
        FURI_LOG_E(TAG, "Gyro Selftest FAIL");
    }

    *accel_ok = run_accel_self_test(icm42688p, accel_raw_bias);
    if(*accel_ok) {
        FURI_LOG_I(TAG, "Accel Selftest PASS");
    } else {
        FURI_LOG_E(TAG, "Accel Selftest FAIL");
    }

    FURI_LOG_I(
        TAG,
        "Gyro bias: x=%ld, y=%ld, z=%ld",
        gyro_raw_bias[0],
        gyro_raw_bias[1],
        gyro_raw_bias[2]);
    FURI_LOG_I(
        TAG,
        "Accel bias: x=%ld, y=%ld, z=%ld",
        accel_raw_bias[0],
        accel_raw_bias[1],
        accel_raw_bias[2]);
    return true;
}

ICM42688P* icm42688p_alloc(FuriHalSpiBusHandle* spi_bus, const GpioPin* irq_pin) {
    ICM42688P* icm42688p = malloc(sizeof(ICM42688P));
    icm42688p->spi_bus = spi_bus;
    icm42688p->irq_pin = irq_pin;
    return icm42688p;
}

void icm42688p_free(ICM42688P* icm42688p) {
    free(icm42688p);
}

bool icm42688p_init(ICM42688P* icm42688p) {
    furi_hal_spi_bus_handle_init(icm42688p->spi_bus);

    // Software reset
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_REG_BANK_SEL, 0); // Set reg bank to 0
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_DEVICE_CONFIG, 0x01); // SPI Mode 0, SW reset
    furi_delay_ms(1);

    uint8_t reg_value = 0;
    bool read_ok = icm42688p_read_reg(icm42688p->spi_bus, ICM42688_WHO_AM_I, &reg_value);
    if(!read_ok) {
        FURI_LOG_E(TAG, "Chip ID read failed");
        return false;
    } else if(reg_value != ICM42688_WHOAMI) {
        FURI_LOG_E(
            TAG, "Sensor returned wrong ID 0x%02X, expected 0x%02X", reg_value, ICM42688_WHOAMI);
        return false;
    }

    // Disable all interrupts
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_INT_SOURCE0, 0);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_INT_SOURCE1, 0);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_INT_SOURCE3, 0);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_INT_SOURCE4, 0);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_REG_BANK_SEL, 4);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_INT_SOURCE6, 0);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_INT_SOURCE7, 0);
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_REG_BANK_SEL, 0);

    // Data format: little endian
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_INTF_CONFIG0, 0);

    // Enable all sensors
    icm42688p_write_reg(
        icm42688p->spi_bus,
        ICM42688_PWR_MGMT0,
        ICM42688_PWR_TEMP_ON | ICM42688_PWR_GYRO_MODE_LN | ICM42688_PWR_ACCEL_MODE_LN);
    furi_delay_ms(45);

    icm42688p_accel_config(icm42688p, AccelFullScale16G, DataRate1kHz);
    icm42688p_gyro_config(icm42688p, GyroFullScale2000DPS, DataRate1kHz);

    return true;
}

bool icm42688p_deinit(ICM42688P* icm42688p) {
    // Software reset
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_REG_BANK_SEL, 0); // Set reg bank to 0
    icm42688p_write_reg(icm42688p->spi_bus, ICM42688_DEVICE_CONFIG, 0x01); // SPI Mode 0, SW reset

    furi_hal_spi_bus_handle_deinit(icm42688p->spi_bus);
    return true;
}
