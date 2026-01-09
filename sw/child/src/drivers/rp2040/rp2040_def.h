/******************************************************************************/
/*! @brief      mxfs filesystem driver defined for rp2040 env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "drivers_def.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "hardware/spi.h"
#include "pico/util/queue.h"

#define SPI spi0 
#define CS_PIN      17 
#define SCK_PIN     18
#define DI_PIN      19
#define DO_PIN      16
#define WP_PIN      14
#define HLD_PIN     15

enum INSTRUCTION {
    DEVICE_RESET            = 0xFF,
    DIE_SELECT              = 0xC2,
    JEDEC_ID                = 0x9F,
    READ_STATUS_REGISTER_L  = 0x0F,
    READ_STATUS_REGISTER_H  = 0x05,
    WRITE_STATUS_REGISTER_L = 0x1F,
    WRITE_STATUS_REGISTER_H = 0x01,
    WRITE_ENABLE            = 0x06,
    WRITE_DISABLE           = 0x04,
    BB_MANAGEMENT           = 0xA1,
    READ_BBM_LUT            = 0xA5,
    LAST_ECC_FAILURE        = 0xA9,
    BLOCK_ERASE             = 0xD8,
    PG_DATA_LOAD            = 0x02,
    RANDOM_PG_DATA_LOAD     = 0x84,
    QUAD_PG_DATA_LOAD       = 0x32,
    RANDOM_QUAD_PG_DATA_LOAD= 0x34,
    PG_EXECUTE              = 0x10,
    PG_DATA_READ            = 0x13,
    READ                    = 0x03,
    FAST_READ               = 0x0B,
    FAST_READ_WITH_4B       = 0x0C,
    FAST_READ_DUAL_OUTPUT   = 0x3B,
    FAST_READ_DUAL_OUTPUT_WITH_4B = 0x3C,
    FAST_READ_QUAD_OUTPUT   = 0x6B,
    FAST_READ_QUAD_OUTPUT_WITH_4B = 0x6C,
    FAST_READ_DUAL_IO       = 0xBB,
    FAST_READ_DUAL_IO_WITH_4B = 0xBC,
    FAST_READ_QUAD_IO       = 0xEB,
    FAST_READ_QUAD_IO_WITH_4B = 0xEC,
};

enum STATUS_REGISTER {
    STATUS_REGISTER_1_ADDR  = 0xA0,
    STATUS_REGISTER_1_TB    = 0x04,
    STATUS_REGISTER_1_BP0   = 0x08,
    STATUS_REGISTER_1_BP1   = 0x10,
    STATUS_REGISTER_1_BP2   = 0x20,
    STATUS_REGISTER_1_BP3   = 0x40,
    STATUS_REGISTER_1_SRP1  = 0x01,
    STATUS_REGISTER_2_ADDR  = 0xB0,
    STATUS_REGISTER_2_ECC_E = 0x10,
    STATUS_REGISTER_2_BUF   = 0x08,
    STATUS_REGISTER_2_OTP_E = 0x40,
    STATUS_REGISTER_3_ADDR  = 0xC0,
    STATUS_REGISTER_3_BUSY  = 0x01,
    STATUS_REGISTER_3_WEL   = 0x02,
    STATUS_REGISTER_3_PFAIL = 0x08,
    STATUS_REGISTER_3_ECC_0 = 0x10,
    STATUS_REGISTER_3_ECC_1 = 0x20,
};


enum CONSTANT {
    SPI_SPEED       = 8000000,
    SPI_BITS        = 8,
    SPI_DELAY       = (SPI_SPEED/100000),
    READ_NO_DATA    = 0,
    READ_WITH_DATA  = 1,
    READ_INVALID_BLOCK = -1,
    ERASE_NO_DONE   = 0,
    ERASE_DONE      = 1,
    WRITE_NO_DONE   = 0,
    WRITE_DONE      = 1,
    WRITE_EXCEPTION = -1,
};

extern int spi_read_registers(
    uint8_t* flg_protected,
    uint8_t* flg_config,
    uint8_t* flg_status
);


