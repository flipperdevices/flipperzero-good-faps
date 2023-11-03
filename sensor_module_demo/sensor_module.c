#include "sensor_module_i.h"
#include "views/sensor_data.h"
#include "imu.h"

#define TAG "SensorModule"

#define BME280_I2C_ADDR (0x76)
#define QMC5883P_I2C_ADDR (0x2C)

#define SEA_LEVEL_PRESSURE (101325.f)

#define ACCEL_GYRO_RAW_RATE DataRate1kHz

struct SensorModuleApp {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SensorData* sensor_data_view;

    I2CDevice* bme280_device;
    I2CDevice* qmc5883p_device;
    FuriHalSpiBusHandle* icm42688p_device;

    BME280* bme280;
    QMC5883P* qmc5883p;
    ICM42688P* icm42688p;

    bool bme280_valid;
    bool qmc5883p_valid;
    bool icm42688p_valid;

    SensorDataType sensor_type_current;

    ICM42688PAccelFullScale accel_scale;
    ICM42688PGyroFullScale gyro_scale;

    float x_min;
    float x_max;
    float y_min;
    float y_max;
    float z_min;
    float z_max;

    ImuThread* imu_thread;
};

static uint32_t app_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static float get_altitude(float pressure) {
    return (1.0f - powf(pressure / SEA_LEVEL_PRESSURE, 0.190295f)) * 44330.0f;
}

static void sensor_module_imu_callback(ImuProcessedData* data, void* context) {
    furi_assert(context);
    SensorModuleApp* app = context;
    SensorDataValues display_data;
    display_data.imu_data.roll = data->roll;
    display_data.imu_data.pitch = data->pitch;
    display_data.imu_data.yaw = data->yaw;
    sensor_data_set_values(app->sensor_data_view, SensorDataIMU, &display_data);
}

static void sensor_module_tick_callback(void* context) {
    furi_assert(context);
    SensorModuleApp* app = context;
    if((app->sensor_type_current == SensorDataBME280) && (app->bme280_valid)) {
        SensorDataValues data;
        BME280Data bme_data;
        bme280_update(app->bme280, &bme_data);
        data.bme280_data.temperature = bme_data.temperature;
        data.bme280_data.humidity = bme_data.humidity;
        data.bme280_data.pressure = bme_data.pressure;
        data.bme280_data.altitude = get_altitude(bme_data.pressure);
        sensor_data_set_values(app->sensor_data_view, SensorDataBME280, &data);
    } else if((app->sensor_type_current == SensorDataAccelRaw) && (app->icm42688p_valid)) {
        SensorDataValues data;
        data.gyro_accel_raw.full_scale = icm42688p_accel_get_full_scale(app->icm42688p);
        ICM42688PRawData raw_data;
        ICM42688PScaledData scaled_data;
        icm42688p_read_accel_raw(app->icm42688p, &raw_data);
        icm42688p_apply_scale(&raw_data, data.gyro_accel_raw.full_scale, &scaled_data);
        data.gyro_accel_raw.x = scaled_data.x;
        data.gyro_accel_raw.y = scaled_data.y;
        data.gyro_accel_raw.z = scaled_data.z;
        sensor_data_set_values(app->sensor_data_view, SensorDataAccelRaw, &data);
    } else if((app->sensor_type_current == SensorDataGyroRaw) && (app->icm42688p_valid)) {
        SensorDataValues data;
        data.gyro_accel_raw.full_scale = icm42688p_gyro_get_full_scale(app->icm42688p);
        ICM42688PRawData raw_data;
        ICM42688PScaledData scaled_data;
        icm42688p_read_gyro_raw(app->icm42688p, &raw_data);
        icm42688p_apply_scale(&raw_data, data.gyro_accel_raw.full_scale, &scaled_data);
        data.gyro_accel_raw.x = scaled_data.x;
        data.gyro_accel_raw.y = scaled_data.y;
        data.gyro_accel_raw.z = scaled_data.z;
        sensor_data_set_values(app->sensor_data_view, SensorDataGyroRaw, &data);
    } else if((app->sensor_type_current == SensorDataIMU) && (app->imu_thread)) {
        imu_get_data(app->imu_thread, sensor_module_imu_callback, app);
    } else if((app->sensor_type_current == SensorDataMagRaw) && (app->qmc5883p_valid)) {
        QMC5883PData raw_data;
        if(qmc5883p_update(app->qmc5883p, &raw_data) == QMC5883PUpdateStatusOK) {
            SensorDataValues data;
            if(app->x_max < raw_data.x) {
                app->x_max = raw_data.x;
            }
            if(app->x_min > raw_data.x) {
                app->x_min = raw_data.x;
            }
            if(app->y_max < raw_data.y) {
                app->y_max = raw_data.y;
            }
            if(app->y_min > raw_data.y) {
                app->y_min = raw_data.y;
            }
            if(app->z_max < raw_data.z) {
                app->z_max = raw_data.z;
            }
            if(app->z_min > raw_data.z) {
                app->z_min = raw_data.z;
            }

            float heading = atan2f(raw_data.x, raw_data.y);
            if(heading < 0.f) {
                heading += 2.f * M_PI;
            }
            if(heading > 2.f * M_PI) {
                heading -= 2.f * M_PI;
            }

            data.mag_raw.x = heading * 180.f / M_PI;
            data.mag_raw.y = app->y_min;
            data.mag_raw.z = app->z_min;
            data.mag_raw.x_max = app->x_max;
            data.mag_raw.y_max = app->y_max;
            data.mag_raw.z_max = app->z_max;
            data.mag_raw.overflow = raw_data.overflow;
            sensor_data_set_values(app->sensor_data_view, SensorDataMagRaw, &data);
        }
    }
}

