#include "sensor_data.h"
#include <furi.h>
#include <gui/elements.h>
#include <locale/locale.h>

struct SensorData {
    View* view;
};

typedef struct {
    bool data_ready;
} SensorDataModel;

static void sensor_data_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    SensorDataModel* model = context;
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 0, 0, AlignLeft, AlignTop, "Compass Test");
    UNUSED(model);
}

static bool sensor_data_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    bool consumed = false;
    UNUSED(event);
    return consumed;
}

SensorData* sensor_data_alloc(void) {
    SensorData* sensor_data = malloc(sizeof(SensorData));
    sensor_data->view = view_alloc();
    view_set_context(sensor_data->view, sensor_data);
    view_allocate_model(sensor_data->view, ViewModelTypeLocking, sizeof(SensorDataModel));
    view_set_draw_callback(sensor_data->view, sensor_data_draw_callback);
    view_set_input_callback(sensor_data->view, sensor_data_input_callback);
    return sensor_data;
}

void sensor_data_free(SensorData* sensor_data) {
    furi_assert(sensor_data);
    view_free(sensor_data->view);
    free(sensor_data);
}

View* sensor_data_get_view(SensorData* sensor_data) {
    furi_assert(sensor_data);
    return sensor_data->view;
}
