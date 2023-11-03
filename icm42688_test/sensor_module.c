#include "sensor_module_i.h"

#define TAG "GyroAccelTest"

#define ACCEL_GYRO_RAW_RATE DataRate1kHz

int32_t sensor_module_app(void* arg) {
    UNUSED(arg);

    DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
    DialogMessage* message = dialog_message_alloc();
    dialog_message_set_header(message, "Sensor Module test", 63, 0, AlignCenter, AlignTop);
    FuriString* msg_string = furi_string_alloc();

    FuriHalSpiBusHandle* icm42688p_device = malloc(sizeof(FuriHalSpiBusHandle));
    memcpy(icm42688p_device, &furi_hal_spi_bus_handle_external, sizeof(FuriHalSpiBusHandle));
    icm42688p_device->cs = &gpio_ext_pc3;
    ICM42688P* icm42688p = icm42688p_alloc(icm42688p_device, &gpio_ext_pb2);
    bool icm42688p_valid = icm42688p_init(icm42688p);
    if(!icm42688p_valid) {
        FURI_LOG_E(TAG, "Failed to initialize ICM42688P");
        dialog_message_set_text(message, "ICM42688P not found", 63, 30, AlignCenter, AlignTop);
    } else {
        icm42688p_accel_config(icm42688p, AccelFullScale16G, ACCEL_GYRO_RAW_RATE);
        icm42688p_gyro_config(icm42688p, GyroFullScale2000DPS, ACCEL_GYRO_RAW_RATE);
        bool gyro_ok, accel_ok;
        icm42688_selftest(icm42688p, &gyro_ok, &accel_ok);
        furi_string_printf(msg_string, "Gyro %s\n", gyro_ok ? "OK" : "FAIL");
        furi_string_cat_printf(msg_string, "Accel %s", accel_ok ? "OK" : "FAIL");
        dialog_message_set_text(
            message, furi_string_get_cstr(msg_string), 63, 20, AlignCenter, AlignTop);
    }

    if(!icm42688p_deinit(icm42688p)) {
        FURI_LOG_E(TAG, "Failed to deinitialize ICM42688P");
    }

    icm42688p_free(icm42688p);
    free(icm42688p_device);

    dialog_message_show(dialogs, message);
    dialog_message_free(message);
    furi_record_close(RECORD_DIALOGS);

    furi_string_free(msg_string);

    return 0;
}