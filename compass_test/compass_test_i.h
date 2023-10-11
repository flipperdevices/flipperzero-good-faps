#pragma once

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <dialogs/dialogs.h>

#include "sensors/i2c.h"
#include "sensors/BME280.h"
#include "sensors/QMC5883P.h"
#include "sensors/ICM42688P.h"
#include "sensors/HMC5883L.h"
#include "sensors/QMC5883L.h"

typedef enum {
    SensorViewData,
} SensorView;

typedef struct CompassTestApp CompassTestApp;
