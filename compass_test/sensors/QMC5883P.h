#pragma once

#include "i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct QMC5883P QMC5883P;

typedef enum {
    QMC5883PUpdateStatusOK,
    QMC5883PUpdateStatusError,
    QMC5883PUpdateStatusTimeout,
} QMC5883PUpdateStatus;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
    bool overflow;
} QMC5883PData;

QMC5883P* qmc5883p_alloc(I2CDevice* device);

bool qmc5883p_init(QMC5883P* qmc5883p);

bool qmc5883p_deinit(QMC5883P* qmc5883p);

QMC5883PUpdateStatus qmc5883p_update(QMC5883P* qmc5883p, QMC5883PData* result);

void qmc5883p_free(QMC5883P* qmc5883p);

#ifdef __cplusplus
}
#endif