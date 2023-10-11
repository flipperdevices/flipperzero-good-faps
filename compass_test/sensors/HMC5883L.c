#include "HMC5883L.h"

#define TAG "HMC5883L"

#define HMC5883L_REG_CRA 0x00
#define HMC5883L_REG_CRB 0x01
#define HMC5883L_REG_MODE 0x02
#define HMC5883L_REG_OUTXM 0x03
#define HMC5883L_REG_OUTXL 0x04
#define HMC5883L_REG_OUTYM 0x07
#define HMC5883L_REG_OUTYL 0x08
#define HMC5883L_REG_OUTZM 0x05
#define HMC5883L_REG_OUTZL 0x06
#define HMC5883L_REG_STATUS 0x09
#define HMC5883L_REG_IDA 0x0A
#define HMC5883L_REG_IDB 0x0B
#define HMC5883L_REG_IDC 0x0C

#define HMC5883L_ID_VALUE_A 0x48
#define HMC5883L_ID_VALUE_B 0x34
#define HMC5883L_ID_VALUE_C 0x33

struct HMC5883L {
    I2CDevice* device;
};

HMC5883L* hmc5883l_alloc(I2CDevice* device) {
    furi_check(device);
    HMC5883L* hmc5883l = (HMC5883L*)malloc(sizeof(HMC5883L));
    hmc5883l->device = device;
    return hmc5883l;
}

void hmc5883l_free(HMC5883L* hmc5883l) {
    free(hmc5883l);
}

bool hmc5883l_init(HMC5883L* hmc5883l) {
    // Read the chip-id
    uint8_t id = i2c_read_reg(hmc5883l->device, HMC5883L_REG_IDA);
    if(id != HMC5883L_ID_VALUE_A) {
        FURI_LOG_E(
            TAG, "Sensor returned wrong ID_A 0x%02X, expected 0x%02X", id, HMC5883L_ID_VALUE_A);
        return false;
    }

    id = i2c_read_reg(hmc5883l->device, HMC5883L_REG_IDB);
    if(id != HMC5883L_ID_VALUE_B) {
        FURI_LOG_E(
            TAG, "Sensor returned wrong ID_B 0x%02X, expected 0x%02X", id, HMC5883L_ID_VALUE_B);
        return false;
    }

    id = i2c_read_reg(hmc5883l->device, HMC5883L_REG_IDC);
    if(id != HMC5883L_ID_VALUE_C) {
        FURI_LOG_E(
            TAG, "Sensor returned wrong ID_C 0x%02X, expected 0x%02X", id, HMC5883L_ID_VALUE_C);
        return false;
    }

    // write control registers
    i2c_write_reg(hmc5883l->device, HMC5883L_REG_CRA, 0x78);
    i2c_write_reg(hmc5883l->device, HMC5883L_REG_CRB, 0x60);
    i2c_write_reg(hmc5883l->device, HMC5883L_REG_MODE, 0x00);
    furi_delay_ms(5);

    return true;
}

bool hmc5883l_deinit(HMC5883L* hmc5883l) {
    i2c_write_reg(hmc5883l->device, HMC5883L_REG_MODE, 0x03); // IDLE mode
    return true;
}

static bool hmc5883l_is_measuring(HMC5883L* hmc5883l) {
    uint8_t status = i2c_read_reg(hmc5883l->device, HMC5883L_REG_STATUS);
    return (status & 0x01) == 0;
}

HMC5883LUpdateStatus hmc5883l_update(HMC5883L* hmc5883l, HMC5883LData* result) {
    uint32_t t = furi_get_tick();
    while(hmc5883l_is_measuring(hmc5883l)) {
        if(furi_get_tick() - t > 100) {
            return HMC5883LUpdateStatusTimeout;
        }
    }

    uint8_t data[6];
    if(!i2c_read_reg_array(hmc5883l->device, HMC5883L_REG_OUTXM, 6, data)) {
        return HMC5883LUpdateStatusError;
    }

    result->x = (int16_t)(data[0] << 8 | data[1]);
    result->z = (int16_t)(data[2] << 8 | data[3]);
    result->y = (int16_t)(data[4] << 8 | data[5]);

    return HMC5883LUpdateStatusOK;
}