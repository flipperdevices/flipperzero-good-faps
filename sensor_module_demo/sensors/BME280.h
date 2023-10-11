#pragma once

#include "i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BME280 BME280;

typedef enum {
    BME280UpdateStatusOK,
    BME280UpdateStatusError,
    BME280UpdateStatusTimeout,
} BME280UpdateStatus;

typedef struct {
    float temperature;
    float humidity;
    float pressure;
} BME280Data;

BME280* bme280_alloc(I2CDevice* device);

bool bme280_init(BME280* bme280);

bool bme280_deinit(BME280* bme280);

BME280UpdateStatus bme280_update(BME280* bme280, BME280Data* data);

void bme280_free(BME280* bme280);

#ifdef __cplusplus
}
#endif