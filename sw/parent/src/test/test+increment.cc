/******************************************************************************/
/*! @brief      Unit tests for sequential page write operations
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "spinand_def.h"
#include "gtest/gtest.h"

extern void init_gpio_spi(int* pspi, int* pgpio, uint32_t** ppgpio_mem);
extern void release_gpio_spi(int spi, int gpio, uint32_t* gpio_mem);
extern void write_enable_un_protect(int spi, uint32_t* gpio_mem);
extern void cleanup_block_page(int spi, uint32_t* gpio_mem, unsigned block, unsigned page);
extern void read_init_block(int spi, uint32_t* gpio_mem, unsigned block);

TEST(__LOGIC, NextPageSameBlock) {
    int spi = 0, gpio = 0;
    uint32_t* gpio_mem = NULL;
    srand(time(NULL));
    unsigned rn = (unsigned)(rand() % 10);
    unsigned block = (971+rn), page = (31+rn);
    int ret;
    uint8_t status = DEVICE_RESET;

    init_gpio_spi(&spi, &gpio, &gpio_mem);
    cleanup_block_page(spi, gpio_mem, block, page);
    write_enable_un_protect(spi, gpio_mem);
    read_init_block(spi, gpio_mem, block);

    uint8_t pagebf[2048] = { 0x00,};
    page_ptr pg = (page_ptr)pagebf;
    pg->magic = MAGIC;

    // write, first page
    for (auto n = sizeof(*pg); n < sizeof(pagebf); n+=sizeof(uint16_t)) {
        uint16_t num(((rn*1000) + 10000+n));
        memcpy(
            &pagebf[n],
            &num,
            sizeof(num)
        );
    }
    status = DEVICE_RESET;
    while(1) {
        ret = write_block_page(
            spi,
            gpio_mem,
            &status,
            block,
            page,
            pagebf,
            sizeof(pagebf) 
        );
        if (ret == WRITE_DONE) { break; }
        usleep(10);
    }
    // write, second page
    for (auto n = sizeof(*pg); n < sizeof(pagebf); n+=sizeof(uint16_t)) {
        uint16_t num(((rn*1000) + 20000+n));
        memcpy(
            &pagebf[n],
            &num,
            sizeof(num)
        );
    }

    status = DEVICE_RESET;
    while(1) {
        ret = write_block_page(
            spi,
            gpio_mem,
            &status,
            block,
            page + 1,
            pagebf,
            sizeof(pagebf) 
        );
        if (ret == WRITE_DONE) { break; }
        usleep(10);
    }

    // test first page
    status = DEVICE_RESET;
    memset(pagebf, 0, sizeof(pagebf));
    while(1) {
        ret = read_by_block_page(
            spi,
            gpio_mem,
            &status,
            block,
            page,
            pagebf,
            sizeof(pagebf)
        );
        if (ret == READ_WITH_DATA) { break; }
        usleep(10);
    }
    pg = (page_ptr)pagebf;
    ASSERT_EQ(pg->magic, MAGIC);
    ASSERT_EQ(pg->flags, 0);
    for (auto n = sizeof(*pg); n < sizeof(pagebf); n+=sizeof(uint16_t)) {
        uint16_t num;
        memcpy(
            &num,
            &pagebf[n],
            sizeof(num)
        );
        ASSERT_EQ(num, (rn*1000) + (10000+n));
    }
    
    // test second page
    status = DEVICE_RESET;
    memset(pagebf, 0, sizeof(pagebf));
    while(1) {
        ret = read_by_block_page(
            spi,
            gpio_mem,
            &status,
            block,
            page + 1,
            pagebf,
            sizeof(pagebf)
        );
        if (ret == READ_WITH_DATA) { break; }
        usleep(10);
    }
    pg = (page_ptr)pagebf;
    ASSERT_EQ(pg->magic, MAGIC);
    ASSERT_EQ(pg->flags, 0);
    for (auto n = sizeof(*pg); n < sizeof(pagebf); n+=sizeof(uint16_t)) {
        uint16_t num;
        memcpy(
            &num,
            &pagebf[n],
            sizeof(num)
        );
        ASSERT_EQ(num, (rn*1000) + (20000+n));
    }

    release_gpio_spi(spi, gpio, gpio_mem);
}


