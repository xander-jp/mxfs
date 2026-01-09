/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of erase operation for BCM2711 env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "bcm2711_def.h"

int mxfs_driver_erase(
    mxfs_inst_ptr inst,
    uint16_t block
)
{
    uint8_t flg_status = 0xFF;
    uint8_t cmd[8] = { 0x00, };

    while(1) {
        spi_read_registers(
            inst->spi,
            inst->gpio_mem,
            NULL,
            NULL,
            &flg_status
        );
        if (!(flg_status & STATUS_REGISTER_3_BUSY)) {
            break;
        }
        usleep(10);
    }
    // un protect
    cmd[0] = WRITE_STATUS_REGISTER_L;
    cmd[1] = STATUS_REGISTER_1_ADDR;
    cmd[2] = 0;
    inst->gpio_mem[GPCLR0] =  (1 << 22);
    spi_write(inst->spi, NULL, cmd, 3);
    inst->gpio_mem[GPSET0] =  (1 << 22);

    while(1) {
        spi_read_registers(
            inst->spi,
            inst->gpio_mem,
            NULL,
            NULL,
            &flg_status
        );
        if (!(flg_status & STATUS_REGISTER_3_BUSY)) {
            break;
        }
        usleep(10);
    }
    // write enable(0)
    cmd[0] = WRITE_ENABLE;
    inst->gpio_mem[GPCLR0] =  (1 << 22);
    spi_write(inst->spi, NULL, cmd, 1);
    inst->gpio_mem[GPSET0] =  (1 << 22);

    while(1) {
        spi_read_registers(
            inst->spi,
            inst->gpio_mem,
            NULL,
            NULL,
            &flg_status
        );
        if (!(flg_status & STATUS_REGISTER_3_BUSY) &&
            flg_status & STATUS_REGISTER_3_WEL)
        {
            break;
        }
        usleep(10);
    }
    // erase 
    cmd[0] = BLOCK_ERASE;
    cmd[1] = 0;
    cmd[2] = (uint8_t)((block >> 2) & 0x00FF);
    cmd[3] = (uint8_t)((block << 6) & 0x00C0);
    inst->gpio_mem[GPCLR0] =  (1 << 22);
    spi_write(inst->spi, NULL, cmd, 4);
    inst->gpio_mem[GPSET0] =  (1 << 22);

    return(OK);
}

