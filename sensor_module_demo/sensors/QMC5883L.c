#include "QMC5883L.h"

#define TAG "QMC5883L"

#define QMC5883L_REG_CHIPID 0x00
#define QMC5883L_REG_X_LSB 0x01
#define QMC5883L_REG_X_MSB 0x02
#define QMC5883L_REG_Y_LSB 0x03
#define QMC5883L_REG_Y_MSB 0x04
#define QMC5883L_REG_Z_LSB 0x05
#define QMC5883L_REG_Z_MSB 0x06
#define QMC5883L_REG_STATUS 0x09
#define QMC5883L_REG_CONTROL_1 0x0A
#define QMC5883L_REG_CONTROL_2 0x0B

#define QMC5883L_CHIPID_VALUE 0x80
// Data Ready register
#define QMC5883L_STATUS_DRDY_POS 0
#define QMC5883L_STATUS_DRDY_MSK (1 << QMC5883L_STATUS_DRDY_POS)
// Axis Value Overflow register
#define QMC5883L_STATUS_OVFL_POS 1
#define QMC5883L_STATUS_OVFL_MSK (1 << QMC5883L_STATUS_OVFL_POS)
// Mode register
#define QMC5883L_CONTROL_1_MODE_POS 0
#define QMC5883L_CONTROL_1_MODE_MSK (0b11 << QMC5883L_CONTROL_1_MODE_POS)
// Output Data Rate register
#define QMC5883L_CONTROL_1_ODR_POS 2
#define QMC5883L_CONTROL_1_ODR_MSK (0b11 << QMC5883L_CONTROL_1_ODR_POS)
// Oversampling Ratio register
#define QMC5883L_CONTROL_1_OSR1_POS 4
#define QMC5883L_CONTROL_1_OSR1_MSK (0b11 << QMC5883L_CONTROL_1_OSR1_POS)
// Downsampling Ratio register
#define QMC5883L_CONTROL_1_OSR2_POS 6
#define QMC5883L_CONTROL_1_OSR2_MSK (0b11 << QMC5883L_CONTROL_1_OSR2_POS)
// Set and reset mode register
#define QMC5883L_CONTROL_2_SET_RESET_MODE_POS 0
#define QMC5883L_CONTROL_2_SET_RESET_MODE_MSK (0b11 << QMC5883L_CONTROL_2_SET_RESET_MODE_POS)
// Range register
#define QMC5883L_CONTROL_2_RANGE_POS 2
#define QMC5883L_CONTROL_2_RANGE_MSK (0b11 << QMC5883L_CONTROL_2_RANGE_POS)
// Self-test register
#define QMC5883L_CONTROL_2_SELF_TEST_POS 6
#define QMC5883L_CONTROL_2_SELF_TEST_MSK (1 << QMC5883L_CONTROL_2_SELF_TEST_POS)
// Soft reset register
#define QMC5883L_CONTROL_2_SOFT_RESET_POS 7
#define QMC5883L_CONTROL_2_SOFT_RESET_MSK (1 << QMC5883L_CONTROL_2_SOFT_RESET_POS)

// Mode values
typedef enum {
    QMC5883LModeSuspend = 0b00,
    QMC5883LModeNormal = 0b01,
    QMC5883LModeSingle = 0b10,
    QMC5883LModeContinuous = 0b11,
} QMC5883LMode;

// Output Data Rate values
typedef enum {
    QMC5883L_ODR_10Hz = 0b00,
    QMC5883L_ODR_50Hz = 0b01,
    QMC5883L_ODR_100Hz = 0b10,
    QMC5883L_ODR_200Hz = 0b11,
} QMC5883L_ODR;

// Oversampling Ratio values
typedef enum {
    QMC5883L_OSR1_8 = 0b00,
    QMC5883L_OSR1_4 = 0b01,
    QMC5883L_OSR1_2 = 0b10,
    QMC5883L_OSR1_1 = 0b11,
} QMC5883L_OSR1;

// Downsampling Ratio values
typedef enum {
    QMC5883L_OSR2_1 = 0b00,
    QMC5883L_OSR2_2 = 0b01,
    QMC5883L_OSR2_4 = 0b10,
    QMC5883L_OSR2_8 = 0b11,
} QMC5883L_OSR2;

// Set and reset mode values
typedef enum {
    QMC5883LSetAndResetOn = 0b00,
    QMC5883LSetOnlyOn = 0b01,
    QMC5883LSetAndResetOff1 = 0b10,
    QMC5883LSetAndResetOff2 = 0b11,
} QMC5883LSetResetMode;

