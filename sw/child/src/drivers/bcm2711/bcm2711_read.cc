/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of read operation for BCM2711 env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/

#include "bcm2711_def.h"

int mxfs_driver_read(
    mxfs_inst_ptr inst,
    uint16_t block,
    uint16_t page,
    uint8_t* buffer,
    uint16_t bufferlen
)
{
    uint8_t flg_status = 0xFF, flg_protected = 0xFF;

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
    spi_read_registers(
        inst->spi,
        inst->gpio_mem,
        &flg_protected,
        NULL,
        NULL 
    );
    
    uint8_t cmd[32] = { 0x00, };
    // read mode 
    cmd[0] = WRITE_STATUS_REGISTER_L;
    cmd[1] = STATUS_REGISTER_2_ADDR;
    cmd[2] = flg_protected;
    cmd[2] &= ~STATUS_REGISTER_2_ECC_E;
    cmd[2] |= STATUS_REGISTER_2_BUF;
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
    // program read
    cmd[0] = PG_DATA_READ;
    cmd[1] = 0;
    cmd[2] = (uint8_t)((block >> 2) & 0x00FF);
    cmd[3] = (uint8_t)((block << 6) & 0x00C0);
    cmd[3] |= (uint8_t)((page) & 0x003F);
    inst->gpio_mem[GPCLR0] =  (1 << 22);
    spi_write(inst->spi, NULL, cmd, 4);
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
    // read
    cmd[0] = READ;
    cmd[1] = 0;
    cmd[2] = 0;
    cmd[3] = 0;
    inst->gpio_mem[GPCLR0] =  (1 << 22);
    spi_write(inst->spi, NULL, cmd, 4);
    spi_read(inst->spi, buffer, bufferlen);
    inst->gpio_mem[GPSET0] = (1 << 22);

    return(OK);
}

