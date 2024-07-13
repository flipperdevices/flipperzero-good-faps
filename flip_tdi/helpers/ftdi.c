#include "ftdi.h"
#include "furi.h"
#include "ftdi_uart.h"

#define TAG "FTDI"

#define FTDI_TX_RX_BUF_SIZE (4096UL)
#define FTDI_INTERFACE_A (0x01UL)
#define FTDI_DRIVER_INTERFACE_A (0x00UL)
#define FTDI_UART_MAX_TX_SIZE (64UL)

struct Ftdi {
    FtdiModemStatus status;
    FuriStreamBuffer* stream_rx;
    FuriStreamBuffer* stream_tx;
    uint32_t baudrate;
    FtdiDataConfig data_config;
    FtdiBitMode bit_mode;
    uint8_t latency_timer;

    FtdiUart* ftdi_uart;
};

static bool ftdi_check_interface(Ftdi* ftdi, uint16_t index) {
    UNUSED(ftdi);
    uint8_t interface = index & 0xff;
    return ((interface) == FTDI_INTERFACE_A) || ((interface) == FTDI_DRIVER_INTERFACE_A);
}

Ftdi* ftdi_alloc(void) {
    Ftdi* ftdi = malloc(sizeof(Ftdi));
    ftdi->stream_rx =
        furi_stream_buffer_alloc(sizeof(uint8_t) * FTDI_TX_RX_BUF_SIZE, sizeof(uint8_t));
    ftdi->stream_tx =
        furi_stream_buffer_alloc(sizeof(uint8_t) * FTDI_TX_RX_BUF_SIZE, sizeof(uint8_t));
    ftdi->baudrate = 115200;



    ftdi->ftdi_uart = ftdi_uart_alloc(ftdi);


    return ftdi;
}

void ftdi_free(Ftdi* ftdi) {
    ftdi_uart_free(ftdi->ftdi_uart);
    furi_stream_buffer_free(ftdi->stream_tx);
    furi_stream_buffer_free(ftdi->stream_rx);
    free(ftdi);
}

void ftdi_reset_purge_rx(Ftdi* ftdi) {
    furi_stream_buffer_reset(ftdi->stream_rx);
}

void ftdi_reset_purge_tx(Ftdi* ftdi) {
    furi_stream_buffer_reset(ftdi->stream_tx);
}

void ftdi_reset_sio(Ftdi* ftdi) {
    UNUSED(ftdi);
}

uint32_t ftdi_set_tx_buf(Ftdi* ftdi, const uint8_t* data, uint32_t size) {
    uint32_t len = furi_stream_buffer_spaces_available(ftdi->stream_tx);

    if(len < size) {
        size = len;
        //ToDo: set error
        //ftdi->status.DR = 1;
        FURI_LOG_E(TAG, "FTDI TX buffer overflow");
    }

    return furi_stream_buffer_send(ftdi->stream_tx, data, size, 0);
}

uint32_t ftdi_get_tx_buf(Ftdi* ftdi, uint8_t* data, uint32_t size) {
    return furi_stream_buffer_receive(ftdi->stream_tx, data, size, 0);
}

uint32_t ftdi_available_tx_buf(Ftdi* ftdi) {
    return furi_stream_buffer_bytes_available(ftdi->stream_tx);
}

uint32_t ftdi_set_rx_buf(Ftdi* ftdi, const uint8_t* data, uint32_t size) {
    uint32_t len = furi_stream_buffer_spaces_available(ftdi->stream_rx);

    if(len < size) {
        size = len;
        //Todo: set error
        FURI_LOG_E(TAG, "FTDI RX buffer overflow");
    }
    return furi_stream_buffer_send(ftdi->stream_rx, data, size, 0);
}

uint32_t ftdi_get_rx_buf(Ftdi* ftdi, uint8_t* data, uint32_t size) {
    return furi_stream_buffer_receive(ftdi->stream_rx, data, size, 0);
}

uint32_t ftdi_available_rx_buf(Ftdi* ftdi) {
    return furi_stream_buffer_bytes_available(ftdi->stream_rx);
}

void ftdi_loopback(Ftdi* ftdi) {
    //todo fix size
    uint8_t data[128];
    uint32_t len = ftdi_get_rx_buf(ftdi, data, 128);
    ftdi_set_tx_buf(ftdi, data, len);
}

