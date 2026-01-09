/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of write operation for BCM2711 env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "bcm2711_def.h"

int mxfs_driver_write(
    mxfs_inst_ptr inst,
    uint16_t block,
    uint16_t page,
    uint8_t* buffer,
    uint16_t bufferlen
)
{
    uint8_t flg_status = 0xFF;
    uint8_t cmd[2112] = { 0x00, };

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
    // program load 
    cmd[0] = RANDOM_PG_DATA_LOAD;
    cmd[1] = 0;
    cmd[2] = 0;
    if (bufferlen > (sizeof(cmd) - 3)) {
        return(ERR);
    }
    memcpy(
        &cmd[3],
        buffer,
        bufferlen
    );
    inst->gpio_mem[GPCLR0] =  (1 << 22);
    spi_write(inst->spi, NULL, cmd, bufferlen + 3);
    inst->gpio_mem[GPSET0] =  (1 << 22);

    printf(">>>> write >>>>\n");
    for (auto n = 0; n < bufferlen + 3; n++) {
        printf("%02X ", cmd[n]);
        if ((n+1) % 32  == 0) {
            printf("\n");
        } else if ((n+1) % 8 == 0) {
            printf(":");
        }
    }
    printf("\n<<<<\n");



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
    // program execute 
    cmd[0] = PG_EXECUTE;
    cmd[1] = 0;
    cmd[2] = (uint8_t)((block >> 2) & 0x00FF);
    cmd[3] = (uint8_t)((block << 6) & 0x00C0);
    cmd[3] |= (uint8_t)(page & 0x003F);
    inst->gpio_mem[GPCLR0] =  (1 << 22);
    spi_write(inst->spi, NULL, cmd, 4);
    inst->gpio_mem[GPSET0] =  (1 << 22);

    return(OK);
}

