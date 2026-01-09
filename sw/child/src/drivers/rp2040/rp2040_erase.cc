/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of erase operation for rp2040 env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "rp2040_def.h"

int mxfs_driver_erase(
    mxfs_inst_ptr inst,
    uint16_t block
)
{
    uint8_t flg_status = 0xFF;
    uint8_t cmd[8] = { 0x00, };
    
    DRIVER_DEBUG("mxfs_driver_erase(%u)", block);

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
    // write enable(0)
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
    // un protect
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
    // write enable(0)
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
    // erase 
    cmd[0] = BLOCK_ERASE;
    cmd[1] = 0;
    cmd[2] = (uint8_t)((block >> 2) & 0x00FF);
    cmd[3] = (uint8_t)((block << 6) & 0x00C0);

    gpio_put(CS_PIN, 0);
    spi_write_blocking(SPI, cmd, 4);
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
    return(OK);
}