/*
https://www.ftdichip.com/Documents/AppNotes/AN_120_Aliasing_VCP_Baud_Rates.pdf

Note: Divisor = 1 and Divisor = 0 are special cases. A divisor of 0 will give 12MBaud, and a
divisor of 1 will give 8MBaud. Sub-integer divisors are not allowed if the main divisor (n) is
either 0 or 1.
    Divisor = 0 -> clk/1
    Divisor = 1 -> clk/1.5
    Divisor = 2 -> clk/2

To alias baud rates between 3MBaud and 12MBaud it is necessary to use driver version 2.4.20 or later
and the most significant bit (MSB) of the divisor must be a 1. This will ensure the divisor is dividing a
12MHz clock and not a 3MHz clock.
Example:
Each field consists of 4 bytes, ordered as follows: Byte0, Byte1, Byte2, Byte3. Bits 13
through 0 denote the integer divisor while bits 16, 15 and 14 denote the sub-integer divisor, as
follows
16,15,14 = 000 - sub-integer divisor = 0
16,15,14 = 001 - sub-integer divisor = 0.5
16,15,14 = 010 - sub-integer divisor = 0.25
16,15,14 = 011 - sub-integer divisor = 0.125
16,15,14 = 100 - sub-integer divisor = 0.375
16,15,14 = 101 - sub-integer divisor = 0.625
16,15,14 = 110 - sub-integer divisor = 0.75
16,15,14 = 111 - sub-integer divisor = 0.875

Step 1 - re-order the bytes: 35,40,01,00 => 00014035 Hex
Step 2 - extract the sub-integer divisor; 16 = 1, 15 = 0, 14 = 1 => sub-integer = 0.625
Step 3 - extract the integer divisor: 13:0 = 0035 Hex = 53 Dec
Step 4 - combine the integer and sub-integer divisors: 53.625 Dec
Step 5 - divide 3000000 by the divisor => 3000000/53.625 = 55944 baud

*/
void ftdi_set_baudrate(Ftdi* ftdi, uint16_t value, uint16_t index) {
    if(!ftdi_check_interface(ftdi, index)) {
        return;
    }

    uint32_t encoded_divisor = ((index & 0xff00) << 8) | value;
    bool is_12mhz = (encoded_divisor & 0x20000) == 0x20000;
    uint32_t clk = is_12mhz ? 12000000 : 3000000;

    uint32_t baudrate = 0;
    float divisor[8] = {0, 0.5, 0.25, 0.125, 0.375, 0.625, 0.75, 0.875};
    uint16_t integer_divisor = encoded_divisor & 0x3FFF;
    float sub_integer_divisor = divisor[(encoded_divisor >> 14) & 0x7];

    if((sub_integer_divisor == 0) && (integer_divisor < 3)) {
        if(integer_divisor == 0) {
            baudrate = clk;
        } else if(integer_divisor == 1) {
            baudrate = (uint32_t)round((float)clk / 1.5f);
        } else if(integer_divisor == 2) {
            baudrate = clk / 2;
        }
    } else {
        baudrate =
            round((float)clk / ((float)integer_divisor + divisor[(encoded_divisor >> 14) & 0x7]));
    }

    furi_log_puts("ftdi_set_baudrate=");
    char tmp_str2[] = "4294967295";
    itoa(baudrate, tmp_str2, 10);
    furi_log_puts(tmp_str2);
    furi_log_puts("\r\n");

    ftdi->baudrate = baudrate;

    ftdi_uart_set_baudrate(ftdi->ftdi_uart, baudrate);
}

void ftdi_set_data_config(Ftdi* ftdi, uint16_t value, uint16_t index) {
    if(!ftdi_check_interface(ftdi, index)) {
        return;
    }
    ftdi->data_config = *((FtdiDataConfig*)&value);
}

void ftdi_set_flow_ctrl(Ftdi* ftdi, uint16_t index) {
    if(!ftdi_check_interface(ftdi, index)) {
        return;
    }
    uint16_t flow_ctrl = index & 0xFF00;
    if(flow_ctrl != FtdiFlowControlDisable) {
        //ToDo: implement FtdiFlowControl
    }
}

void ftdi_set_bitmode(Ftdi* ftdi, uint16_t value, uint16_t index) {
    if(!ftdi_check_interface(ftdi, index)) {
        return;
    }
    //todo no use mask value&0xFF
    uint8_t bit_mode = value >> 8;
    ftdi->bit_mode = *((FtdiBitMode*)&(bit_mode));
}

void ftdi_set_latency_timer(Ftdi* ftdi, uint16_t value, uint16_t index) {
    if(!ftdi_check_interface(ftdi, index)) {
        return;
    }
    ftdi->latency_timer = value;
}

uint8_t ftdi_get_latency_timer(Ftdi* ftdi) {
    return ftdi->latency_timer;
}

// FTDI modem status

// Layout of the first byte:
//     - B0..B3 - must be 0
//     - B4       Clear to send (CTS)
//                  0 = inactive
//                  1 = active
//     - B5       Data set ready (DTS)
//                  0 = inactive
//                  1 = active
//     - B6       Ring indicator (RI)
//                  0 = inactive
//                  1 = active
//     - B7       Receive line signal detect (RLSD)
//                  0 = inactive
//                  1 = active

//     Layout of the second byte:
//     - B0       Data ready (DR)
//     - B1       Overrun error (OE)
//     - B2       Parity error (PE)
//     - B3       Framing error (FE)
//     - B4       Break interrupt (BI)
//     - B5       Transmitter holding register (THRE)
//     - B6       Transmitter empty (TEMT)
//     - B7       Error in RCVR FIFO
void ftdi_get_modem_status(uint16_t* status) {
    FtdiModemStatus modem_status = {0};
    modem_status.RESERVED1 = 1;
    modem_status.TEMT = 1;
    modem_status.THRE = 1;

    *status = *((uint16_t*)&modem_status);
}




void ftdi_start_uart_tx(Ftdi* ftdi) {
    ftdi_uart_tx(ftdi->ftdi_uart);
}