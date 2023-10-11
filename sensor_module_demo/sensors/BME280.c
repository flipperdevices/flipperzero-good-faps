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

#include "BME280.h"

typedef struct {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
} BME280_temp_cal;

typedef struct {
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
} BME280_press_cal;

typedef struct {
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} BME280_hum_cal;

struct BME280 {
    // Temperature calibration data
    BME280_temp_cal temp_cal;
    // Pressure calibration data
    BME280_press_cal press_cal;
    // Humidity calibration data
    BME280_hum_cal hum_cal;
    // Last calibration update time
    uint32_t last_cal_update_time;
    // Temperature correction value
    int32_t t_fine;
    // I2C device
    I2CDevice* device;
};

// Calibration update interval
#define BOSCH_CAL_UPDATE_INTERVAL 60000

#define TEMP_CAL_START_ADDR 0x88
#define PRESS_CAL_START_ADDR 0x8E
#define HUM_CAL_H1_ADDR 0xA1
#define HUM_CAL_H2_ADDR 0xE1

#define BMP280_ID 0x58
#define BME280_ID 0x60

#define BMx280_REG_STATUS 0xF3
#define BMx280_REG_CTRL_MEAS 0xF4
#define BMx280_REG_CONFIG 0xF5
#define BME280_REG_CTRL_HUM 0xF2

// Temperature oversampling settings
#define BMx280_TEMP_OVERSAMPLING_SKIP 0b00000000
#define BMx280_TEMP_OVERSAMPLING_1 0b00100000
#define BMx280_TEMP_OVERSAMPLING_2 0b01000000
#define BMx280_TEMP_OVERSAMPLING_4 0b01100000
#define BMx280_TEMP_OVERSAMPLING_8 0b10000000
#define BMx280_TEMP_OVERSAMPLING_16 0b10100000

// Pressure oversampling settings
#define BMx280_PRESS_OVERSAMPLING_SKIP 0b00000000
#define BMx280_PRESS_OVERSAMPLING_1 0b00000100
#define BMx280_PRESS_OVERSAMPLING_2 0b00001000
#define BMx280_PRESS_OVERSAMPLING_4 0b00001100
#define BMx280_PRESS_OVERSAMPLING_8 0b00010000
#define BMx280_PRESS_OVERSAMPLING_16 0b00010100

// Humidity oversampling settings
#define BME280_HUM_OVERSAMPLING_SKIP 0b00000000
#define BME280_HUM_OVERSAMPLING_1 0b00000001
#define BME280_HUM_OVERSAMPLING_2 0b00000010
#define BME280_HUM_OVERSAMPLING_4 0b00000011
#define BME280_HUM_OVERSAMPLING_8 0b00000100
#define BME280_HUM_OVERSAMPLING_16 0b00000101u

// Mode
#define BMx280_MODE_SLEEP 0b00000000 // Sleep mode
#define BMx280_MODE_FORCED 0b00000001 // Forced 1 measurement
#define BMx280_MODE_NORMAL 0b00000011 // Normal mode

// Normal mode standby time
#define BMx280_STANDBY_TIME_0_5 0b00000000
#define BMx280_STANDBY_TIME_62_5 0b00100000
#define BMx280_STANDBY_TIME_125 0b01000000
#define BMx280_STANDBY_TIME_250 0b01100000
#define BMx280_STANDBY_TIME_500 0b10000000
#define BMx280_STANDBY_TIME_1000 0b10100000
#define BMx280_STANDBY_TIME_2000 0b11000000
#define BMx280_STANDBY_TIME_4000 0b11100000

// IIR filter coefficient
#define BMx280_FILTER_COEFF_1 0b00000000
#define BMx280_FILTER_COEFF_2 0b00000100
#define BMx280_FILTER_COEFF_4 0b00001000
#define BMx280_FILTER_COEFF_8 0b00001100
#define BMx280_FILTER_COEFF_16 0b00010000

// Enable 3-wire SPI interface
#define BMx280_SPI_3W_ENABLE 0b00000001
#define BMx280_SPI_3W_DISABLE 0b00000000

#define TAG "BME280"

static float bme280_compensate_temperature(BME280* bme280, int32_t adc_T) {
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)bme280->temp_cal.dig_T1 << 1))) *
            ((int32_t)bme280->temp_cal.dig_T2)) >>
           11;
    var2 = (((((adc_T >> 4) - ((int32_t)bme280->temp_cal.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)bme280->temp_cal.dig_T1))) >>
             12) *
            ((int32_t)bme280->temp_cal.dig_T3)) >>
           14;
    bme280->t_fine = var1 + var2;
    return ((bme280->t_fine * 5 + 128) >> 8) / 100.0f;
}

