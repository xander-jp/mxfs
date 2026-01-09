/******************************************************************************/
/*! @brief      SPI NAND flash page write operation
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "spinand_def.h"

// write block, page 
int write_block_page(
    int spi,
    uint32_t* gpio,
    uint8_t* status,
    uint16_t block,
    uint16_t page,
    uint8_t* wbf,
    uint16_t wbflen
)
{
    uint8_t flg_status = 0xFF;
    uint8_t cmd[2112] = { 0x00, };
    spi_read_registers(
        spi,
        gpio,
        NULL,
        NULL,
        &flg_status
    );
    if ((flg_status & STATUS_REGISTER_3_BUSY)) {
        return(WRITE_NO_DONE);
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
        // write enable(0)
        cmd[0] = WRITE_ENABLE;
        gpio[GPCLR0] =  (1 << 22);
        spi_write(spi, NULL, cmd, 1);
        gpio[GPSET0] =  (1 << 22);
        (*status) = WRITE_DISABLE;
    } else if ((*status) == WRITE_DISABLE) {
        if (flg_status & STATUS_REGISTER_3_WEL) {
            // program load 
            cmd[0] = RANDOM_PG_DATA_LOAD;
            cmd[1] = 0;
            cmd[2] = 0;
            if (wbflen > (sizeof(cmd) - 3)) {
                return(WRITE_EXCEPTION);
            }
            memcpy(
                &cmd[3],
                wbf,
                wbflen
            );
            gpio[GPCLR0] =  (1 << 22);
            spi_write(spi, NULL, cmd, wbflen + 3);
            gpio[GPSET0] =  (1 << 22);
            (*status) = RANDOM_PG_DATA_LOAD;
        }
    } else if ((*status) == RANDOM_PG_DATA_LOAD) {
        // write enable(1)
        cmd[0] = WRITE_ENABLE;
        gpio[GPCLR0] =  (1 << 22);
        spi_write(spi, NULL, cmd, 1);
        gpio[GPSET0] =  (1 << 22);
        (*status) = PG_EXECUTE;
    } else if ((*status) == PG_EXECUTE) {
        if (flg_status & STATUS_REGISTER_3_WEL) {
            // program execute 
            cmd[0] = PG_EXECUTE;
            cmd[1] = 0;
            cmd[2] = (uint8_t)((block >> 2) & 0x00FF);
            cmd[3] = (uint8_t)((block << 6) & 0x00C0);
            cmd[3] |= (uint8_t)(page & 0x003F);
            gpio[GPCLR0] =  (1 << 22);
            spi_write(spi, NULL, cmd, 4);
            gpio[GPSET0] =  (1 << 22);
            (*status) = DEVICE_RESET;
            return(WRITE_DONE);
        }
    }
    return(WRITE_NO_DONE);
}


