#include "sensor_data.h"
#include <furi.h>
#include <gui/elements.h>
#include "../sensor_module_i.h"
#include <locale/locale.h>

#define VALUES_X 0
#define VALUES_Y 12

struct SensorData {
    View* view;

    SensorDataPageCallback page_cb;
    SensorDataKeyCallback key_cb;
    void* context;
};

typedef enum {
    SensorPageBme280Env,
    SensorPageBme280Alt,
    SensorPageAccelRaw,
    SensorPageGyroRaw,
    SensorPageImu,
    SensorPageMagRaw,
    SensorPageMax,
} SensorDataPage;

typedef struct {
    bool data_ready;
    SensorDataPage page;
    SensorDataValues data;
} SensorDataModel;

static const char* page_name[SensorPageMax] = {
    [SensorPageBme280Env] = "BME280 Environment",
    [SensorPageBme280Alt] = "BME280 Altitude",
    [SensorPageAccelRaw] = "Accelerometer Raw",
    [SensorPageGyroRaw] = "Gyroscope Raw",
    [SensorPageImu] = "IMU 6dof",
    [SensorPageMagRaw] = "Magnetic Raw",

};

static SensorDataType get_page_data_type(SensorDataPage page) {
    switch(page) {
    case SensorPageBme280Env:
    case SensorPageBme280Alt:
        return SensorDataBME280;
    case SensorPageAccelRaw:
        return SensorDataAccelRaw;
    case SensorPageGyroRaw:
        return SensorDataGyroRaw;
    case SensorPageImu:
        return SensorDataIMU;
    case SensorPageMagRaw:
        return SensorDataMagRaw;
    default:
        break;
    }
    return SensorDataNone;
}

static void draw_bme280_env(Canvas* canvas, SensorDataModel* model) {
    FuriString* line_text = furi_string_alloc();
    if(model->data_ready) {
        if(locale_get_measurement_unit() == LocaleMeasurementUnitsMetric) {
            float temp = model->data.bme280_data.temperature;
            furi_string_printf(line_text, "Temp: %.1f  C", (double)temp);
        } else {
            float temp = locale_celsius_to_fahrenheit(model->data.bme280_data.temperature);
            furi_string_printf(line_text, "Temp: %.1f  F", (double)temp);
        }
        uint8_t sign_x =
            canvas_string_width(canvas, furi_string_get_cstr(line_text)) + VALUES_X - 8;
        canvas_draw_circle(canvas, sign_x, VALUES_Y, 1);

        furi_string_cat_printf(
            line_text, "\nHumidity: %.1f%%\n", (double)model->data.bme280_data.humidity);

        furi_string_cat_printf(
            line_text,
            "Pressure: \n    %.2fkPa/",
            (double)(model->data.bme280_data.pressure / 1000.f));
        if(locale_get_measurement_unit() == LocaleMeasurementUnitsMetric) {
            float pressure_mmhg = model->data.bme280_data.pressure / 101325.f * 760.f;
            furi_string_cat_printf(line_text, "%.0fmmHg", (double)pressure_mmhg);
        } else {
            float pressure_inhg = model->data.bme280_data.pressure / 101325.f * 29.92f;
            furi_string_cat_printf(line_text, "%.1finHg", (double)pressure_inhg);
        }

    } else {
        furi_string_printf(line_text, "Temp: ---\nHumidity: ---\nPressure:\n   ---");
    }
    elements_multiline_text_aligned(
        canvas, VALUES_X, VALUES_Y, AlignLeft, AlignTop, furi_string_get_cstr(line_text));
    furi_string_free(line_text);
}

static void draw_bme280_alt(Canvas* canvas, SensorDataModel* model) {
    FuriString* line_text = furi_string_alloc();
    if(model->data_ready) {
        furi_string_printf(
            line_text, "Pressure: %.0fPa\n", (double)(model->data.bme280_data.pressure));
        furi_string_cat_printf(
            line_text, "Alt: %.2fm", (double)(model->data.bme280_data.altitude));
    } else {
        furi_string_printf(line_text, "Pressure: ---\n");
        furi_string_cat_printf(line_text, "Alt: ---");
    }
    elements_multiline_text_aligned(
        canvas, VALUES_X, VALUES_Y, AlignLeft, AlignTop, furi_string_get_cstr(line_text));
    furi_string_free(line_text);
}

static void draw_accel_gyro_raw(Canvas* canvas, SensorDataModel* model) {
    char* unit_str = (model->page == SensorPageAccelRaw) ? "G" : "dps";
    FuriString* line_text = furi_string_alloc();
    if(model->data_ready) {
        furi_string_printf(
            line_text,
            "X: %.2f %s\nY: %.2f %s\nZ: %.2f %s\n",
            (double)(model->data.gyro_accel_raw.x),
            unit_str,
            (double)(model->data.gyro_accel_raw.y),
            unit_str,
            (double)(model->data.gyro_accel_raw.z),
            unit_str);
        furi_string_cat_printf(
            line_text,
            "Full Scale: %.3f %s",
            (double)(model->data.gyro_accel_raw.full_scale),
            unit_str);
    } else {
        furi_string_printf(line_text, "X: ---\nY: ---\nZ: ---\nFull Scale: ---");
    }
    elements_multiline_text_aligned(
        canvas, VALUES_X, VALUES_Y, AlignLeft, AlignTop, furi_string_get_cstr(line_text));
    furi_string_free(line_text);
}