static float bme280_compensate_pressure(BME280* bme280, int32_t adc_P) {
    int32_t var1, var2;
    uint32_t p;
    var1 = (((int32_t)bme280->t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)bme280->press_cal.dig_P6);
    var2 = var2 + ((var1 * ((int32_t)bme280->press_cal.dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)bme280->press_cal.dig_P4) << 16);
    var1 = (((bme280->press_cal.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) +
            ((((int32_t)bme280->press_cal.dig_P2) * var1) >> 1)) >>
           18;
    var1 = ((((32768 + var1)) * ((int32_t)bme280->press_cal.dig_P1)) >> 15);
    if(var1 == 0) {
        return 0; // avoid exception caused by division by zero
    }
    p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
    if(p < 0x80000000) {
        p = (p << 1) / ((uint32_t)var1);
    } else {
        p = (p / (uint32_t)var1) * 2;
    }
    var1 = (((int32_t)bme280->press_cal.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)bme280->press_cal.dig_P8)) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + bme280->press_cal.dig_P7) >> 4));
    return p;
}

static float bme280_compensate_humidity(BME280* bme280, int32_t adc_H) {
    int32_t v_x1_u32r;
    v_x1_u32r = (bme280->t_fine - ((int32_t)76800));

    v_x1_u32r =
        (((((adc_H << 14) - (((int32_t)bme280->hum_cal.dig_H4) << 20) -
            (((int32_t)bme280->hum_cal.dig_H5) * v_x1_u32r)) +
           ((int32_t)16384)) >>
          15) *
         (((((((v_x1_u32r * ((int32_t)bme280->hum_cal.dig_H6)) >> 10) *
              (((v_x1_u32r * ((int32_t)bme280->hum_cal.dig_H3)) >> 11) + ((int32_t)32768))) >>
             10) +
            ((int32_t)2097152)) *
               ((int32_t)bme280->hum_cal.dig_H2) +
           8192) >>
          14));

    v_x1_u32r =
        (v_x1_u32r -
         (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)bme280->hum_cal.dig_H1)) >>
          4));

    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return ((uint32_t)(v_x1_u32r >> 12)) / 1024.0f;
}

static bool bme280_read_calibration_values(BME280* bme280) {
    if(!i2c_read_reg_array(bme280->device, TEMP_CAL_START_ADDR, 6, (uint8_t*)&bme280->temp_cal))
        return false;

    FURI_LOG_D(
        TAG,
        "Sensor BMx280 T1-T3: %d, %d, %d",
        bme280->temp_cal.dig_T1,
        bme280->temp_cal.dig_T2,
        bme280->temp_cal.dig_T3);

    if(!i2c_read_reg_array(bme280->device, PRESS_CAL_START_ADDR, 18, (uint8_t*)&bme280->press_cal))
        return false;

    FURI_LOG_D(
        TAG,
        "Sensor BMx280 P1-P9: %d, %d, %d, %d, %d, %d, %d, %d, %d",
        bme280->press_cal.dig_P1,
        bme280->press_cal.dig_P2,
        bme280->press_cal.dig_P3,
        bme280->press_cal.dig_P4,
        bme280->press_cal.dig_P5,
        bme280->press_cal.dig_P6,
        bme280->press_cal.dig_P7,
        bme280->press_cal.dig_P8,
        bme280->press_cal.dig_P9);

    uint8_t buff[7] = {0};
    if(!i2c_read_reg_array(bme280->device, HUM_CAL_H1_ADDR, 1, buff)) return false;
    bme280->hum_cal.dig_H1 = buff[0];

    if(!i2c_read_reg_array(bme280->device, HUM_CAL_H2_ADDR, 7, buff)) return false;
    bme280->hum_cal.dig_H2 = (uint16_t)(buff[0] | ((uint16_t)buff[1] << 8));
    bme280->hum_cal.dig_H3 = buff[2];
    bme280->hum_cal.dig_H4 = ((int16_t)buff[3] << 4) | (buff[4] & 0x0F);
    bme280->hum_cal.dig_H5 = (buff[4] & 0x0F) | ((int16_t)buff[5] << 4);
    bme280->hum_cal.dig_H6 = buff[6];

    FURI_LOG_D(
        TAG,
        "Sensor BMx280: H1-H6: %d, %d, %d, %d, %d, %d",
        bme280->hum_cal.dig_H1,
        bme280->hum_cal.dig_H2,
        bme280->hum_cal.dig_H3,
        bme280->hum_cal.dig_H4,
        bme280->hum_cal.dig_H5,
        bme280->hum_cal.dig_H6);

    bme280->last_cal_update_time = furi_get_tick();
    return true;
}
static bool bme280_is_measuring(BME280* bme280) {
    return (bool)((i2c_read_reg(bme280->device, BMx280_REG_STATUS) & 0x08) >> 3);
}