// Range values
typedef enum {
    QMC5883LRange30Gauss = 0b00,
    QMC5883LRange12Gauss = 0b01,
    QMC5883LRange8Gauss = 0b10,
    QMC5883LRange2Gauss = 0b11,
} QMC5883LRange;

struct QMC5883L {
    I2CDevice* device;
};

QMC5883L* qmc5883l_alloc(I2CDevice* device) {
    furi_check(device);
    QMC5883L* qmc5883l = (QMC5883L*)malloc(sizeof(QMC5883L));
    qmc5883l->device = device;
    return qmc5883l;
}

void qmc5883l_free(QMC5883L* qmc5883l) {
    free(qmc5883l);
}

bool qmc5883l_init(QMC5883L* qmc5883l) {
    // Reset the device using soft-reset
    i2c_write_reg(
        qmc5883l->device, QMC5883L_REG_CONTROL_2, 1 << QMC5883L_CONTROL_2_SOFT_RESET_POS);
    furi_delay_ms(10);

    // Read the chip-id
    uint8_t id = i2c_read_reg(qmc5883l->device, QMC5883L_REG_CHIPID);

    if(id != QMC5883L_CHIPID_VALUE) {
        FURI_LOG_E(
            TAG, "Sensor returned wrong ID 0x%02X, expected 0x%02X", id, QMC5883L_CHIPID_VALUE);
        return false;
    }

    // Magic reset sequence
    i2c_write_reg(qmc5883l->device, 0x29, 0x06);
    furi_delay_ms(10);

    // Set the control registers
    const uint8_t control_1 = 0xC5;
    const uint8_t control_2 = 0x0C;

    // write control_2 register
    i2c_write_reg(qmc5883l->device, QMC5883L_REG_CONTROL_2, control_2);
    furi_delay_ms(10);

    // sudo write control_2 register
    i2c_write_reg(qmc5883l->device, QMC5883L_REG_CONTROL_2, control_2);
    furi_delay_ms(10);

    // write control_1 register
    i2c_write_reg(qmc5883l->device, QMC5883L_REG_CONTROL_1, control_1);
    furi_delay_ms(10);

    // sudo write control_1 register
    i2c_write_reg(qmc5883l->device, QMC5883L_REG_CONTROL_1, control_1);
    furi_delay_ms(10);

    // Check if the device is OK
    uint8_t data;

    data = i2c_read_reg(qmc5883l->device, QMC5883L_REG_CONTROL_2);
    if(data != control_2) {
        FURI_LOG_E(TAG, "Sensor returned wrong data 0x%02X, expected 0x%02X", data, control_2);
        return false;
    }

    data = i2c_read_reg(qmc5883l->device, QMC5883L_REG_CONTROL_1);
    if(data != control_1) {
        FURI_LOG_E(TAG, "Sensor returned wrong data 0x%02X, expected 0x%02X", data, control_1);
        return false;
    }

    return true;
}

bool qmc5883l_deinit(QMC5883L* qmc5883l) {
    // Reset the device using soft-reset
    i2c_write_reg(
        qmc5883l->device, QMC5883L_REG_CONTROL_2, 1 << QMC5883L_CONTROL_2_SOFT_RESET_POS);
    return true;
}

static bool qmc5883l_is_measuring(QMC5883L* qmc5883l, bool* overflow) {
    uint8_t status = i2c_read_reg(qmc5883l->device, QMC5883L_REG_STATUS);
    *overflow = (status & QMC5883L_STATUS_OVFL_MSK) != 0;
    return (status & QMC5883L_STATUS_DRDY_MSK) == 0;
}

QMC5883LUpdateStatus qmc5883l_update(QMC5883L* qmc5883l, QMC5883LData* result) {
    uint32_t t = furi_get_tick();
    bool overflow;
    while(qmc5883l_is_measuring(qmc5883l, &overflow)) {
        if(furi_get_tick() - t > 100) {
            return QMC5883LUpdateStatusTimeout;
        }
    }

    uint8_t data[6];
    if(!i2c_read_reg_array(qmc5883l->device, QMC5883L_REG_X_LSB, 6, data)) {
        return QMC5883LUpdateStatusError;
    }

    result->x = (int16_t)(data[1] << 8 | data[0]);
    result->y = (int16_t)(data[3] << 8 | data[2]);
    result->z = (int16_t)(data[5] << 8 | data[4]);
    result->overflow = overflow;

    return QMC5883LUpdateStatusOK;
}