static void draw_mag_raw(Canvas* canvas, SensorDataModel* model) {
    FuriString* line_text = furi_string_alloc();
    if(model->data_ready) {
        furi_string_printf(
            line_text,
            "X: %.2f %.2f\nY: %.2f %.2f\nZ: %.2f %.2f\nOverflow: %d",
            (double)(model->data.mag_raw.x),
            (double)(model->data.mag_raw.x_max),
            (double)(model->data.mag_raw.y),
            (double)(model->data.mag_raw.y_max),
            (double)(model->data.mag_raw.z),
            (double)(model->data.mag_raw.z_max),
            model->data.mag_raw.overflow);
    } else {
        furi_string_printf(line_text, "X: ---\nY: ---\nZ: ---");
    }
    elements_multiline_text_aligned(
        canvas, VALUES_X, VALUES_Y, AlignLeft, AlignTop, furi_string_get_cstr(line_text));
    furi_string_free(line_text);
}

static void draw_imu_data(Canvas* canvas, SensorDataModel* model) {
    FuriString* line_text = furi_string_alloc();
    if(model->data_ready) {
        furi_string_printf(
            line_text,
            "r: %.2f\np: %.2f\ny: %.2f",
            (double)(model->data.imu_data.roll),
            (double)(model->data.imu_data.pitch),
            (double)(model->data.imu_data.yaw));
    } else {
        furi_string_printf(line_text, "r: ---\np: ---\ny: ---");
    }
    elements_multiline_text_aligned(
        canvas, VALUES_X, VALUES_Y, AlignLeft, AlignTop, furi_string_get_cstr(line_text));
    furi_string_free(line_text);
}

static void sensor_data_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    SensorDataModel* model = context;
    furi_assert(model->page < SensorPageMax);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 0, 0, AlignLeft, AlignTop, page_name[model->page]);
    canvas_set_font(canvas, FontSecondary);
    if(model->page == SensorPageBme280Env) {
        draw_bme280_env(canvas, model);
    } else if(model->page == SensorPageBme280Alt) {
        draw_bme280_alt(canvas, model);
    } else if((model->page == SensorPageAccelRaw) || (model->page == SensorPageGyroRaw)) {
        draw_accel_gyro_raw(canvas, model);
    } else if(model->page == SensorPageImu) {
        draw_imu_data(canvas, model);
    } else if(model->page == SensorPageMagRaw) {
        draw_mag_raw(canvas, model);
    }
}

static bool sensor_data_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    SensorData* sensor_data = context;
    bool consumed = false;

    if((event->type == InputTypeShort) &&
       ((event->key == InputKeyRight) || (event->key == InputKeyLeft))) {
        SensorDataType data_type = SensorDataNone;
        with_view_model(
            sensor_data->view,
            SensorDataModel * model,
            {
                if(event->key == InputKeyRight) {
                    model->page = (model->page + 1) % SensorPageMax;
                } else {
                    model->page = ((model->page - 1) + SensorPageMax) % SensorPageMax;
                }
                model->data_ready = false;
                data_type = get_page_data_type(model->page);
            },
            true);
        if((data_type != SensorDataNone) && (sensor_data->page_cb)) {
            sensor_data->page_cb(data_type, sensor_data->context);
        }
        consumed = true;
    }

    if((event->type == InputTypeShort) &&
       ((event->key == InputKeyUp) || (event->key == InputKeyDown) ||
        (event->key == InputKeyOk))) {
        SensorDataType data_type = SensorDataNone;
        with_view_model(
            sensor_data->view,
            SensorDataModel * model,
            { data_type = get_page_data_type(model->page); },
            false);
        if(sensor_data->key_cb) {
            sensor_data->key_cb(data_type, event->key, sensor_data->context);
        }
    }

    return consumed;
}

SensorData*
    sensor_data_alloc(SensorDataPageCallback page_cb, SensorDataKeyCallback key_cb, void* context) {
    SensorData* sensor_data = malloc(sizeof(SensorData));
    sensor_data->page_cb = page_cb;
    sensor_data->key_cb = key_cb;
    sensor_data->context = context;
    sensor_data->view = view_alloc();
    view_set_context(sensor_data->view, sensor_data);
    view_allocate_model(sensor_data->view, ViewModelTypeLocking, sizeof(SensorDataModel));
    view_set_draw_callback(sensor_data->view, sensor_data_draw_callback);
    view_set_input_callback(sensor_data->view, sensor_data_input_callback);
    if(sensor_data->page_cb) {
        sensor_data->page_cb(SensorDataBME280, sensor_data->context);
    }
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

void sensor_data_set_values(SensorData* sensor_data, SensorDataType type, SensorDataValues* data) {
    furi_assert(sensor_data);
    with_view_model(
        sensor_data->view,
        SensorDataModel * model,
        {
            if(get_page_data_type(model->page) == type) {
                memcpy(&(model->data), data, sizeof(SensorDataValues));
                model->data_ready = true;
            }
        },
        true);
}
