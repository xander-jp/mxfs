/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of read operation for rp2040 env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "rp2040_def.h"

int mxfs_driver_read(
    mxfs_inst_ptr inst,
    uint16_t block,
    uint16_t page,
    uint8_t* buffer,
    uint16_t bufferlen
)
{
    uint8_t flg_status = 0xFF, flg_protected = 0xFF;
    DRIVER_DEBUG("mxfs_driver_read(%4u: %4u)", block, page);

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
    spi_read_registers(
        &flg_protected,
        NULL,
        NULL 
    );
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
    
    uint8_t cmd[32] = { 0x00, };
    // read mode
    // ECC-E bit to 0.
    cmd[0] = WRITE_STATUS_REGISTER_L;
    cmd[1] = STATUS_REGISTER_2_ADDR;
    cmd[2] = flg_protected;
    cmd[2] &= ~STATUS_REGISTER_2_ECC_E;
    cmd[2] |= STATUS_REGISTER_2_BUF;
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
    // program read
    cmd[0] = PG_DATA_READ;
    cmd[1] = 0;
    cmd[2] = (uint8_t)((block >> 2) & 0x00FF);
    cmd[3] = (uint8_t)((block << 6) & 0x00C0);
    cmd[3] |= (uint8_t)((page) & 0x003F);
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
    // read
    cmd[0] = READ;
    cmd[1] = 0;
    cmd[2] = 0;
    cmd[3] = 0;

    gpio_put(CS_PIN, 0);
    spi_write_blocking(SPI, cmd, 4);
    spi_read_blocking(SPI, 0, buffer, bufferlen);
    gpio_put(CS_PIN, 1);
#if 0
    if (block < 2) {
        printf("<<<< read <<<<\n");
        for (auto n = 0; n < bufferlen; n++) {
            printf("%02X ", buffer[n]);
            if ((n+1) % 32  == 0) {
                printf("\n");
            } else if ((n+1) % 8 == 0) {
                printf(":");
            }
        }
        printf("\n>>>>\n");
    }
#endif
    return(OK);
}

