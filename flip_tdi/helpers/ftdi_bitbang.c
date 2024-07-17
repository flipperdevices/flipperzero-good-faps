#include "ftdi_bitbang.h"
#include "furi.h"
#include <furi_hal.h>
#include <furi_hal_bus.h>
#include <furi_hal_resources.h>
#include "ftdi_gpio.h"

#define TAG "FTDI_BITBANG"

// #define gpio_b0 (gpio_ext_pa7)
// #define gpio_b1 (gpio_ext_pa6)
// #define gpio_b2 (gpio_ext_pa4)
// #define gpio_b3 (gpio_ext_pb3)
// #define gpio_b4 (gpio_ext_pb2)
// #define gpio_b5 (gpio_ext_pc3)
// #define gpio_b6 (gpio_ext_pc1)
// #define gpio_b7 (gpio_ext_pc0)
typedef uint8_t (*FtdiBitbangGpioIO)(uint8_t state);
struct FtdiBitbang
{
    FuriThread *worker_thread;
    Ftdi *ftdi;
    uint32_t speed;
    bool enable;
    uint8_t gpio_mask;
    bool async;
    FtdiBitbangGpioIO gpio_io[8];
    uint8_t gpio_data;
};

typedef enum
{
    WorkerEventReserved = (1 << 0), // Reserved for StreamBuffer internal event
    WorkerEventStop = (1 << 1),
    WorkerEventTimerUpdate = (1 << 2),
} WorkerEvent;

#define WORKER_EVENTS_MASK (WorkerEventStop | WorkerEventTimerUpdate)

// static void ftdi_bitbang_gpio_init(FtdiBitbang* ftdi_bitbang);
// static void ftdi_bitbang_gpio_deinit(FtdiBitbang* ftdi_bitbang);
// static void ftdi_bitbang_gpio_set(FtdiBitbang* ftdi_bitbang, uint8_t gpio_data);
static void ftdi_bitbang_tim_init(FtdiBitbang *ftdi_bitbang);
static void ftdi_bitbang_tim_deinit(FtdiBitbang *ftdi_bitbang);

void ftdi_bitbang_gpio_set_direction(FtdiBitbang *ftdi_bitbang)
{
    ftdi_gpio_set_direction(ftdi_bitbang->gpio_mask);
    if (ftdi_bitbang->gpio_mask & 0b00000001)
    {
        ftdi_bitbang->gpio_io[0] = ftdi_gpio_set_b0;
    }
    else
    {
        ftdi_bitbang->gpio_io[0] = ftdi_gpio_get_b0;
    }

    if (ftdi_bitbang->gpio_mask & 0b00000010)
    {
        ftdi_bitbang->gpio_io[1] = ftdi_gpio_set_b1;
    }
    else
    {
        ftdi_bitbang->gpio_io[1] = ftdi_gpio_get_b1;
    }

    if (ftdi_bitbang->gpio_mask & 0b00000100)
    {
        ftdi_bitbang->gpio_io[2] = ftdi_gpio_set_b2;
    }
    else
    {
        ftdi_bitbang->gpio_io[2] = ftdi_gpio_get_b2;
    }

    if (ftdi_bitbang->gpio_mask & 0b00001000)
    {
        ftdi_bitbang->gpio_io[3] = ftdi_gpio_set_b3;
    }
    else
    {
        ftdi_bitbang->gpio_io[3] = ftdi_gpio_get_b3;
    }

    if (ftdi_bitbang->gpio_mask & 0b00010000)
    {
        ftdi_bitbang->gpio_io[4] = ftdi_gpio_set_b4;
    }
    else
    {
        ftdi_bitbang->gpio_io[4] = ftdi_gpio_get_b4;
    }

    if (ftdi_bitbang->gpio_mask & 0b00100000)
    {
        ftdi_bitbang->gpio_io[5] = ftdi_gpio_set_b5;
    }
    else
    {
        ftdi_bitbang->gpio_io[5] = ftdi_gpio_get_b5;
    }

    if (ftdi_bitbang->gpio_mask & 0b01000000)
    {
        ftdi_bitbang->gpio_io[6] = ftdi_gpio_set_b6;
    }
    else
    {
        ftdi_bitbang->gpio_io[6] = ftdi_gpio_get_b6;
    }

    if (ftdi_bitbang->gpio_mask & 0b10000000)
    {
        ftdi_bitbang->gpio_io[7] = ftdi_gpio_set_b7;
    }
    else
    {
        ftdi_bitbang->gpio_io[7] = ftdi_gpio_get_b7;
    }
}

