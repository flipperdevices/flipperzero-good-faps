#include "imu_thread.h"
#include <furi_hal.h>
#include <furi.h>
#include "sensors/ICM42688P.h"

#define TAG "IMU"

#define ACCEL_GYRO_RATE DataRate1kHz

#define HID_RATE_DIV 5

typedef enum {
    ImuStop = (1 << 0),
    ImuNewData = (1 << 1),
    ImuReset = (1 << 2),
} ImuThreadFlags;

#define FLAGS_ALL (ImuStop | ImuNewData | ImuReset)

struct ImuThread {
    FuriThread* thread;
    ICM42688P* icm42688p;
    ImuDataCallback callback;
    void* context;
};

static void imu_irq_callback(void* context) {
    furi_assert(context);
    ImuThread* imu = context;
    furi_thread_flags_set(furi_thread_get_id(imu->thread), ImuNewData);
}

static int32_t imu_thread(void* context) {
    furi_assert(context);
    ImuThread* imu = context;
    uint32_t sample_cnt = 0;
    int16_t data_min[6];
    int16_t data_max[6];
    bool reset_flag = false;

    icm42688p_accel_config(imu->icm42688p, AccelFullScale2G, ACCEL_GYRO_RATE);
    icm42688p_gyro_config(imu->icm42688p, GyroFullScale15_625DPS, ACCEL_GYRO_RATE);

    icm42688_fifo_enable(imu->icm42688p, imu_irq_callback, imu);

    while(1) {
        uint32_t events = furi_thread_flags_wait(FLAGS_ALL, FuriFlagWaitAny, FuriWaitForever);

        if(events & ImuStop) {
            break;
        }

        if(events & ImuReset) {
            reset_flag = true;
        }

        if(events & ImuNewData) {
            uint16_t data_pending = icm42688_fifo_get_count(imu->icm42688p);
            ICM42688PFifoPacket data;
            while(data_pending--) {
                icm42688_fifo_read(imu->icm42688p, &data);
                int16_t data_cur[6];
                data_cur[0] = data.a_x;
                data_cur[1] = data.a_y;
                data_cur[2] = data.a_z;
                data_cur[3] = data.g_x;
                data_cur[4] = data.g_y;
                data_cur[5] = data.g_z;
                if(reset_flag) {
                    reset_flag = false;
                    for(size_t i = 0; i < 6; i++) {
                        data_min[i] = data_cur[i];
                        data_max[i] = data_cur[i];
                    }
                    
                } else {
                    for(size_t i = 0; i < 6; i++) {
                        if(data_cur[i] < data_min[i]) {
                            data_min[i] = data_cur[i];
                        } else if(data_cur[i] > data_max[i]) {
                            data_max[i] = data_cur[i];
                        }
                    }
                }
                sample_cnt++;
                if ((imu->callback) && (sample_cnt > 20)) {
                    sample_cnt = 0;
                    uint32_t data_diff[6];
                    for(size_t i = 0; i < 6; i++) {
                        data_diff[i] = abs(data_max[i] - data_min[i]);
                    }
                    imu->callback(data_diff, imu->context);
                }
            }
        }
    }

    icm42688_fifo_disable(imu->icm42688p);

    return 0;
}

void imu_reset_data(ImuThread* imu) {
    furi_assert(imu);
    furi_thread_flags_set(furi_thread_get_id(imu->thread), ImuReset);
}

ImuThread* imu_start(ICM42688P* icm42688p, ImuDataCallback callback, void* context) {
    ImuThread* imu = malloc(sizeof(ImuThread));
    imu->icm42688p = icm42688p;
    imu->callback = callback;
    imu->context = context;
    imu->thread = furi_thread_alloc_ex("ImuThread", 4096, imu_thread, imu);
    furi_thread_start(imu->thread);

    return imu;
}

void imu_stop(ImuThread* imu) {
    furi_assert(imu);

    furi_thread_flags_set(furi_thread_get_id(imu->thread), ImuStop);

    furi_thread_join(imu->thread);
    furi_thread_free(imu->thread);

    free(imu);
}
