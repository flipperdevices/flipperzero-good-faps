#include "compass_test_i.h"
#include "views/sensor_data.h"
#include <storage/storage.h>

#define TAG "CompassTest"

#define QMC5883P_I2C_ADDR (0x2C)
#define HMC5883L_I2C_ADDR (0x1E)
#define QMC5883L_I2C_ADDR (0x0D)

#define ACCEL_GYRO_RAW_RATE DataRate1kHz

#define TICK_PERIOD 10

struct CompassTestApp {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SensorData* sensor_data_view;

    I2CDevice* qmc5883p_device;
    I2CDevice* hmc5883l_device;
    I2CDevice* qmc5883l_device;
    FuriHalSpiBusHandle* icm42688p_device;

    QMC5883P* qmc5883p;
    HMC5883L* hmc5883l;
    QMC5883L* qmc5883l;
    ICM42688P* icm42688p;

    Storage* fs_api;
    File* file;
    uint32_t tick_nb;
    FuriString* csv_string;
};

static void data_record_start(CompassTestApp* app) {
    app->fs_api = furi_record_open(RECORD_STORAGE);
    app->file = storage_file_alloc(app->fs_api);
    furi_assert(storage_file_open(
        app->file,
        STORAGE_APP_DATA_PATH_PREFIX "/out.csv",
        FSAM_READ | FSAM_WRITE,
        FSOM_CREATE_ALWAYS));
    const char* csv_header = "tick;acc_x;acc_y;acc_z;mag_x;mag_y;mag_z\n";
    storage_file_write(app->file, csv_header, strlen(csv_header));

    app->csv_string = furi_string_alloc();
}

static void data_record_end(CompassTestApp* app) {
    storage_file_close(app->file);
    storage_file_free(app->file);
    furi_record_close(RECORD_STORAGE);

    furi_string_free(app->csv_string);
}

static void data_record_tick(CompassTestApp* app) {
    furi_string_printf(app->csv_string, "%lu;", app->tick_nb);
    ICM42688PRawData accel_raw;
    icm42688p_read_accel_raw(app->icm42688p, &accel_raw);
    furi_string_cat_printf(app->csv_string, "%d;%d;%d;", accel_raw.x, accel_raw.y, accel_raw.z);
    QMC5883PData mag_raw;
    if(qmc5883p_update(app->qmc5883p, &mag_raw) != QMC5883PUpdateStatusOK) {
        return;
    }
    // HMC5883LData mag_raw;
    // if(hmc5883l_update(app->hmc5883l, &mag_raw) != HMC5883LUpdateStatusOK) {
    //     return;
    // }
    // QMC5883LData mag_raw;
    // if(qmc5883l_update(app->qmc5883l, &mag_raw) != QMC5883LUpdateStatusOK) {
    //     return;
    // }
    furi_string_cat_printf(app->csv_string, "%d;%d;%d\n", mag_raw.x, mag_raw.y, mag_raw.z);

    const char* csv_line = furi_string_get_cstr(app->csv_string);
    storage_file_write(app->file, csv_line, strlen(csv_line));
    app->tick_nb++;
}

static uint32_t app_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static void compass_test_tick_callback(void* context) {
    furi_assert(context);
    CompassTestApp* app = context;
    data_record_tick(app);
    UNUSED(app);
}

