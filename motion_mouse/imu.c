#include "imu.h"
#include <furi_hal.h>
#include <furi.h>
#include "sensors/ICM42688P.h"

#define TAG "IMU"

#define FILTER_SAMPLE_FREQ 200.f
#define FILTER_BETA 0.1f

#define SENSITIVITY_K 30.f

#define IMU_CALI_AVG 64

typedef enum {
    ImuThreadStop = (1 << 0),
    ImuThreadNewData = (1 << 1),
    ImuThreadGetData = (1 << 2),
} ImuThreadFlags;

struct ImuThread {
    FuriThread* thread;
    ICM42688P* icm42688p;

    ImuProcessedData processed_data;
    ImuDataCallback data_callback;
    void* data_context;
};

static void imu_madgwick_filter(
    ImuProcessedData* out,
    ICM42688PScaledData* accel,
    ICM42688PScaledData* gyro);

static void imu_irq_callback(void* context) {
    furi_assert(context);
    ImuThread* imu = context;
    furi_thread_flags_set(furi_thread_get_id(imu->thread), ImuThreadNewData);
}

static void imu_process_data(ImuThread* imu, ICM42688PFifoPacket* in_data) {
    ICM42688PScaledData accel_data;
    ICM42688PScaledData gyro_data;

    // Get accel and gyro data in g and degrees/s
    icm42688p_apply_scale_fifo(imu->icm42688p, in_data, &accel_data, &gyro_data);

    // Gyro: degrees/s to rads/s
    gyro_data.x = gyro_data.x / 180.f * M_PI;
    gyro_data.y = gyro_data.y / 180.f * M_PI;
    gyro_data.z = gyro_data.z / 180.f * M_PI;

    // Sensor Fusion algorithm
    ImuProcessedData* out = &imu->processed_data;
    imu_madgwick_filter(out, &accel_data, &gyro_data);

    // Quaternion to euler angles
    float roll = atan2f(
        out->q0 * out->q1 + out->q2 * out->q3, 0.5f - out->q1 * out->q1 - out->q2 * out->q2);
    float pitch = asinf(-2.0f * (out->q1 * out->q3 - out->q0 * out->q2));
    float yaw = atan2f(
        out->q1 * out->q2 + out->q0 * out->q3, 0.5f - out->q2 * out->q2 - out->q3 * out->q3);

    // Euler angles: rads to degrees
    out->roll = roll / M_PI * 180.f;
    out->pitch = pitch / M_PI * 180.f;
    out->yaw = yaw / M_PI * 180.f;
}

static void calibrate_gyro(ImuThread* imu) {
    ICM42688PRawData data;
    ICM42688PScaledData offset_scaled = {.x = 0.f, .y = 0.f, .z = 0.f};

    icm42688p_write_gyro_offset(imu->icm42688p, &offset_scaled);
    furi_delay_ms(10);

    int32_t avg_x = 0;
    int32_t avg_y = 0;
    int32_t avg_z = 0;

    for(uint8_t i = 0; i < IMU_CALI_AVG; i++) {
        icm42688p_read_gyro_raw(imu->icm42688p, &data);
        avg_x += data.x;
        avg_y += data.y;
        avg_z += data.z;
        furi_delay_ms(2);
    }

    data.x = avg_x / IMU_CALI_AVG;
    data.y = avg_y / IMU_CALI_AVG;
    data.z = avg_z / IMU_CALI_AVG;

    icm42688p_apply_scale(&data, icm42688p_gyro_get_full_scale(imu->icm42688p), &offset_scaled);
    FURI_LOG_I(
        TAG,
        "Offsets: x %f, y %f, z %f",
        (double)offset_scaled.x,
        (double)offset_scaled.y,
        (double)offset_scaled.z);
    icm42688p_write_gyro_offset(imu->icm42688p, &offset_scaled);

    // TODO: save offsets to file
}

static float imu_angle_diff(float a, float b) {
    float diff = a - b;
    if(diff > 180.f)
        diff -= 360.f;
    else if(diff < -180.f)
        diff += 360.f;

    return diff;
}

