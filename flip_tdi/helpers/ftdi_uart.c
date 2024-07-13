#include "ftdi_uart.h"
#include "furi.h"
#include <furi_hal.h>

#include <stm32wbxx_ll_lpuart.h>

#define TAG "FTDI_UART"
#define FTDI_UART_MAX_TXRX_SIZE (64UL)

struct FtdiUart {
    FuriThread* worker_thread;
    FuriHalSerialHandle* serial_handle;
    uint32_t baudrate;
    Ftdi* ftdi;
};

typedef enum {
    WorkerEventReserved = (1 << 0), // Reserved for StreamBuffer internal event
    WorkerEventStop = (1 << 1),
    WorkerEventRxData = (1 << 2),
    WorkerEventRxIdle = (1 << 3),
    WorkerEventRxOverrunError = (1 << 4),
    WorkerEventRxFramingError = (1 << 5),
    WorkerEventRxNoiseError = (1 << 6),
    WorkerEventTXData = (1 << 7),
} WorkerEvent;

#define WORKER_EVENTS_MASK                                                                 \
    (WorkerEventStop | WorkerEventRxData | WorkerEventRxIdle | WorkerEventRxOverrunError | \
     WorkerEventRxFramingError | WorkerEventRxNoiseError | WorkerEventTXData)

static void ftdi_uart_irq_cb(
    FuriHalSerialHandle* handle,
    FuriHalSerialRxEvent ev,
    size_t size,
    void* context) {
    FtdiUart* ftdi_uart = context;

    if(ev & (FuriHalSerialRxEventData | FuriHalSerialRxEventIdle)) {
        uint8_t data[FURI_HAL_SERIAL_DMA_BUFFER_SIZE] = {0};
        while(size) {
            size_t ret = furi_hal_serial_dma_rx(
                handle,
                data,
                (size > FURI_HAL_SERIAL_DMA_BUFFER_SIZE) ? FURI_HAL_SERIAL_DMA_BUFFER_SIZE : size);
            ftdi_set_tx_buf(ftdi_uart->ftdi, data, ret);
            size -= ret;
        };
        //furi_thread_flags_set(furi_thread_get_id(usb_uart->worker_thread), WorkerEventRxData);
    }
}

static int32_t uart_echo_worker(void* context) {
    furi_assert(context);
    FtdiUart* ftdi_uart = context;

    FURI_LOG_I(TAG, "Worker started");

    while(1) {
        uint32_t events =
            furi_thread_flags_wait(WORKER_EVENTS_MASK, FuriFlagWaitAny, FuriWaitForever);
        furi_check((events & FuriFlagError) == 0);

        if(events & WorkerEventStop) break;

        if(events & WorkerEventTXData) {
            size_t length = 0;
            uint8_t data[FTDI_UART_MAX_TXRX_SIZE];
            do {
                length = ftdi_get_rx_buf(ftdi_uart->ftdi, data, FTDI_UART_MAX_TXRX_SIZE);
                if(length > 0) {
                    furi_hal_serial_tx(ftdi_uart->serial_handle, data, length);
                }
            } while(length > 0);
        }

        // if(events & WorkerEventRxData) {
        //     size_t length = 0;
        //     do {
        //         uint8_t data[FTDI_UART_MAX_TXRX_SIZE];
        //         //length = ftdi_get_rx_buf(ftdi_uart->ftdi, data, FTDI_UART_MAX_TXRX_SIZE);

        //         if(length > 0) {
        //             ftdi_set_tx_buf(ftdi_uart->ftdi, data, length);
        //         }
        //     } while(length > 0);
        // }

        if(events & WorkerEventRxIdle) {
            //furi_hal_serial_tx(ftdi_uart->serial_handle, (uint8_t*)"\r\nDetect IDLE\r\n", 15);
        }

        // if(events &
        //    (WorkerEventRxOverrunError | WorkerEventRxFramingError | WorkerEventRxNoiseError)) {
        //     if(events & WorkerEventRxOverrunError) {
        //         furi_hal_serial_tx(ftdi_uart->serial_handle, (uint8_t*)"\r\nDetect ORE\r\n", 14);
        //     }
        //     if(events & WorkerEventRxFramingError) {
        //         furi_hal_serial_tx(ftdi_uart->serial_handle, (uint8_t*)"\r\nDetect FE\r\n", 13);
        //     }
        //     if(events & WorkerEventRxNoiseError) {
        //         furi_hal_serial_tx(ftdi_uart->serial_handle, (uint8_t*)"\r\nDetect NE\r\n", 13);
        //     }
        // }
    }
    FURI_LOG_I(TAG, "Worker stopped");
    return 0;
}

