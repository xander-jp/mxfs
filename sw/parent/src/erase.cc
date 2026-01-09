/******************************************************************************/
/*! @brief      SPI NAND flash block erase operation
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "spinand_def.h"

int erase_block(
    int spi,
    uint32_t* gpio,
    uint8_t* status,
    uint16_t block
)
{
    uint8_t flg_protected = 0xFF, flg_config = 0xFF, flg_status = 0xFF;
    uint8_t cmd[32] = { 0x00, };

    spi_read_registers(
        spi,
        gpio,
        &flg_protected,
        &flg_config,
        &flg_status
    );
    if ((flg_status & STATUS_REGISTER_3_BUSY)) {
        return(ERASE_NO_DONE); 
    }

    if ((*status) == DEVICE_RESET) {
        // un protect
        cmd[0] = WRITE_STATUS_REGISTER_L;
        cmd[1] = STATUS_REGISTER_1_ADDR;
        cmd[2] = 0;
        gpio[GPCLR0] =  (1 << 22);
        spi_write(spi, NULL, cmd, 3);
        gpio[GPSET0] =  (1 << 22);
        (*status) = WRITE_ENABLE;
    } else if ((*status) == WRITE_ENABLE) {
        // write enable(1)
        cmd[0] = WRITE_ENABLE;
        gpio[GPCLR0] =  (1 << 22);
        spi_write(spi, NULL, cmd, 1);
        gpio[GPSET0] =  (1 << 22);
        (*status) = BLOCK_ERASE;
    } else if ((*status) == BLOCK_ERASE) {
        if (flg_status & STATUS_REGISTER_3_WEL) {
            // block erase
            cmd[0] = BLOCK_ERASE;
            cmd[1] = 0;
            cmd[2] = (uint8_t)((block >> 2) & 0x00FF);
            cmd[3] = (uint8_t)((block << 6) & 0x00C0);
            gpio[GPCLR0] =  (1 << 22);
            spi_write(spi, NULL, cmd, 4);
            gpio[GPSET0] =  (1 << 22);
            (*status) = DEVICE_RESET;
            return(ERASE_DONE);
        }
    }
    return(ERASE_NO_DONE);
}

