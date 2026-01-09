/******************************************************************************/
/*! @brief      SPI NAND flash read operations implementation
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "spinand_def.h"

// initialize
int read_initialize(
    int spi,
    uint32_t* gpio,
    uint8_t* status,
    uint16_t block
)
{
    uint8_t flg_protected = 0xFF, flg_status = 0xFF;
    uint8_t cmd[32] = { 0x00, };

    spi_read_registers(
        spi,
        gpio,
        NULL,
        NULL,
        &flg_status
    );
    if ((flg_status & STATUS_REGISTER_3_BUSY)) {
        return(READ_NO_DATA); 
    }

    if ((*status) == DEVICE_RESET) {
        spi_read_registers(
            spi,
            gpio,
            &flg_protected,
            NULL,
            NULL
        );
        // read mode 
        cmd[0] = WRITE_STATUS_REGISTER_L;
        cmd[1] = STATUS_REGISTER_2_ADDR;
        cmd[2] = flg_protected;
        cmd[2] &= ~STATUS_REGISTER_2_ECC_E;
        cmd[2] |= STATUS_REGISTER_2_BUF;
        gpio[GPCLR0] =  (1 << 22);
        spi_write(spi, NULL, cmd, 3);
        gpio[GPSET0] =  (1 << 22);
        (*status) = PG_DATA_READ;
    } else if ((*status) == PG_DATA_READ) {
        // program read 
        cmd[0] = PG_DATA_READ;
        cmd[1] = 0;
        cmd[2] = (uint8_t)((block >> 2) & 0x00FF);
        cmd[3] = (uint8_t)((block << 6) & 0x00C0);
        cmd[3] |= (uint8_t)(0);
        gpio[GPCLR0] =  (1 << 22);
        spi_write(spi, NULL, cmd, 4);
        gpio[GPSET0] =  (1 << 22);
        (*status) = READ;
    } else if ((*status) == READ) {
        // program read 
        cmd[0] = READ;
        cmd[1] = 0;
        cmd[2] = 0;
        cmd[3] = 0;
        gpio[GPCLR0] =  (1 << 22);
        spi_write(spi, NULL, cmd, 4);
        memset(cmd, 0, sizeof(cmd));
        spi_read(spi, cmd, 32);
        gpio[GPSET0] =  (1 << 22);
        (*status) = DEVICE_RESET;
        
        page_ptr pg = (page_ptr)cmd;
        if (pg->magic != 0xdeadc0de && pg->magic != 0xFFFFFFFF) {
            printf("invalid block(%d) %08x, read_initialize\n", block, pg->magic);
            // return(READ_INVALID_BLOCK);
            return(READ_WITH_DATA);
        } else {
            return(READ_WITH_DATA);
        }
    }
    return(READ_NO_DATA);
}

// read by block, page 
int read_by_block_page(
    int spi,
    uint32_t* gpio,
    uint8_t* status,
    uint16_t block,
    uint16_t page,
    uint8_t* rbf,
    uint16_t rbflen
)
{
    uint8_t flg_protected = 0xFF, flg_status = 0xFF;
    uint8_t cmd[32] = { 0x00, };
    spi_read_registers(
        spi,
        gpio,
        NULL,
        NULL,
        &flg_status
    );
    if ((flg_status & STATUS_REGISTER_3_BUSY)) {
        return(READ_NO_DATA);
    }
    if ((*status) == DEVICE_RESET) {
        spi_read_registers(
            spi,
            gpio,
            &flg_protected,
            NULL,
            NULL
        );
        // read mode 
        cmd[0] = WRITE_STATUS_REGISTER_L;
        cmd[1] = STATUS_REGISTER_2_ADDR;
        cmd[2] = flg_protected;
        cmd[2] &= ~STATUS_REGISTER_2_ECC_E;
        cmd[2] |= STATUS_REGISTER_2_BUF;
        gpio[GPCLR0] =  (1 << 22);
        spi_write(spi, NULL, cmd, 3);
        gpio[GPSET0] =  (1 << 22);
        (*status) = PG_DATA_READ;
    } else if ((*status) == PG_DATA_READ) {
        // program read 
        cmd[0] = PG_DATA_READ;
        cmd[1] = 0;
        cmd[2] = (uint8_t)((block >> 2) & 0x00FF);
        cmd[3] = (uint8_t)((block << 6) & 0x00C0);
        cmd[3] |= (uint8_t)((page) & 0x003F);
        gpio[GPCLR0] =  (1 << 22);
        spi_write(spi, NULL, cmd, 4);
        gpio[GPSET0] =  (1 << 22);
        (*status) = READ;
    } else if ((*status) == READ) {
        // program read 
        cmd[0] = READ;
        cmd[1] = 0;
        cmd[2] = 0;
        cmd[3] = 0;
        gpio[GPCLR0] =  (1 << 22);
        spi_write(spi, NULL, cmd, 4);
        spi_read(spi, rbf, rbflen);
        gpio[GPSET0] = (1 << 22);
        (*status) = DEVICE_RESET;
        
        page_ptr pg = (page_ptr)rbf;
        printf(
            ">> block : %d/ page : %d,"
            "magic: %08x, flags: %08x, time: %08x, fcs: %08x\n",
            block, page,
            pg->magic,
            pg->flags,
            pg->time,
            pg->fcs
        );
        if (pg->magic != 0xdeadc0de && pg->magic != 0xFFFFFFFF) {
            printf("invalid block(%d), %08X, read_by_block_page\n", block, pg->magic);
            return(READ_WITH_DATA);
            // return(READ_INVALID_BLOCK);
        } else {
            return(READ_WITH_DATA);
        }
    }
    return(READ_NO_DATA);
}


