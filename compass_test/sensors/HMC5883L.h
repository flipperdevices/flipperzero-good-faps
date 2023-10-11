#pragma once

#include "i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HMC5883L HMC5883L;

typedef enum {
    HMC5883LUpdateStatusOK,
    HMC5883LUpdateStatusError,
    HMC5883LUpdateStatusTimeout,
} HMC5883LUpdateStatus;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} HMC5883LData;

HMC5883L* hmc5883l_alloc(I2CDevice* device);

bool hmc5883l_init(HMC5883L* hmc5883l);

bool hmc5883l_deinit(HMC5883L* hmc5883l);

HMC5883LUpdateStatus hmc5883l_update(HMC5883L* hmc5883l, HMC5883LData* result);

void hmc5883l_free(HMC5883L* hmc5883l);

#ifdef __cplusplus
}
#endif