void ftdi_bitbang_gpio_init(FtdiBitbang *ftdi_bitbang)
{
    ftdi_gpio_init(ftdi_bitbang->gpio_mask);
    ftdi_bitbang_gpio_set_direction(ftdi_bitbang);
}

uint8_t ftdi_bitbang_gpio_io(FtdiBitbang *ftdi_bitbang, uint8_t gpio_data_out)
{
    uint8_t gpio_data_in = 0;
    gpio_data_in |= ftdi_bitbang->gpio_io[0](gpio_data_out & 0b00000001);
    gpio_data_in |= ftdi_bitbang->gpio_io[1](gpio_data_out & 0b00000010);
    gpio_data_in |= ftdi_bitbang->gpio_io[2](gpio_data_out & 0b00000100);
    gpio_data_in |= ftdi_bitbang->gpio_io[3](gpio_data_out & 0b00001000);
    gpio_data_in |= ftdi_bitbang->gpio_io[4](gpio_data_out & 0b00010000);
    gpio_data_in |= ftdi_bitbang->gpio_io[5](gpio_data_out & 0b00100000);
    gpio_data_in |= ftdi_bitbang->gpio_io[6](gpio_data_out & 0b01000000);
    gpio_data_in |= ftdi_bitbang->gpio_io[7](gpio_data_out & 0b10000000);
    return gpio_data_in;
}

static int32_t ftdi_bitbang_worker(void *context)
{
    furi_assert(context);
    FtdiBitbang *ftdi_bitbang = context;

    uint8_t buffer[64];

    FURI_LOG_I(TAG, "Worker started");
    ftdi_bitbang_tim_init(ftdi_bitbang);
    // uint8_t gpio_data[1] = {0};
    while (1)
    {
        uint32_t events =
            furi_thread_flags_wait(WORKER_EVENTS_MASK, FuriFlagWaitAny, FuriWaitForever);
        furi_check((events & FuriFlagError) == 0);

        if (events & WorkerEventStop)
            break;
        if (events & WorkerEventTimerUpdate)
        {
            size_t length = ftdi_get_rx_buf(ftdi_bitbang->ftdi, buffer, 1);
            if (length > 0)
            {
                // ftdi_bitbang_gpio_set(ftdi_bitbang, buffer[0]);
                ftdi_bitbang->gpio_data = ftdi_bitbang_gpio_io(ftdi_bitbang, buffer[0]);
                if (!ftdi_bitbang->async)
                {
                    // buffer[0] = ftdi_bitbang_gpio_get(ftdi_bitbang);
                    ftdi_set_tx_buf(ftdi_bitbang->ftdi, &ftdi_bitbang->gpio_data, 1);
                }
            }
            if (ftdi_bitbang->enable && ftdi_bitbang->async)
            {
                ftdi_bitbang->gpio_data = ftdi_bitbang_gpio_io(ftdi_bitbang, ftdi_bitbang->gpio_data);
                ftdi_set_tx_buf(ftdi_bitbang->ftdi, &ftdi_bitbang->gpio_data, 1);
            }
        }
    }
    ftdi_bitbang_tim_deinit(ftdi_bitbang);
    // ftdi_bitbang_gpio_deinit(ftdi_bitbang);
    // ftdi_gpio_deinit();
    FURI_LOG_I(TAG, "Worker stopped");
    return 0;
}

// #pragma GCC push_options
// #pragma GCC optimize("Ofast,unroll-loops")
// // #pragma GCC optimize("O3")
// // #pragma GCC optimize("-funroll-all-loops")

// static void ftdi_bitbang_gpio_init(FtdiBitbang* ftdi_bitbang) {
//     if(ftdi_bitbang->gpio_mask & 0b00000001) {
//         furi_hal_gpio_init(&gpio_b0, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
//     } else {
//         furi_hal_gpio_init(&gpio_b0, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
//     }
//     if(ftdi_bitbang->gpio_mask & 0b00000010) {
//         furi_hal_gpio_init(&gpio_b1, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
//     } else {
//         furi_hal_gpio_init(&gpio_b1, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
//     }
//     if(ftdi_bitbang->gpio_mask & 0b00000100) {
//         furi_hal_gpio_init(&gpio_b2, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
//     } else {
//         furi_hal_gpio_init(&gpio_b2, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
//     }
//     if(ftdi_bitbang->gpio_mask & 0b00001000) {
//         furi_hal_gpio_init(&gpio_b3, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
//     } else {
//         furi_hal_gpio_init(&gpio_b3, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
//     }
//     if(ftdi_bitbang->gpio_mask & 0b00010000) {
//         furi_hal_gpio_init(&gpio_b4, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
//     } else {
//         furi_hal_gpio_init(&gpio_b4, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
//     }
//     if(ftdi_bitbang->gpio_mask & 0b00100000) {
//         furi_hal_gpio_init(&gpio_b5, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
//     } else {
//         furi_hal_gpio_init(&gpio_b5, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
//     }
//     if(ftdi_bitbang->gpio_mask & 0b01000000) {
//         furi_hal_gpio_init(&gpio_b6, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
//     } else {
//         furi_hal_gpio_init(&gpio_b6, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
//     }
//     if(ftdi_bitbang->gpio_mask & 0b10000000) {
//         furi_hal_gpio_init(&gpio_b7, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
//     } else {
//         furi_hal_gpio_init(&gpio_b7, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
//     }
// }

