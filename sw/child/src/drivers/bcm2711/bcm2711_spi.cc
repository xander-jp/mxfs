/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of SPI operation for BCM2711 env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "bcm2711_def.h"

int spi_read(
    int fd,
    uint8_t* rx,
    unsigned rxlen
)
{
    uint8_t* tx = (uint8_t*)malloc(rxlen);
    if (!tx) {
        return(ERR);
    }
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
    auto ret = (ioctl(fd, SPI_IOC_MESSAGE(1), ioc));
    free(tx);
    if (ret != rxlen) {
        DRIVER_DEBUG("ioctl: %d, %u", ret, rxlen);
    }
    return(ret);
}

int spi_write(
    int fd,
    const char* msg,
    uint8_t* tx,
    unsigned txlen
)
{
    uint8_t* rx = (uint8_t*)malloc(txlen);
    if (!rx) {
        return(ERR);
    }
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
    if (ret != txlen) {
        DRIVER_DEBUG("ioctl: %d, %u", ret, txlen);
    }
    free(rx);
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
    return(0);
}


int mxfs_driver_create_spi(
    mxfs_inst_ptr inst
)
{
    inst->spi = 1;

    inst->spi = open(SPI0, O_RDWR);
    if (inst->spi > 0 && inst->gpio > 0 && inst->gpio_mem != NULL) {
        uint32_t wr_mode = 0;
        uint8_t bits_per_word = SPI_BITS;
        uint32_t max_speed = SPI_SPEED;
        if (ioctl(inst->spi, SPI_IOC_WR_MODE32, &wr_mode) < 0) { return(ERR); }
        if (ioctl(inst->spi, SPI_IOC_RD_MODE32, &wr_mode) < 0) { return(ERR); }
        if (ioctl(inst->spi, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) < 0) { return(ERR); }
        if (ioctl(inst->spi, SPI_IOC_RD_BITS_PER_WORD, &bits_per_word) < 0) { return(ERR); }
        if (ioctl(inst->spi, SPI_IOC_WR_MAX_SPEED_HZ, &max_speed) < 0) { return(ERR); }
        if (ioctl(inst->spi, SPI_IOC_RD_MAX_SPEED_HZ, &max_speed) < 0) { return(ERR); }

        // software reset.
        uint8_t cmd[8] = { DEVICE_RESET, };
        inst->gpio_mem[GPCLR0] =  (1 << 22);
        spi_write(inst->spi, "reset", cmd, 1);
        inst->gpio_mem[GPSET0] =  (1 << 22);

        usleep(1000);

        // device id
        cmd[0] = JEDEC_ID;
        cmd[1] = 0;
        inst->gpio_mem[GPCLR0] =  (1 << 22);
        spi_write(inst->spi, "device id", cmd, 2);
        memset(cmd, 0, sizeof(cmd));
        spi_read(inst->spi, cmd, 3);
        inst->gpio_mem[GPSET0] =  (1 << 22);

        DRIVER_INFO(
            "device(%02X %02X %02X)",
            cmd[0],
            cmd[1],
            cmd[2]
        );
        // W25N01G
        if (cmd[0] != 0xEF || cmd[1] != 0xAA || cmd[2] != 0x21) {
            PANIC("unknown device id");
        }
    }

    DRIVER_INFO("generated spi file-descriptor, BCM2711..(%p)", inst->spi);
    return(OK);
}

int mxfs_driver_release_spi(
    mxfs_inst_ptr inst
)
{
    close(inst->spi);
    printf("close spi..(%p)\n", inst->spi);
    return(OK);
}