static int32_t imu_thread(void* context) {
    furi_assert(context);
    ImuThread* imu = context;
    FURI_LOG_I(TAG, "Start");

    float yaw_last = 0.f;
    float pitch_last = 0.f;
    float diff_x = 0.f;
    float diff_y = 0.f;

    FuriHalUsbInterface* usb_mode_prev = furi_hal_usb_get_config();
    furi_hal_usb_set_config(&usb_hid, NULL);

    calibrate_gyro(imu);

    imu->processed_data.q0 = 1.f;
    imu->processed_data.q1 = 0.f;
    imu->processed_data.q2 = 0.f;
    imu->processed_data.q3 = 0.f;
    icm42688_fifo_enable(imu->icm42688p, imu_irq_callback, imu);

    while(1) {
        uint32_t events = furi_thread_flags_wait(
            ImuThreadStop | ImuThreadNewData | ImuThreadGetData, FuriFlagWaitAny, FuriWaitForever);

        if(events & ImuThreadStop) {
            break;
        }

        if(events & ImuThreadNewData) {
            uint16_t data_pending = icm42688_fifo_get_count(imu->icm42688p);
            ICM42688PFifoPacket data;
            while(data_pending--) {
                icm42688_fifo_read(imu->icm42688p, &data);
                imu_process_data(imu, &data);
                if((imu->processed_data.pitch > -75.f) && (imu->processed_data.pitch < 75.f) &&
                   (isfinite(imu->processed_data.pitch) != 0)) {
                    int8_t mouse_x = 0;
                    int8_t mouse_y = 0;

                    diff_x += imu_angle_diff(yaw_last, imu->processed_data.yaw) * SENSITIVITY_K;
                    if(diff_x < -127.f) {
                        mouse_x = -127;
                    } else if(diff_x > 127.f) {
                        mouse_x = 127;
                    } else {
                        mouse_x = (int8_t)diff_x;
                    }
                    diff_x -= (float)mouse_x;
                    yaw_last = imu->processed_data.yaw;

                    diff_y +=
                        imu_angle_diff(pitch_last, -imu->processed_data.pitch) * SENSITIVITY_K;
                    if(diff_y < -127.f) {
                        mouse_y = -127;
                    } else if(diff_y > 127.f) {
                        mouse_y = 127;
                    } else {
                        mouse_y = (int8_t)diff_y;
                    }
                    diff_y -= (float)mouse_y;
                    pitch_last = -imu->processed_data.pitch;

                    furi_hal_hid_mouse_move(mouse_x, mouse_y);
                }
            }
        }

        if(events & ImuThreadGetData) {
            if(imu->data_callback) {
                imu->data_callback(&(imu->processed_data), imu->data_context);
            }
        }
    }

    furi_hal_usb_set_config(usb_mode_prev, NULL);

    icm42688_fifo_disable(imu->icm42688p);
    FURI_LOG_I(TAG, "End");

    return 0;
}

void imu_get_data(ImuThread* imu, ImuDataCallback callback, void* context) {
    furi_assert(imu);
    imu->data_callback = callback;
    imu->data_context = context;
    furi_thread_flags_set(furi_thread_get_id(imu->thread), ImuThreadGetData);
}

ImuThread* imu_start(ICM42688P* icm42688p) {
    ImuThread* imu = malloc(sizeof(ImuThread));
    imu->icm42688p = icm42688p;
    imu->thread = furi_thread_alloc_ex("ImuThread", 4096, imu_thread, imu);
    furi_thread_start(imu->thread);

    return imu;
}

void imu_stop(ImuThread* imu) {
    furi_assert(imu);

    furi_thread_flags_set(furi_thread_get_id(imu->thread), ImuThreadStop);

    furi_thread_join(imu->thread);
    furi_thread_free(imu->thread);

    free(imu);
}

/* Simple madgwik filter, based on: https://github.com/arduino-libraries/MadgwickAHRS/*/

static float imu_inv_sqrt(float x) {
    /* close-to-optimal method with low cost from http://pizer.wordpress.com/2008/10/12/fast-inverse-square-root */
    unsigned int i = 0x5F1F1412 - (*(unsigned int*)&x >> 1);
    float tmp = *(float*)&i;
    return tmp * (1.69000231f - 0.714158168f * x * tmp * tmp);
}

