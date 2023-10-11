#pragma once

#include "i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct QMC5883L QMC5883L;

typedef enum {
    QMC5883LUpdateStatusOK,
    QMC5883LUpdateStatusError,
    QMC5883LUpdateStatusTimeout,
} QMC5883LUpdateStatus;

typedef struct {
    float x;
    float y;
    float z;
    bool overflow;
} QMC5883LData;

QMC5883L* qmc5883l_alloc(I2CDevice* device);

bool qmc5883l_init(QMC5883L* qmc5883l);

bool qmc5883l_deinit(QMC5883L* qmc5883l);

QMC5883LUpdateStatus qmc5883l_update(QMC5883L* qmc5883l, QMC5883LData* result);

void qmc5883l_free(QMC5883L* qmc5883l);

#ifdef __cplusplus
}
#endif