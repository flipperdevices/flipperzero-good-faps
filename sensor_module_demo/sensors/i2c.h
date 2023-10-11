#pragma once

#include <furi.h>
#include <furi_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct I2CDevice I2CDevice;

I2CDevice* i2c_device_alloc(FuriHalI2cBusHandle* handle, uint8_t address);

void i2c_device_free(I2CDevice* device);

bool i2c_is_device_ready(I2CDevice* device);

uint8_t i2c_read_reg(I2CDevice* device, uint8_t reg);

bool i2c_read_array(I2CDevice* device, uint8_t len, uint8_t* data);

bool i2c_read_reg_array(I2CDevice* device, uint8_t startReg, uint8_t len, uint8_t* data);

bool i2c_write_reg(I2CDevice* device, uint8_t reg, uint8_t value);

bool i2c_write_array(I2CDevice* device, uint8_t len, uint8_t* data);

bool i2c_write_reg_array(I2CDevice* device, uint8_t startReg, uint8_t len, uint8_t* data);

#ifdef __cplusplus
}
#endif