static void imu_madgwick_filter(
    ImuProcessedData* out,
    ICM42688PScaledData* accel,
    ICM42688PScaledData* gyro) {
    float recipNorm;
    float s0, s1, s2, s3;
    float qDot1, qDot2, qDot3, qDot4;
    float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2, _8q1, _8q2, q0q0, q1q1, q2q2, q3q3;

    // Rate of change of quaternion from gyroscope
    qDot1 = 0.5f * (-out->q1 * gyro->x - out->q2 * gyro->y - out->q3 * gyro->z);
    qDot2 = 0.5f * (out->q0 * gyro->x + out->q2 * gyro->z - out->q3 * gyro->y);
    qDot3 = 0.5f * (out->q0 * gyro->y - out->q1 * gyro->z + out->q3 * gyro->x);
    qDot4 = 0.5f * (out->q0 * gyro->z + out->q1 * gyro->y - out->q2 * gyro->x);

    // Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
    if(!((accel->x == 0.0f) && (accel->y == 0.0f) && (accel->z == 0.0f))) {
        // Normalise accelerometer measurement
        recipNorm = imu_inv_sqrt(accel->x * accel->x + accel->y * accel->y + accel->z * accel->z);
        accel->x *= recipNorm;
        accel->y *= recipNorm;
        accel->z *= recipNorm;

        // Auxiliary variables to avoid repeated arithmetic
        _2q0 = 2.0f * out->q0;
        _2q1 = 2.0f * out->q1;
        _2q2 = 2.0f * out->q2;
        _2q3 = 2.0f * out->q3;
        _4q0 = 4.0f * out->q0;
        _4q1 = 4.0f * out->q1;
        _4q2 = 4.0f * out->q2;
        _8q1 = 8.0f * out->q1;
        _8q2 = 8.0f * out->q2;
        q0q0 = out->q0 * out->q0;
        q1q1 = out->q1 * out->q1;
        q2q2 = out->q2 * out->q2;
        q3q3 = out->q3 * out->q3;

        // Gradient decent algorithm corrective step
        s0 = _4q0 * q2q2 + _2q2 * accel->x + _4q0 * q1q1 - _2q1 * accel->y;
        s1 = _4q1 * q3q3 - _2q3 * accel->x + 4.0f * q0q0 * out->q1 - _2q0 * accel->y - _4q1 +
             _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * accel->z;
        s2 = 4.0f * q0q0 * out->q2 + _2q0 * accel->x + _4q2 * q3q3 - _2q3 * accel->y - _4q2 +
             _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * accel->z;
        s3 = 4.0f * q1q1 * out->q3 - _2q1 * accel->x + 4.0f * q2q2 * out->q3 - _2q2 * accel->y;
        recipNorm =
            imu_inv_sqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
        s0 *= recipNorm;
        s1 *= recipNorm;
        s2 *= recipNorm;
        s3 *= recipNorm;

        // Apply feedback step
        qDot1 -= FILTER_BETA * s0;
        qDot2 -= FILTER_BETA * s1;
        qDot3 -= FILTER_BETA * s2;
        qDot4 -= FILTER_BETA * s3;
    }

    // Integrate rate of change of quaternion to yield quaternion
    out->q0 += qDot1 * (1.0f / FILTER_SAMPLE_FREQ);
    out->q1 += qDot2 * (1.0f / FILTER_SAMPLE_FREQ);
    out->q2 += qDot3 * (1.0f / FILTER_SAMPLE_FREQ);
    out->q3 += qDot4 * (1.0f / FILTER_SAMPLE_FREQ);

    // Normalise quaternion
    recipNorm = imu_inv_sqrt(
        out->q0 * out->q0 + out->q1 * out->q1 + out->q2 * out->q2 + out->q3 * out->q3);
    out->q0 *= recipNorm;
    out->q1 *= recipNorm;
    out->q2 *= recipNorm;
    out->q3 *= recipNorm;
}
