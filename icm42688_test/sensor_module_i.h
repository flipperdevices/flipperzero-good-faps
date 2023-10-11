#pragma once

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <dialogs/dialogs.h>

#include "sensors/ICM42688P.h"

typedef enum {
    SensorViewData,
} SensorView;

typedef enum {
    SensorDataNone,
    SensorDataBME280,
    SensorDataGyroRaw,
    SensorDataAccelRaw,
    SensorDataIMU,
    SensorDataMagRaw,
} SensorDataType;

typedef struct SensorModuleApp SensorModuleApp;