static bool sensor_module_change_page_callback(SensorDataType type, void* context) {
    furi_assert(context);
    SensorModuleApp* app = context;

    if((app->imu_thread) && (type != SensorDataIMU)) {
        imu_stop(app->imu_thread);
        app->imu_thread = NULL;
    }

    if(!app->bme280_valid) {
        if(type == SensorDataBME280) {
            return false;
        }
    }
    if(!app->icm42688p_valid) {
        if((type == SensorDataGyroRaw) || (type == SensorDataAccelRaw) ||
           (type == SensorDataIMU)) {
            return false;
        }
    }
    if(!app->qmc5883p_valid) {
        if(type == SensorDataMagRaw) {
            return false;
        }
    }

    app->sensor_type_current = type;
    if(type == SensorDataIMU) {
        app->imu_thread = imu_start(app->icm42688p);
    }
    return true;
}

#define IMU_CALI_AVG 64

static void calibrate_gyro(SensorModuleApp* app) {
    ICM42688PRawData data;
    ICM42688PScaledData offset_scaled = {.x = 0.f, .y = 0.f, .z = 0.f};

    icm42688p_write_gyro_offset(app->icm42688p, &offset_scaled);
    furi_delay_ms(10);

    int32_t avg_x = 0;
    int32_t avg_y = 0;
    int32_t avg_z = 0;

    for(uint8_t i = 0; i < IMU_CALI_AVG; i++) {
        icm42688p_read_gyro_raw(app->icm42688p, &data);
        avg_x += data.x;
        avg_y += data.y;
        avg_z += data.z;
        furi_delay_ms(2);
    }

    data.x = avg_x / IMU_CALI_AVG;
    data.y = avg_y / IMU_CALI_AVG;
    data.z = avg_z / IMU_CALI_AVG;

    icm42688p_apply_scale(&data, icm42688p_gyro_get_full_scale(app->icm42688p), &offset_scaled);
    FURI_LOG_I(
        TAG,
        "Offsets: x %f, y %f, z %f",
        (double)offset_scaled.x,
        (double)offset_scaled.y,
        (double)offset_scaled.z);
    icm42688p_write_gyro_offset(app->icm42688p, &offset_scaled);

    // TODO: save offsets to file
}

static void
    sensor_module_key_press_callback(SensorDataType data_type, InputKey key, void* context) {
    furi_assert(context);
    SensorModuleApp* app = context;
    if((data_type == SensorDataAccelRaw) && (app->icm42688p_valid)) {
        if(key == InputKeyUp) {
            app->accel_scale =
                ((app->accel_scale - 1) + AccelFullScaleTotal) % AccelFullScaleTotal;
        } else if(key == InputKeyDown) {
            app->accel_scale = (app->accel_scale + 1) % AccelFullScaleTotal;
        }
        icm42688p_accel_config(app->icm42688p, app->accel_scale, ACCEL_GYRO_RAW_RATE);
    } else if((data_type == SensorDataGyroRaw) && (app->icm42688p_valid)) {
        if(key == InputKeyUp) {
            app->gyro_scale = ((app->gyro_scale - 1) + GyroFullScaleTotal) % GyroFullScaleTotal;
            icm42688p_gyro_config(app->icm42688p, app->gyro_scale, ACCEL_GYRO_RAW_RATE);
        } else if(key == InputKeyDown) {
            app->gyro_scale = (app->gyro_scale + 1) % GyroFullScaleTotal;
            icm42688p_gyro_config(app->icm42688p, app->gyro_scale, ACCEL_GYRO_RAW_RATE);
        } else if(key == InputKeyOk) {
            calibrate_gyro(app);
        }
    }
}

