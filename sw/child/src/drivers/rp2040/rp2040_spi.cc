/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of SPI operation for rp2040 env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "rp2040_def.h"


int spi_read_registers(
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
        gpio_put(CS_PIN, 0);
        spi_write_blocking(SPI, cmd, 2);
        spi_read_blocking(SPI, 0, flg_protected, 1);
        gpio_put(CS_PIN, 1);
    }
    // configure
    if (flg_config) {
        cmd[0] = READ_STATUS_REGISTER_L;
        cmd[1] = STATUS_REGISTER_2_ADDR;
        gpio_put(CS_PIN, 0);
        spi_write_blocking(SPI, cmd, 2);
        spi_read_blocking(SPI, 0, flg_config, 1);
        gpio_put(CS_PIN, 1);
    }

    // status
    if (flg_status) {
        cmd[0] = READ_STATUS_REGISTER_L;
        cmd[1] = STATUS_REGISTER_3_ADDR;
        gpio_put(CS_PIN, 0);
        spi_write_blocking(SPI, cmd, 2);
        spi_read_blocking(SPI, 0, flg_status, 1);
        gpio_put(CS_PIN, 1);
    }
    return(OK);
}

int mxfs_driver_create_spi(
    mxfs_inst_ptr inst
)
{
    // setup spi.
#ifndef __RP2040__
    spi_init(SPI, 500 * 1000);
#else
    spi_init(SPI, 8 * 1000 * 1000);
#endif
    spi_set_slave(SPI, false);
#ifndef __RP2040__
    spi_set_format(SPI, 8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);
#else
    spi_set_format(SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
#endif

    gpio_set_function(SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(DI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(DO_PIN, GPIO_FUNC_SPI);
    // gpio_set_function(CS_PIN, GPIO_FUNC_SPI);

    // blink initialization.(SPI)
    for(int n = 0; n < 5; n++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(200);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(200);
    }

    uint8_t flg_status = 0xFF;
    uint8_t cmd[8] = { 0x00, };

    while(1) {
        spi_read_registers(
            NULL,
            NULL,
            &flg_status
        );
        if ((flg_status & STATUS_REGISTER_3_BUSY)) {
            sleep_ms(1);
            continue;
        }
        break;
    }
    cmd[0] = DEVICE_RESET;
    gpio_put(CS_PIN, 0);
    spi_write_blocking(SPI, cmd, 1);
    gpio_put(CS_PIN, 1);
    sleep_ms(1);

    while(1) {
        spi_read_registers(
            NULL,
            NULL,
            &flg_status
        );
        if ((flg_status & STATUS_REGISTER_3_BUSY)) {
            sleep_ms(1);
            continue;
        }
        break;
    }
    cmd[0] = JEDEC_ID;
    cmd[1] = 0;
    gpio_put(CS_PIN, 0);
    spi_write_blocking(SPI, cmd, 2);
    spi_read_blocking(SPI, 0, &cmd[2], 3);
    gpio_put(CS_PIN, 1);
    DRIVER_INFO(
        "device id: %02x %02x %02x",
        cmd[2],
        cmd[3],
        cmd[4]
    );
    // W25N01G
    if (cmd[2] != 0xEF || cmd[3] != 0xAA || cmd[4] != 0x21) {
        PANIC("not supported");
    }
    cmd[0] = WRITE_STATUS_REGISTER_L;
    cmd[1] = STATUS_REGISTER_1_ADDR;
    cmd[2] = 0;
    gpio_put(CS_PIN, 0);
    spi_write_blocking(SPI, cmd, 3);
    gpio_put(CS_PIN, 1);
    while(1) {
        spi_read_registers(
            NULL,
            NULL,
            &flg_status
        );
        if ((flg_status & STATUS_REGISTER_3_BUSY)) {
            sleep_ms(1);
            continue;
        }
        break;
    }

    cmd[0] = WRITE_ENABLE;
    gpio_put(CS_PIN, 0);
    spi_write_blocking(SPI, cmd, 1);
    gpio_put(CS_PIN, 1);

    while(1) {
        spi_read_registers(
            NULL,
            NULL,
            &flg_status
        );
        if ((flg_status & STATUS_REGISTER_3_BUSY)) {
            sleep_ms(1);
            continue;
        }
        if (flg_status & STATUS_REGISTER_3_WEL) {
            break;
        }
        sleep_ms(1);
    }



    DRIVER_INFO("mxfs_driver_create_spi, RP2040..(%p)", inst);
    return(OK);
}

int mxfs_driver_release_spi(
    mxfs_inst_ptr inst
)
{
    DRIVER_INFO("mxfs_driver_release_spi..(%p)", inst);
    return(OK);
}

