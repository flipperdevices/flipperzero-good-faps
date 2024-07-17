#include "ftdi_mpsse.h"
#include "furi.h"
#include <furi_hal.h>

#define TAG "FTDI_MPSSE"

struct FtdiMpsse {
    Ftdi* ftdi;
    uint8_t gpio_state;
    uint8_t gpio_mask;
    uint16_t data_size;
    bool is_loopback;
    bool is_div5;
    bool is_clk3phase;
    bool is_adaptive;
};

FtdiMpsse* ftdi_mpsse_alloc(Ftdi* ftdi) {
    FtdiMpsse* ftdi_mpsse = malloc(sizeof(FtdiMpsse));
    ftdi_mpsse->ftdi = ftdi;
    ftdi_mpsse->gpio_state = 0;
    ftdi_mpsse->gpio_mask = 0;
    ftdi_mpsse->data_size = 0;
    ftdi_mpsse->is_loopback = false;
    ftdi_mpsse->is_div5 = false;
    ftdi_mpsse->is_clk3phase = false;
    ftdi_mpsse->is_adaptive = false;
    return ftdi_mpsse;
}

void ftdi_mpsse_free(FtdiMpsse* ftdi_mpsse) {
    if(!ftdi_mpsse) return;
    free(ftdi_mpsse);
    ftdi_mpsse = NULL;
}

uint8_t ftdi_mpsse_get_data_stream(FtdiMpsse* ftdi_mpsse) {
    uint8_t data = 0;
    //Todo add timeout
    ftdi_get_rx_buf(ftdi_mpsse->ftdi, &data, 1);
    return data;
}

void ftdi_mpssse_set_data_stream(FtdiMpsse* ftdi_mpsse, uint8_t* data, uint16_t size) {
    ftdi_set_tx_buf(ftdi_mpsse->ftdi, data, size);
}

void ftdi_mpsse_get_data(FtdiMpsse* ftdi_mpsse) {
    //todo add support for tx buffer, data_size_max = 0xFF00
    ftdi_mpsse->data_size++;
    while(ftdi_mpsse->data_size--) {
        ftdi_mpsse_get_data_stream(ftdi_mpsse);
    }
}

static uint16_t ftdi_mpsse_get_data_size(FtdiMpsse* ftdi_mpsse) {
    return (uint16_t)ftdi_mpsse_get_data_stream(ftdi_mpsse) << 8 |
           ftdi_mpsse_get_data_stream(ftdi_mpsse);
}

static inline void ftdi_mpsse_skeep_data(FtdiMpsse* ftdi_mpsse) {
    ftdi_mpsse->data_size++;
    while(ftdi_mpsse->data_size--) {
        ftdi_mpsse_get_data_stream(ftdi_mpsse);
    }
}

