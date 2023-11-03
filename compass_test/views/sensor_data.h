#pragma once

#include "../compass_test_i.h"

typedef struct SensorData SensorData;

SensorData* sensor_data_alloc(void);

void sensor_data_free(SensorData* sensor_data);

View* sensor_data_get_view(SensorData* sensor_data);