FtdiUart* ftdi_uart_alloc(Ftdi* ftdi) {
    FtdiUart* ftdi_uart = malloc(sizeof(FtdiUart));
    ftdi_uart->baudrate = 115200;

    ftdi_uart->serial_handle = furi_hal_serial_control_acquire(FuriHalSerialIdLpuart);
    furi_check(ftdi_uart->serial_handle);
    furi_hal_serial_init(ftdi_uart->serial_handle, ftdi_uart->baudrate);
    furi_hal_serial_enable_direction(ftdi_uart->serial_handle, FuriHalSerialDirectionRx);
    furi_hal_serial_enable_direction(ftdi_uart->serial_handle, FuriHalSerialDirectionTx);
    furi_hal_serial_dma_rx_start(ftdi_uart->serial_handle, ftdi_uart_irq_cb, ftdi_uart, false);
    ftdi_uart->ftdi = ftdi;

    ftdi_uart->worker_thread = furi_thread_alloc_ex(TAG, 1024, uart_echo_worker, ftdi_uart);
    furi_thread_start(ftdi_uart->worker_thread);

    return ftdi_uart;
}

void ftdi_uart_free(FtdiUart* ftdi_uart) {
    furi_thread_flags_set(furi_thread_get_id(ftdi_uart->worker_thread), WorkerEventStop);
    furi_thread_join(ftdi_uart->worker_thread);
    furi_thread_free(ftdi_uart->worker_thread);

    furi_hal_serial_deinit(ftdi_uart->serial_handle);
    furi_hal_serial_control_release(ftdi_uart->serial_handle);
    ftdi_uart->serial_handle = NULL;
    free(ftdi_uart);
}

void ftdi_uart_tx(FtdiUart* ftdi_uart) {
    furi_thread_flags_set(furi_thread_get_id(ftdi_uart->worker_thread), WorkerEventTXData);
}

void ftdi_uart_set_baudrate(FtdiUart* ftdi_uart, uint32_t baudrate) {
    ftdi_uart->baudrate = baudrate;
    furi_hal_serial_set_br(ftdi_uart->serial_handle, baudrate);
}

void ftdi_uart_set_data_config(FtdiUart* ftdi_uart, FtdiDataConfig* data_config) {
    furi_assert(data_config);
    UNUSED(ftdi_uart);

    bool is_uart_enabled = LL_LPUART_IsEnabled(LPUART1);

    if(is_uart_enabled) {
        LL_LPUART_Disable(LPUART1);
    }

    uint32_t data_width = LL_LPUART_DATAWIDTH_8B;
    uint32_t parity_mode = LL_LPUART_PARITY_NONE;
    uint32_t stop_bits_mode = LL_LPUART_STOPBITS_2;

    switch(data_config->BITS) {
    case FtdiBits7:
        data_width = LL_LPUART_DATAWIDTH_7B;
        break;
    case FtdiBits8:
        data_width = LL_LPUART_DATAWIDTH_8B;
        break;
    default:
        break;
    }

    switch(data_config->PARITY) {
    case FtdiParityNone:
        parity_mode = LL_LPUART_PARITY_NONE;
        break;
    case FtdiParityOdd:
        parity_mode = LL_LPUART_PARITY_ODD;
        break;
    case FtdiParityEven:
    case FtdiParityMark:
    case FtdiParitySpace:
        parity_mode = LL_LPUART_PARITY_EVEN;
        break;
    default:
        break;
    }

    switch(data_config->STOP_BITS) {
    case FtdiStopBits1:
        stop_bits_mode = LL_LPUART_STOPBITS_1;
        break;
    case FtdiStopBits15:
    case FtdiStopBits2:
        stop_bits_mode = LL_LPUART_STOPBITS_2;
        break;
    default:
        break;
    }

    LL_LPUART_ConfigCharacter(LPUART1, data_width, parity_mode, stop_bits_mode);
    if(is_uart_enabled) {
        LL_LPUART_Enable(LPUART1);
    }
}