static CompassTestApp* compass_test_alloc(void) {
    CompassTestApp* app = malloc(sizeof(CompassTestApp));
    bool show_error = false;

    app->qmc5883p_device = i2c_device_alloc(&furi_hal_i2c_handle_external, QMC5883P_I2C_ADDR << 1);
    app->qmc5883p = qmc5883p_alloc(app->qmc5883p_device);
    bool dev_valid = qmc5883p_init(app->qmc5883p);
    if(!dev_valid) {
        FURI_LOG_E(TAG, "Failed to initialize QMC5883P");
        show_error = true;
    }

    app->hmc5883l_device = i2c_device_alloc(&furi_hal_i2c_handle_external, HMC5883L_I2C_ADDR << 1);
    app->hmc5883l = hmc5883l_alloc(app->hmc5883l_device);
    dev_valid = hmc5883l_init(app->hmc5883l);
    if(!dev_valid) {
        FURI_LOG_E(TAG, "Failed to initialize HMC5883L");
        // show_error = true;
    }

    app->qmc5883l_device = i2c_device_alloc(&furi_hal_i2c_handle_external, QMC5883L_I2C_ADDR << 1);
    app->qmc5883l = qmc5883l_alloc(app->qmc5883l_device);
    dev_valid = qmc5883l_init(app->qmc5883l);
    if(!dev_valid) {
        FURI_LOG_E(TAG, "Failed to initialize QMC5883L");
        // show_error = true;
    }

    app->icm42688p_device = malloc(sizeof(FuriHalSpiBusHandle));
    memcpy(app->icm42688p_device, &furi_hal_spi_bus_handle_external, sizeof(FuriHalSpiBusHandle));
    app->icm42688p_device->cs = &gpio_ext_pc3;
    app->icm42688p = icm42688p_alloc(app->icm42688p_device, &gpio_ext_pb2);
    dev_valid = icm42688p_init(app->icm42688p);
    if(!dev_valid) {
        FURI_LOG_E(TAG, "Failed to initialize ICM42688P");
        show_error = true;
    } else {
        icm42688p_accel_config(app->icm42688p, AccelFullScale16G, ACCEL_GYRO_RAW_RATE);
        icm42688p_gyro_config(app->icm42688p, GyroFullScale2000DPS, ACCEL_GYRO_RAW_RATE);
    }

    app->gui = furi_record_open(RECORD_GUI);

    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);

    app->sensor_data_view = sensor_data_alloc();
    view_set_previous_callback(sensor_data_get_view(app->sensor_data_view), app_exit);
    view_dispatcher_add_view(
        app->view_dispatcher, SensorViewData, sensor_data_get_view(app->sensor_data_view));

    view_dispatcher_set_tick_event_callback(
        app->view_dispatcher, compass_test_tick_callback, TICK_PERIOD);

    if(show_error) {
        DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
        DialogMessage* message = dialog_message_alloc();
        dialog_message_set_header(message, "Sensor Module error", 63, 0, AlignCenter, AlignTop);
        dialog_message_set_text(message, "Module not conntected", 63, 30, AlignCenter, AlignTop);
        dialog_message_show(dialogs, message);
        dialog_message_free(message);
        furi_record_close(RECORD_DIALOGS);
        view_dispatcher_stop(app->view_dispatcher);

    } else {
        view_dispatcher_switch_to_view(app->view_dispatcher, SensorViewData);
    }

    return app;
}

static void compass_test_free(CompassTestApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, SensorViewData);
    sensor_data_free(app->sensor_data_view);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);

    if(!qmc5883p_deinit(app->qmc5883p)) {
        FURI_LOG_E(TAG, "Failed to deinitialize QMC5883P");
    }

    if(!hmc5883l_deinit(app->hmc5883l)) {
        FURI_LOG_E(TAG, "Failed to deinitialize HMC5883L");
    }

    if(!qmc5883l_deinit(app->qmc5883l)) {
        FURI_LOG_E(TAG, "Failed to deinitialize QMC5883L");
    }

    if(!icm42688p_deinit(app->icm42688p)) {
        FURI_LOG_E(TAG, "Failed to deinitialize ICM42688P");
    }

    qmc5883p_free(app->qmc5883p);
    hmc5883l_free(app->hmc5883l);
    qmc5883l_free(app->qmc5883l);
    icm42688p_free(app->icm42688p);
    i2c_device_free(app->qmc5883p_device);
    i2c_device_free(app->hmc5883l_device);
    i2c_device_free(app->qmc5883l_device);
    free(app->icm42688p_device);

    free(app);
}

int32_t compass_test_app(void* arg) {
    UNUSED(arg);
    CompassTestApp* app = compass_test_alloc();

    data_record_start(app);

    view_dispatcher_run(app->view_dispatcher);

    data_record_end(app);
    compass_test_free(app);
    return 0;
}