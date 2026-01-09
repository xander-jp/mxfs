/******************************************************************************/
/*! @brief      SPI communication interface implementation
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "spinand_def.h"

int spi_read(
    int fd,
    uint8_t* rx,
    unsigned rxlen 
)
{
    uint8_t tx[rxlen] = { 0x00, };
    struct spi_ioc_transfer ioc[] = {
        {
            .tx_buf = (unsigned long)tx,
            .rx_buf = (unsigned long)rx,
            .len = rxlen,
            .speed_hz = SPI_SPEED,
            .delay_usecs = SPI_DELAY,
            .bits_per_word = SPI_BITS,
            .cs_change = 0,
        },
    };
    return(ioctl(fd, SPI_IOC_MESSAGE(1), ioc));
}

int spi_write(
    int fd,
    const char* msg,
    uint8_t* tx,
    unsigned txlen 
)
{
    uint8_t rx[txlen] = { 0x00, };
    struct spi_ioc_transfer ioc[] = {
        {
            .tx_buf = (unsigned long)tx,
            .rx_buf = (unsigned long)rx,
            .len = txlen,
            .speed_hz = SPI_SPEED,
            .delay_usecs = SPI_DELAY,
            .bits_per_word = SPI_BITS,
            .cs_change = 0,
        },
    };
    int ret = (ioctl(fd, SPI_IOC_MESSAGE(1), ioc));
    if (msg) {
        printf("%-16s [%d , %d]", msg, ret ,(int)txlen);
        for (int n = 0;n < ret; n++) {
            printf("%02x ", tx[n]);
        }
        printf("\n");
    }
    return(ret);
}

int spi_read_registers(
    int spi,
    uint32_t* gpio,
    uint8_t* flg_protected,
    uint8_t* flg_config,
    uint8_t* flg_status
)
{
    uint8_t cmd[32] = { 0, };

    // protected
    if (flg_protected) {
        cmd[0] = READ_STATUS_REGISTER_L;
        cmd[1] = STATUS_REGISTER_1_ADDR;
        gpio[GPCLR0] =  (1 << 22);
        spi_write(spi, NULL, cmd, 2);
        spi_read(spi, flg_protected, 1);
        gpio[GPSET0] =  (1 << 22);
    }
    // configure
    if (flg_config) {
        cmd[0] = READ_STATUS_REGISTER_L;
        cmd[1] = STATUS_REGISTER_2_ADDR;
        gpio[GPCLR0] =  (1 << 22);
        spi_write(spi, NULL, cmd, 2);
        spi_read(spi, flg_config, 1);
        gpio[GPSET0] =  (1 << 22);
    }

    // status
    if (flg_status) {
        cmd[0] = READ_STATUS_REGISTER_L;
        cmd[1] = STATUS_REGISTER_3_ADDR;
        gpio[GPCLR0] =  (1 << 22);
        spi_write(spi, NULL, cmd, 2);
        spi_read(spi, flg_status, 1);
        gpio[GPSET0] =  (1 << 22);
    }
#if 0
    printf("status (%02X %02X %02X)\n",
        (*flg_protected),
        (*flg_config),
        (*flg_status)
    );
#endif
    return(0);
}