// static void ftdi_bitbang_gpio_deinit(FtdiBitbang* ftdi_bitbang) {
//     UNUSED(ftdi_bitbang);
//     furi_hal_gpio_init(&gpio_b0, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
//     furi_hal_gpio_init(&gpio_b1, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
//     furi_hal_gpio_init(&gpio_b2, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
//     furi_hal_gpio_init(&gpio_b3, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
//     furi_hal_gpio_init(&gpio_b4, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
//     furi_hal_gpio_init(&gpio_b5, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
//     furi_hal_gpio_init(&gpio_b6, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
//     furi_hal_gpio_init(&gpio_b7, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
// }

// static void ftdi_bitbang_gpio_set(FtdiBitbang* ftdi_bitbang, uint8_t gpio_data) {
//     if(ftdi_bitbang->gpio_mask & 0b00000001) {
//         if(gpio_data & 0b00000001) {
//             furi_hal_gpio_write(&gpio_b0, 1);
//         } else {
//             furi_hal_gpio_write(&gpio_b0, 0);
//         }
//     }

//     if(ftdi_bitbang->gpio_mask & 0b00000010) {
//         if(gpio_data & 0b00000010) {
//             furi_hal_gpio_write(&gpio_b1, 1);
//         } else {
//             furi_hal_gpio_write(&gpio_b1, 0);
//         }
//     }

//     if(ftdi_bitbang->gpio_mask & 0b00000100) {
//         if(gpio_data & 0b00000100) {
//             furi_hal_gpio_write(&gpio_b2, 1);
//         } else {
//             furi_hal_gpio_write(&gpio_b2, 0);
//         }
//     }

//     if(ftdi_bitbang->gpio_mask & 0b00001000) {
//         if(gpio_data & 0b00001000) {
//             furi_hal_gpio_write(&gpio_b3, 1);
//         } else {
//             furi_hal_gpio_write(&gpio_b3, 0);
//         }
//     }

//     if(ftdi_bitbang->gpio_mask & 0b00010000) {
//         if(gpio_data & 0b00010000) {
//             furi_hal_gpio_write(&gpio_b4, 1);
//         } else {
//             furi_hal_gpio_write(&gpio_b4, 0);
//         }
//     }

//     if(ftdi_bitbang->gpio_mask & 0b00100000) {
//         if(gpio_data & 0b00100000) {
//             furi_hal_gpio_write(&gpio_b5, 1);
//         } else {
//             furi_hal_gpio_write(&gpio_b5, 0);
//         }
//     }

//     if(ftdi_bitbang->gpio_mask & 0b01000000) {
//         if(gpio_data & 0b01000000) {
//             furi_hal_gpio_write(&gpio_b6, 1);
//         } else {
//             furi_hal_gpio_write(&gpio_b6, 0);
//         }
//     }

//     if(ftdi_bitbang->gpio_mask & 0b10000000) {
//         if(gpio_data & 0b10000000) {
//             furi_hal_gpio_write(&gpio_b7, 1);
//         } else {
//             furi_hal_gpio_write(&gpio_b7, 0);
//         }
//     }
// }

// uint8_t ftdi_bitbang_gpio_get(FtdiBitbang* ftdi_bitbang) {
//     UNUSED(ftdi_bitbang);
//     uint8_t gpio_data = 0;
//     if(furi_hal_gpio_read(&gpio_b0)) {
//         gpio_data |= 0b00000001;
//     }
//     if(furi_hal_gpio_read(&gpio_b1)) {
//         gpio_data |= 0b00000010;
//     }
//     if(furi_hal_gpio_read(&gpio_b2)) {
//         gpio_data |= 0b00000100;
//     }
//     if(furi_hal_gpio_read(&gpio_b3)) {
//         gpio_data |= 0b00001000;
//     }
//     if(furi_hal_gpio_read(&gpio_b4)) {
//         gpio_data |= 0b00010000;
//     }
//     if(furi_hal_gpio_read(&gpio_b5)) {
//         gpio_data |= 0b00100000;
//     }
//     if(furi_hal_gpio_read(&gpio_b6)) {
//         gpio_data |= 0b01000000;
//     }
//     if(furi_hal_gpio_read(&gpio_b7)) {
//         gpio_data |= 0b10000000;
//     }