static SensorModuleApp* sensor_module_alloc(void) {
    SensorModuleApp* app = malloc(sizeof(SensorModuleApp));
    bool show_error = false;

    app->bme280_device = i2c_device_alloc(&furi_hal_i2c_handle_external, BME280_I2C_ADDR << 1);
    app->bme280 = bme280_alloc(app->bme280_device);
    app->bme280_valid = bme280_init(app->bme280);
    if(!app->bme280_valid) {
        FURI_LOG_E(TAG, "Failed to initialize BME280");
        show_error = true;
    }

    app->qmc5883p_device = i2c_device_alloc(&furi_hal_i2c_handle_external, QMC5883P_I2C_ADDR << 1);
    app->qmc5883p = qmc5883p_alloc(app->qmc5883p_device);
    app->qmc5883p_valid = qmc5883p_init(app->qmc5883p);
    if(!app->qmc5883p_valid) {
        FURI_LOG_E(TAG, "Failed to initialize QMC5883P");
        show_error = true;
    }

    app->icm42688p_device = malloc(sizeof(FuriHalSpiBusHandle));
    memcpy(app->icm42688p_device, &furi_hal_spi_bus_handle_external, sizeof(FuriHalSpiBusHandle));
    app->icm42688p_device->cs = &gpio_ext_pc3;
    app->icm42688p = icm42688p_alloc(app->icm42688p_device, &gpio_ext_pb2);
    app->icm42688p_valid = icm42688p_init(app->icm42688p);
    if(!app->icm42688p_valid) {
        FURI_LOG_E(TAG, "Failed to initialize ICM42688P");
        show_error = true;
    } else {
        app->accel_scale = AccelFullScale16G;
        app->gyro_scale = GyroFullScale2000DPS;
        icm42688p_accel_config(app->icm42688p, app->accel_scale, ACCEL_GYRO_RAW_RATE);
        icm42688p_gyro_config(app->icm42688p, app->gyro_scale, ACCEL_GYRO_RAW_RATE);
    }

    app->gui = furi_record_open(RECORD_GUI);

    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);

    app->sensor_data_view = sensor_data_alloc(
        sensor_module_change_page_callback, sensor_module_key_press_callback, app);
    view_set_previous_callback(sensor_data_get_view(app->sensor_data_view), app_exit);
    view_dispatcher_add_view(
        app->view_dispatcher, SensorViewData, sensor_data_get_view(app->sensor_data_view));

    view_dispatcher_set_tick_event_callback(
        app->view_dispatcher, sensor_module_tick_callback, 250);

    if(show_error) {
        DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
        DialogMessage* message = dialog_message_alloc();
        dialog_message_set_header(message, "Sensor Module error", 63, 0, AlignCenter, AlignTop);
        if((!app->bme280_valid) && (!app->qmc5883p_valid) && (!app->icm42688p_valid)) {
            dialog_message_set_text(
                message, "Module not conntected", 63, 30, AlignCenter, AlignTop);
            dialog_message_show(dialogs, message);
            dialog_message_free(message);
            furi_record_close(RECORD_DIALOGS);
            view_dispatcher_stop(app->view_dispatcher);
        } else {
            FuriString* msg_text = furi_string_alloc();
            if(!app->bme280_valid) {
                furi_string_cat(msg_text, "BME280 not found\n");
            }
            if(!app->qmc5883p_valid) {
                furi_string_cat(msg_text, "QMC5883 not found\n");
            }
            if(!app->icm42688p_valid) {
                furi_string_cat(msg_text, "ICM42688 not found\n");
            }
            dialog_message_set_text(
                message, furi_string_get_cstr(msg_text), 63, 30, AlignCenter, AlignTop);
            dialog_message_show(dialogs, message);
            dialog_message_free(message);
            furi_record_close(RECORD_DIALOGS);
            furi_string_free(msg_text);
            view_dispatcher_switch_to_view(app->view_dispatcher, SensorViewData);
        }
    } else {
        view_dispatcher_switch_to_view(app->view_dispatcher, SensorViewData);
    }

    return app;
}

static void sensor_module_free(SensorModuleApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, SensorViewData);
    sensor_data_free(app->sensor_data_view);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);

    if(app->imu_thread) {
        imu_stop(app->imu_thread);
        app->imu_thread = NULL;
    }

    if(!bme280_deinit(app->bme280)) {
        FURI_LOG_E(TAG, "Failed to deinitialize BME280");
    }

    if(!qmc5883p_deinit(app->qmc5883p)) {
        FURI_LOG_E(TAG, "Failed to deinitialize QMC5883P");
    }

    if(!icm42688p_deinit(app->icm42688p)) {
        FURI_LOG_E(TAG, "Failed to deinitialize ICM42688P");
    }

    qmc5883p_free(app->qmc5883p);
    bme280_free(app->bme280);
    icm42688p_free(app->icm42688p);
    i2c_device_free(app->bme280_device);
    i2c_device_free(app->qmc5883p_device);
    free(app->icm42688p_device);

    free(app);
}

int32_t sensor_module_app(void* arg) {
    UNUSED(arg);
    SensorModuleApp* app = sensor_module_alloc();

    view_dispatcher_run(app->view_dispatcher);
    sensor_module_free(app);
    return 0;
}