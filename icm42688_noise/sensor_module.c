#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <dialogs/dialogs.h>
#include "imu_thread.h"

#define TAG "GyroAccelTest"

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriMessageQueue* input_queue;
    FuriMutex** model_mutex;

    FuriHalSpiBusHandle* icm42688p_device;
    ICM42688P* icm42688p;
    bool icm42688p_valid;

    ImuThread* imu_thread;

    uint32_t diff_data[16];
} SensorModuleApp;

void imu_data_callback(uint32_t* data, void* context) {
    SensorModuleApp* app = context;
    furi_assert(app);

    furi_check(furi_mutex_acquire(app->model_mutex, FuriWaitForever) == FuriStatusOk);
    memcpy(app->diff_data, data, sizeof(app->diff_data));
    furi_mutex_release(app->model_mutex);
    view_port_update(app->view_port);
}

static void render_callback(Canvas* canvas, void* ctx) {
    SensorModuleApp* app = ctx;
    furi_assert(app);

    furi_check(furi_mutex_acquire(app->model_mutex, FuriWaitForever) == FuriStatusOk);

    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontSecondary);

    FuriString* line_text = furi_string_alloc();

    elements_button_center(canvas, "Reset");

    furi_string_printf(
        line_text,
        "Accel:\nx: %lu\ny: %lu\nz: %lu",
        app->diff_data[0],
        app->diff_data[1],
        app->diff_data[2]);
    elements_multiline_text_aligned(
        canvas, 0, 0, AlignLeft, AlignTop, furi_string_get_cstr(line_text));

    furi_string_printf(
        line_text,
        "Gyro:\nx: %lu\ny: %lu\nz: %lu",
        app->diff_data[3],
        app->diff_data[4],
        app->diff_data[5]);
    elements_multiline_text_aligned(
        canvas, 64, 0, AlignLeft, AlignTop, furi_string_get_cstr(line_text));
    furi_string_free(line_text);

    furi_mutex_release(app->model_mutex);
}

static void input_callback(InputEvent* input_event, void* ctx) {
    SensorModuleApp* app = ctx;
    furi_message_queue_put(app->input_queue, input_event, 0);
}

static SensorModuleApp* sensor_module_alloc(void) {
    SensorModuleApp* app = malloc(sizeof(SensorModuleApp));

    app->icm42688p_device = malloc(sizeof(FuriHalSpiBusHandle));
    memcpy(app->icm42688p_device, &furi_hal_spi_bus_handle_external, sizeof(FuriHalSpiBusHandle));
    app->icm42688p_device->cs = &gpio_ext_pc3;
    app->icm42688p = icm42688p_alloc(app->icm42688p_device, &gpio_ext_pb2);
    app->icm42688p_valid = icm42688p_init(app->icm42688p);

    app->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->model_mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, render_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);

    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    return app;
}

static void sensor_module_free(SensorModuleApp* app) {
    if(app->imu_thread) {
        imu_stop(app->imu_thread);
        app->imu_thread = NULL;
    }

    gui_remove_view_port(app->gui, app->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app->view_port);
    furi_message_queue_free(app->input_queue);
    furi_mutex_free(app->model_mutex);

    if(!icm42688p_deinit(app->icm42688p)) {
        FURI_LOG_E(TAG, "Failed to deinitialize ICM42688P");
    }

    icm42688p_free(app->icm42688p);
    free(app->icm42688p_device);

    free(app);
}

int32_t sensor_module_app(void* arg) {
    UNUSED(arg);
    SensorModuleApp* app = sensor_module_alloc();

    if(!app->icm42688p_valid) {
        DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
        DialogMessage* message = dialog_message_alloc();
        dialog_message_set_header(message, "Sensor Module error", 63, 0, AlignCenter, AlignTop);

        dialog_message_set_text(message, "Module not conntected", 63, 30, AlignCenter, AlignTop);
        dialog_message_show(dialogs, message);
        dialog_message_free(message);
        furi_record_close(RECORD_DIALOGS);

        sensor_module_free(app);
        return 0;
    }

    app->imu_thread = imu_start(app->icm42688p, imu_data_callback, app);

    while(1) {
        InputEvent input;
        if(furi_message_queue_get(app->input_queue, &input, FuriWaitForever) == FuriStatusOk) {
            if((input.key == InputKeyBack) && (input.type == InputTypeShort)) {
                break;
            } else if((input.key == InputKeyOk) && (input.type == InputTypeShort)) {
                furi_delay_ms(1000);
                imu_reset_data(app->imu_thread);
            }
        }
    }

    sensor_module_free(app);
    return 0;
}