BME280* bme280_alloc(I2CDevice* device) {
    furi_check(device);
    BME280* bme280 = malloc(sizeof(BME280));
    bme280->device = device;
    return bme280;
}

bool bme280_init(BME280* bme280) {
    // Reset the device using soft-reset
    i2c_write_reg(bme280->device, 0xE0, 0xB6);

    // Read the chip-id
    uint8_t id = i2c_read_reg(bme280->device, 0xD0);

    if(id != BME280_ID) {
        FURI_LOG_E(TAG, "Sensor returned wrong ID 0x%02X, expected 0x%02X", id, BME280_ID);
        return false;
    }

    // Set humidity oversampling to 1
    i2c_write_reg(bme280->device, BME280_REG_CTRL_HUM, BME280_HUM_OVERSAMPLING_1);
    i2c_write_reg(
        bme280->device, BME280_REG_CTRL_HUM, i2c_read_reg(bme280->device, BME280_REG_CTRL_HUM));

    // Set temperature oversampling to 1, pressure oversampling to 4, and mode to normal
    i2c_write_reg(
        bme280->device,
        BMx280_REG_CTRL_MEAS,
        BMx280_TEMP_OVERSAMPLING_1 | BMx280_PRESS_OVERSAMPLING_4 | BMx280_MODE_NORMAL);

    // Set measure interval to 500ms, IIR filter coefficient to 16 and disable 3-wire SPI
    i2c_write_reg(
        bme280->device,
        BMx280_REG_CONFIG,
        BMx280_STANDBY_TIME_500 | BMx280_FILTER_COEFF_16 | BMx280_SPI_3W_DISABLE);

    // read calibration values
    if(!bme280_read_calibration_values(bme280)) {
        FURI_LOG_E(TAG, "Failed to read calibration values");
        return false;
    }
    return true;
}

bool bme280_deinit(BME280* bme280) {
    // set to sleep mode
    i2c_write_reg(bme280->device, BMx280_REG_CTRL_MEAS, BMx280_MODE_SLEEP);
    return true;
}

BME280UpdateStatus bme280_update(BME280* bme280, BME280Data* data) {
    uint32_t t = furi_get_tick();

    uint8_t buff[3];

    // check if sensor is initialized
    i2c_read_reg_array(bme280->device, 0xF4, 2, buff);
    if(buff[0] == 0) {
        FURI_LOG_W(TAG, "Sensor is not initialized!");
        return BME280UpdateStatusError;
    }

    while(bme280_is_measuring(bme280)) {
        if(furi_get_tick() - t > 100) {
            return BME280UpdateStatusTimeout;
        }
    }

    if(furi_get_tick() - bme280->last_cal_update_time > BOSCH_CAL_UPDATE_INTERVAL) {
        if(!bme280_read_calibration_values(bme280)) {
            FURI_LOG_E(TAG, "Failed to read calibration values");
            return BME280UpdateStatusError;
        }
    }

    if(!i2c_read_reg_array(bme280->device, 0xFA, 3, buff)) {
        return BME280UpdateStatusTimeout;
    }

    int32_t adc_T = ((int32_t)buff[0] << 12) | ((int32_t)buff[1] << 4) | ((int32_t)buff[2] >> 4);
    if(!i2c_read_reg_array(bme280->device, 0xF7, 3, buff)) {
        return BME280UpdateStatusTimeout;
    }

    int32_t adc_P = ((int32_t)buff[0] << 12) | ((int32_t)buff[1] << 4) | ((int32_t)buff[2] >> 4);
    if(!i2c_read_reg_array(bme280->device, 0xFD, 2, buff)) {
        return BME280UpdateStatusTimeout;
    }

    int32_t adc_H = ((uint16_t)buff[0] << 8) | buff[1];
    data->temperature = bme280_compensate_temperature(bme280, adc_T);
    data->pressure = bme280_compensate_pressure(bme280, adc_P);
    data->humidity = bme280_compensate_humidity(bme280, adc_H);
    return BME280UpdateStatusOK;
}

void bme280_free(BME280* bme280) {
    free(bme280);
}