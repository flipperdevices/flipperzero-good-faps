#include "QMC5883L.h"

#define TAG "QMC5883L"

#define QMC5883L_REG_X_LSB 0x00
#define QMC5883L_REG_X_MSB 0x01
#define QMC5883L_REG_Y_LSB 0x02
#define QMC5883L_REG_Y_MSB 0x03
#define QMC5883L_REG_Z_LSB 0x04
#define QMC5883L_REG_Z_MSB 0x05
#define QMC5883L_REG_STATUS 0x06
#define QMC5883L_REG_TEMP_LSB 0x07
#define QMC5883L_REG_TEMP_MSB 0x08
#define QMC5883L_REG_CR1 0x09
#define QMC5883L_REG_CR2 0x0A
#define QMC5883L_REG_CR3 0x0B
#define QMC5883L_REG_ID 0x0D

#define QMC5883L_ID_VALUE 0xFF

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
    // Soft reset
    i2c_write_reg(qmc5883l->device, QMC5883L_REG_CR2, (1 << 7));
    furi_delay_ms(5);

    // Read the chip-id
    uint8_t id = i2c_read_reg(qmc5883l->device, QMC5883L_REG_ID);
    if(id != QMC5883L_ID_VALUE) {
        FURI_LOG_E(TAG, "Sensor returned wrong ID 0x%02X, expected 0x%02X", id, QMC5883L_ID_VALUE);
        return false;
    }

    // write control registers
    i2c_write_reg(qmc5883l->device, QMC5883L_REG_CR1, 0xDD);
    i2c_write_reg(qmc5883l->device, QMC5883L_REG_CR2, 0x40);
    i2c_write_reg(qmc5883l->device, QMC5883L_REG_CR3, 0x01);
    furi_delay_ms(5);

    return true;
}

bool qmc5883l_deinit(QMC5883L* qmc5883l) {
    i2c_write_reg(qmc5883l->device, QMC5883L_REG_CR2, (1 << 7));
    return true;
}

static bool qmc5883l_is_measuring(QMC5883L* qmc5883l) {
    uint8_t status = i2c_read_reg(qmc5883l->device, QMC5883L_REG_STATUS);
    return (status & 0x01) == 0;
}

QMC5883LUpdateStatus qmc5883l_update(QMC5883L* qmc5883l, QMC5883LData* result) {
    uint32_t t = furi_get_tick();
    while(qmc5883l_is_measuring(qmc5883l)) {
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

    return QMC5883LUpdateStatusOK;
}