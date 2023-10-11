#pragma once

#include "../sensor_module_i.h"

typedef struct SensorData SensorData;

typedef union {
    struct {
        float temperature;
        float humidity;
        float pressure;
        float altitude;
    } bme280_data;
    struct {
        float full_scale;
        float x;
        float y;
        float z;
    } gyro_accel_raw;
    struct {
        float x;
        float y;
        float z;
        float x_max;
        float y_max;
        float z_max;
        bool overflow;
    } mag_raw;
    struct {
        float q0;
        float q1;
        float q2;
        float q3;
        float roll;
        float pitch;
        float yaw;
    } imu_data;
} SensorDataValues;

typedef void (*SensorDataPageCallback)(SensorDataType type, void* context);

typedef void (*SensorDataKeyCallback)(SensorDataType data_type, InputKey key, void* context);

SensorData*
    sensor_data_alloc(SensorDataPageCallback page_cb, SensorDataKeyCallback key_cb, void* context);

void sensor_data_free(SensorData* sensor_data);

View* sensor_data_get_view(SensorData* sensor_data);

void sensor_data_set_values(SensorData* sensor_data, SensorDataType type, SensorDataValues* data);
