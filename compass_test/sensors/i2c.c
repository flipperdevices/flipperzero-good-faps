/*
    Unitemp - Universal temperature reader
    Copyright (C) 2022-2023  Victor Nikitchuk (https://github.com/quen0n)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "i2c.h"

struct I2CDevice {
    FuriHalI2cBusHandle* handle;
    uint8_t address;
};

bool i2c_is_device_ready(I2CDevice* device) {
    furi_hal_i2c_acquire(device->handle);
    bool status = furi_hal_i2c_is_device_ready(device->handle, device->address, 10);
    furi_hal_i2c_release(device->handle);
    return status;
}

uint8_t i2c_read_reg(I2CDevice* device, uint8_t reg) {
    furi_hal_i2c_acquire(device->handle);
    uint8_t buff[1] = {0};
    furi_hal_i2c_read_mem(device->handle, device->address, reg, buff, 1, 10);
    furi_hal_i2c_release(device->handle);
    return buff[0];
}

bool i2c_read_array(I2CDevice* device, uint8_t len, uint8_t* data) {
    furi_hal_i2c_acquire(device->handle);
    bool status = furi_hal_i2c_rx(device->handle, device->address, data, len, 10);
    furi_hal_i2c_release(device->handle);
    return status;
}

bool i2c_read_reg_array(I2CDevice* device, uint8_t startReg, uint8_t len, uint8_t* data) {
    furi_hal_i2c_acquire(device->handle);
    bool status = furi_hal_i2c_read_mem(device->handle, device->address, startReg, data, len, 10);
    furi_hal_i2c_release(device->handle);
    return status;
}

bool i2c_write_reg(I2CDevice* device, uint8_t reg, uint8_t value) {
    furi_hal_i2c_acquire(device->handle);
    uint8_t buff[1] = {value};
    bool status = furi_hal_i2c_write_mem(device->handle, device->address, reg, buff, 1, 10);
    furi_hal_i2c_release(device->handle);
    return status;
}

bool i2c_write_array(I2CDevice* device, uint8_t len, uint8_t* data) {
    furi_hal_i2c_acquire(device->handle);
    bool status = furi_hal_i2c_tx(device->handle, device->address, data, len, 10);
    furi_hal_i2c_release(device->handle);
    return status;
}

bool i2c_write_reg_array(I2CDevice* device, uint8_t startReg, uint8_t len, uint8_t* data) {
    furi_hal_i2c_acquire(device->handle);
    bool status = furi_hal_i2c_write_mem(device->handle, device->address, startReg, data, len, 10);
    furi_hal_i2c_release(device->handle);
    return status;
}

I2CDevice* i2c_device_alloc(FuriHalI2cBusHandle* handle, uint8_t address) {
    I2CDevice* device = malloc(sizeof(I2CDevice));
    device->handle = handle;
    device->address = address;
    return device;
}

void i2c_device_free(I2CDevice* device) {
    free(device);
}