void ftdi_mpsse_state_machine(FtdiMpsse* ftdi_mpsse) {
    uint8_t data = ftdi_mpsse_get_data_stream(ftdi_mpsse);
    switch(data) {
    case FtdiMpsseCommandsSetBitsLow: // 0x80  Change LSB GPIO output */
        ftdi_mpsse->gpio_state = ftdi_mpsse_get_data_stream(ftdi_mpsse);
        ftdi_mpsse->gpio_state = ftdi_mpsse_get_data_stream(ftdi_mpsse);
        //Write to GPIO
        break;
    case FtdiMpsseCommandsSetBitsHigh: // 0x82  Change MSB GPIO output */
        //Todo not supported
        ftdi_mpsse_get_data_stream(ftdi_mpsse);
        ftdi_mpsse_get_data_stream(ftdi_mpsse);
        break;
    case FtdiMpsseCommandsGetBitsLow: // 0x81  Get LSB GPIO output */
        //Read GPIO
        //add to buffer
        //read Gpio
        ftdi_mpssse_set_data_stream(ftdi_mpsse, &ftdi_mpsse->gpio_state, 1);
        break;
    case FtdiMpsseCommandsGetBitsHigh: // 0x83  Get MSB GPIO output */
        //Todo not supported
        //add to buffer FF
        uint8_t gpio = 0xFF;
        ftdi_mpssse_set_data_stream(ftdi_mpsse, &gpio, 1);
        break;
    case FtdiMpsseCommandsSendImmediate: // 0x87  Send immediate */
        //tx data to host add callback
        break;
    case FtdiMpsseCommandsWriteBytesPveMsb: // 0x10  Write bytes with positive edge clock, MSB first */
        //spi mode 1,3
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        ftdi_mpsse_get_data(ftdi_mpsse);
        break;
    case FtdiMpsseCommandsWriteBytesNveMsb: // 0x11  Write bytes with negative edge clock, MSB first */
        //spi mode 0,2
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        ftdi_mpsse_get_data(ftdi_mpsse);
        break;
    case FtdiMpsseCommandsWriteBitsPveMsb: // 0x12  Write bits with positive edge clock, MSB first */
        //not supported
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        ftdi_mpsse_skeep_data(ftdi_mpsse);
        break;
    case FtdiMpsseCommandsWriteBitsNveMsb: // 0x13  Write bits with negative edge clock, MSB first */
        //not supported
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        ftdi_mpsse_skeep_data(ftdi_mpsse);
        break;
    case FtdiMpsseCommandsWriteBytesPveLsb: // 0x18  Write bytes with positive edge clock, LSB first */
        //spi mode 1,3
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        ftdi_mpsse_get_data(ftdi_mpsse);
        break;
    case FtdiMpsseCommandsWriteBytesNveLsb: // 0x19  Write bytes with negative edge clock, LSB first */
        //spi mode 0,2
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        ftdi_mpsse_get_data(ftdi_mpsse);
        break;
    case FtdiMpsseCommandsWriteBitsPveLsb: // 0x1a  Write bits with positive edge clock, LSB first */
        //not supported
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        ftdi_mpsse_skeep_data(ftdi_mpsse);
        break;
    case FtdiMpsseCommandsWriteBitsNveLsb: // 0x1b  Write bits with negative edge clock, LSB first */
        //not supported
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        ftdi_mpsse_skeep_data(ftdi_mpsse);
        break;
    case FtdiMpsseCommandsReadBytesPveMsb: // 0x20  Read bytes with positive edge clock, MSB first */
        //spi mode 1,3
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //write data
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsReadBytesNveMsb: // 0x24  Read bytes with negative edge clock, MSB first */
        //spi mode 0,2
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //write data
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsReadBitsPveMsb: // 0x22  Read bits with positive edge clock, MSB first */
        //not supported
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //write data
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsReadBitsNveMsb: // 0x26  Read bits with negative edge clock, MSB first */
        //not supported
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //write data
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsReadBytesPveLsb: // 0x28  Read bytes with positive edge clock, LSB first */
        //spi mode 1,3
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //write data
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsReadBytesNveLsb: // 0x2c  Read bytes with negative edge clock, LSB first */
        //spi mode 0,2
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //write data
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsReadBitsPveLsb: // 0x2a  Read bits with positive edge clock, LSB first */
        //not supported
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //write data
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsReadBitsNveLsb: // 0x2e  Read bits with negative edge clock, LSB first */
        //not supported
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //write data
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsRwBytesPveNveMsb: // 0x31  Read/Write bytes with positive edge clock, MSB first */
        //spi mode 1,3
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        //ftdi_mpsse_get_data(ftdi_mpsse);
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsRwBytesNvePveMsb: // 0x34  Read/Write bytes with negative edge clock, MSB first */
        //spi mode 0,2
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        //ftdi_mpsse_get_data(ftdi_mpsse);
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsRwBitsPveNveMsb: // 0x33  Read/Write bits with positive edge clock, MSB first */
        //not supported
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        //ftdi_mpsse_skeep_data(ftdi_mpsse);
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsRwBitsNvePveMsb: // 0x36  Read/Write bits with negative edge clock, MSB first */
        //not supported
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        //ftdi_mpsse_skeep_data(ftdi_mpsse);
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsRwBytesPveNveLsb: // 0x39  Read/Write bytes with positive edge clock, LSB first */
        //spi mode 1,3
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        //ftdi_mpsse_get_data(ftdi_mpsse);
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsRwBytesNvePveLsb: // 0x3c  Read/Write bytes with negative edge clock, LSB first */
        //spi mode 0,2
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        //ftdi_mpsse_get_data(ftdi_mpsse);
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsRwBitsPveNveLsb: // 0x3b  Read/Write bits with positive edge clock, LSB first */
        //not supported
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        //ftdi_mpsse_skeep_data(ftdi_mpsse);
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsRwBitsNvePveLsb: // 0x3e  Read/Write bits with negative edge clock, LSB first */
        //not supported
        ftdi_mpsse->data_size = ftdi_mpsse_get_data_size(ftdi_mpsse);
        //read data
        //ftdi_mpsse_skeep_data(ftdi_mpsse);
        //ftdi_mpssse_set_data_stream(ftdi_mpsse, 0xFF, ftdi_mpsse->data_size);
        break;
    case FtdiMpsseCommandsWriteBitsTmsPve: // 0x4a  Write bits with TMS, positive edge clock */
        //not supported
        break;
    case FtdiMpsseCommandsWriteBitsTmsNve: // 0x4b  Write bits with TMS, negative edge clock */
        //not supported
        break;
    case FtdiMpsseCommandsRwBitsTmsPvePve: // 0x6a  Read/Write bits with TMS, positive edge clock, MSB first */
        //not supported
        break;
    case FtdiMpsseCommandsRwBitsTmsPveNve: // 0x6b  Read/Write bits with TMS, positive edge clock, MSB first */
        //not supported
        break;
    case FtdiMpsseCommandsRwBitsTmsNvePve: // 0x6e  Read/Write bits with TMS, negative edge clock, MSB first */
        //not supported
        break;
    case FtdiMpsseCommandsRwBitsTmsNveNve: // 0x6f  Read/Write bits with TMS, negative edge clock, MSB first */
        //not supported
        break;
    case FtdiMpsseCommandsLoopbackStart: // 0x84  Enable loopback */
        ftdi_mpsse->is_loopback = true;
        break;
    case FtdiMpsseCommandsLoopbackEnd: // 0x85  Disable loopback */
        ftdi_mpsse->is_loopback = false;
        break;
    case FtdiMpsseCommandsSetTckDivisor: // 0x86  Set clock */

        break;
    case FtdiMpsseCommandsDisDiv5: // 0x8a  Disable divide by 5 */
        ftdi_mpsse->is_div5 = false;
        break;
    case FtdiMpsseCommandsEnDiv5: // 0x8b  Enable divide by 5 */
        ftdi_mpsse->is_div5 = true;
        break;
    case FtdiMpsseCommandsEnableClk3Phase: // 0x8c  Enable 3-phase data clocking (I2C) */
        ftdi_mpsse->is_clk3phase = true;
        break;
    case FtdiMpsseCommandsDisableClk3Phase: // 0x8d  Disable 3-phase data clocking */
        ftdi_mpsse->is_clk3phase = false;
        break;
    case FtdiMpsseCommandsClkBitsNoData: // 0x8e  Allows JTAG clock to be output w/o data */
        //not supported
        break;
    case FtdiMpsseCommandsClkBytesNoData: // 0x8f  Allows JTAG clock to be output w/o data */
        //not supported
        break;
    case FtdiMpsseCommandsClkWaitOnHigh: // 0x94  Clock until GPIOL1 is high */
        //not supported
        break;
    case FtdiMpsseCommandsClkWaitOnLow: // 0x95  Clock until GPIOL1 is low */
        //not supported
        break;
    case FtdiMpsseCommandsEnableClkAdaptive: // 0x96  Enable JTAG adaptive clock for ARM */
        ftdi_mpsse->is_adaptive = true;
        break;
    case FtdiMpsseCommandsDisableClkAdaptive: // 0x97  Disable JTAG adaptive clock */
        ftdi_mpsse->is_adaptive = false;
        break;
    case FtdiMpsseCommandsClkCountWaitOnHigh: // 0x9c  Clock byte cycles until GPIOL1 is high */
        //not supported
        break;
    case FtdiMpsseCommandsClkCountWaitOnLow: // 0x9d  Clock byte cycles until GPIOL1 is low */
        //not supported
        break;
    case FtdiMpsseCommandsDriveZero: // 0x9e  Drive-zero mode */
        //not supported
        break;
    case FtdiMpsseCommandsWaitOnHigh: // 0x88  Wait until GPIOL1 is high */
        //not supported
        break;
    case FtdiMpsseCommandsWaitOnLow: // 0x89  Wait until GPIOL1 is low */
        //not supported
        break;
    case FtdiMpsseCommandsReadShort: // 0x90  Read short */
        //not supported
        break;
    case FtdiMpsseCommandsReadExtended: // 0x91  Read extended */
        //not supported
        break;
    case FtdiMpsseCommandsWriteShort: // 0x92  Write short */
        //not supported
        break;
    case FtdiMpsseCommandsWriteExtended: // 0x93  Write extended */
        //not supported
        break;

    default:
        break;
    }
}
