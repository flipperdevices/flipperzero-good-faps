#pragma once

typedef struct {
    float q0;
    float q1;
    float q2;
    float q3;
    float roll;
    float pitch;
    float yaw;
} ImuProcessedData;

typedef struct ImuThread ImuThread;

typedef void (*ImuDataCallback)(ImuProcessedData* data, void* context);

void imu_get_data(ImuThread* imu, ImuDataCallback callback, void* context);

ImuThread* imu_start(ICM42688P* icm42688p);

void imu_stop(ImuThread* imu);
