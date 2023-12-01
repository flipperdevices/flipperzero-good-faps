#pragma once

#include "sensors/ICM42688P.h"

typedef struct ImuThread ImuThread;

typedef void (*ImuDataCallback)(uint32_t* data, void* context);

ImuThread* imu_start(ICM42688P* icm42688p, ImuDataCallback callback, void* context);

void imu_stop(ImuThread* imu);

void imu_reset_data(ImuThread* imu);
