/******************************************************************************/
/*! @brief      mxfs filesystem driver defined for BCM2711 env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "drivers_def.h"

#include <linux/types.h> 
#include <linux/spi/spidev.h> 

#define GPIO ("/dev/gpiomem")
#define SPI0 ("/dev/spidev0.0")
#define SPI1 ("/dev/spidev0.1")



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


enum SPI {
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

enum BCM2711_REGISTERS {
    GPIO_BASE       = 0x20200000,
    GPIO_MASK       = 0xF4,
    //
    GPFSEL0         = (0x00 >> 2),
    GPFSEL1         = (0x04 >> 2),
    GPFSEL2         = (0x08 >> 2),
    GPFSEL3         = (0x0C >> 2),
    GPFSEL4         = (0x10 >> 2),
    GPFSEL5         = (0x14 >> 2),
    GPSET0          = (0x1C >> 2),
    GPSET1          = (0x20 >> 2),
    GPCLR0          = (0x28 >> 2),
    GPCLR1          = (0x2C >> 2),
    GPLEV0          = (0x34 >> 2),
    GPLEV1          = (0x38 >> 2),
    GPEDS0          = (0x40 >> 2),
    GPEDS1          = (0x44 >> 2),
    GPREN0          = (0x4C >> 2),
    GPREN1          = (0x50 >> 2),
    GPFEN0          = (0x58 >> 2),
    GPFEN1          = (0x5C >> 2),
    GPHEN0          = (0x64 >> 2),
    GPHEN1          = (0x68 >> 2),
    GPLEN0          = (0x70 >> 2),
    GPLEN1          = (0x74 >> 2),
    GPAREN0         = (0x7C >> 2),
    GPAREN1         = (0x80 >> 2),
    GPAFEN0         = (0x88 >> 2),
    GPAFEN1         = (0x8C >> 2),
    GPIO_PUP_PDN_CNTRL_REG0 = (0xE4),
    GPIO_PUP_PDN_CNTRL_REG1 = (0xE8),
    GPIO_PUP_PDN_CNTRL_REG2 = (0xEC),
    GPIO_PUP_PDN_CNTRL_REG3 = (0xF0),
};


extern int spi_read(
    int fd,
    uint8_t* rx,
    unsigned rxlen
);
extern int spi_write(
    int fd,
    const char* msg,
    uint8_t* tx,
    unsigned txlen
);
extern int spi_read_registers(
    int spi,
    uint32_t* gpio,
    uint8_t* flg_protected,
    uint8_t* flg_config,
    uint8_t* flg_status
);


