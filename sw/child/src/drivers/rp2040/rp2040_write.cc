/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of write operation for rp2040 env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "rp2040_def.h"

int mxfs_driver_write(
    mxfs_inst_ptr inst,
    uint16_t block,
    uint16_t page,
    uint8_t* buffer,
    uint16_t bufferlen
)
{
    DRIVER_DEBUG("mxfs_driver_write(%p)", inst);
    uint8_t flg_status = 0xFF;
    uint8_t cmd[PAGE_SIZE + 64] = { 0x00, };
    
    if (bufferlen > (sizeof(cmd) - 3)) {
        DRIVER_ERROR("buffer over. %u > %u", bufferlen, sizeof(cmd));
        return(ERR);
    }
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
    // program load 
    cmd[0] = RANDOM_PG_DATA_LOAD;
    cmd[1] = 0;
    cmd[2] = 0;

    memcpy(
        &cmd[3],
        buffer,
        bufferlen
    );
    gpio_put(CS_PIN, 0);
    spi_write_blocking(SPI, cmd, PAGE_SIZE + 3);
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

#if 0
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
#endif
    // program execute 
    cmd[0] = PG_EXECUTE;
    cmd[1] = 0;
    cmd[2] = (uint8_t)((block >> 2) & 0x00FF);
    cmd[3] = (uint8_t)((block << 6) & 0x00C0);
    cmd[3] |= (uint8_t)(page & 0x003F);
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