//     return gpio_data;
// }

// #pragma GCC pop_options

uint8_t ftdi_bitbang_gpio_get(FtdiBitbang *ftdi_bitbang)
{
    return ftdi_bitbang_gpio_io(ftdi_bitbang, ftdi_bitbang->gpio_data);
}

FtdiBitbang *ftdi_bitbang_alloc(Ftdi *ftdi)
{
    FtdiBitbang *ftdi_bitbang = malloc(sizeof(FtdiBitbang));
    ftdi_bitbang->ftdi = ftdi;
    ftdi_bitbang->enable = false;
    ftdi_bitbang->gpio_mask = 0;
    ftdi_bitbang_gpio_init(ftdi_bitbang);
    ftdi_bitbang->worker_thread =
        furi_thread_alloc_ex(TAG, 1024, ftdi_bitbang_worker, ftdi_bitbang);

    furi_thread_start(ftdi_bitbang->worker_thread);
    return ftdi_bitbang;
}

void ftdi_bitbang_free(FtdiBitbang *ftdi_bitbang)
{
    if (!ftdi_bitbang)
        return;

    ftdi_bitbang->enable = false;
    furi_thread_flags_set(ftdi_bitbang->worker_thread, WorkerEventStop);
    furi_thread_join(ftdi_bitbang->worker_thread);
    furi_thread_free(ftdi_bitbang->worker_thread);

    ftdi_gpio_deinit();

    free(ftdi_bitbang);
    ftdi_bitbang = NULL;
}

void ftdi_bitbang_set_gpio(FtdiBitbang *ftdi_bitbang, uint8_t gpio_mask)
{
    ftdi_bitbang->gpio_mask = gpio_mask;
    ftdi_bitbang_gpio_set_direction(ftdi_bitbang);
}

void ftdi_bitbang_enable(FtdiBitbang *ftdi_bitbang, bool enable, bool async)
{
    ftdi_bitbang->enable = enable;
    ftdi_bitbang->async = async;

    if (enable)
    {
        LL_TIM_SetCounter(TIM17, 0);
        LL_TIM_EnableCounter(TIM17);
    }
    else
    {
        LL_TIM_DisableCounter(TIM17);
    }
}

void ftdi_bitbang_set_speed(FtdiBitbang *ftdi_bitbang, uint32_t speed)
{
    UNUSED(ftdi_bitbang);
    UNUSED(speed);

    uint32_t freq_div = 64000000LU / speed;
    uint32_t prescaler = freq_div / 0x10000LU;
    uint32_t period = freq_div / (prescaler + 1);
    LL_TIM_SetPrescaler(TIM17, prescaler);
    LL_TIM_SetAutoReload(TIM17, period - 1);
}

static void ftdi_bitbang_tim_isr(void *context)
{
    FtdiBitbang *ftdi_bitbang = context;
    UNUSED(ftdi_bitbang);
    if (LL_TIM_IsActiveFlag_UPDATE(TIM17))
    {
        LL_TIM_ClearFlag_UPDATE(TIM17);
        furi_thread_flags_set(
            furi_thread_get_id(ftdi_bitbang->worker_thread), WorkerEventTimerUpdate);
    }
}

static void ftdi_bitbang_tim_init(FtdiBitbang *ftdi_bitbang)
{
    furi_hal_bus_enable(FuriHalBusTIM17);

    LL_TIM_SetCounterMode(TIM17, LL_TIM_COUNTERMODE_UP);
    LL_TIM_SetClockDivision(TIM17, LL_TIM_CLOCKDIVISION_DIV1);
    LL_TIM_SetAutoReload(TIM17, 6400 - 1);
    LL_TIM_SetPrescaler(TIM17, 0); // 10kHz
    LL_TIM_SetClockSource(TIM17, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_DisableARRPreload(TIM17);

    furi_hal_interrupt_set_isr(
        FuriHalInterruptIdTim1TrgComTim17, ftdi_bitbang_tim_isr, ftdi_bitbang);

    LL_TIM_EnableIT_UPDATE(TIM17);
}

static void ftdi_bitbang_tim_deinit(FtdiBitbang *ftdi_bitbang)
{
    UNUSED(ftdi_bitbang);
    LL_TIM_DisableCounter(TIM17);
    furi_hal_bus_disable(FuriHalBusTIM17);
    furi_hal_interrupt_set_isr(FuriHalInterruptIdTim1TrgComTim17, NULL, NULL);
}