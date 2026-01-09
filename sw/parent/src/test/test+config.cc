/******************************************************************************/
/*! @brief      Unit tests for configuration storage operations
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

static const char* CONFIG_JSON = "\
{\
   network: { wifi_ssid: xxxx-yyyy-zzzz, cplane_host: 192.168.0.123, cplane_port: 50012, },\
   device: { id: mxfs-00, }, \
   sensors: {\
     speed: {id:1234, circumference: 2096, },\
     power: {id: 3456, offset: 100, },\
     cadence: {id: 4567, },\
   },\
}\
";

TEST(__CONFIG, DummySetup) {
    int spi = 0, gpio = 0;
    uint32_t* gpio_mem = NULL;
    int ret;
    uint8_t status = DEVICE_RESET;

    init_gpio_spi(&spi, &gpio, &gpio_mem);
    cleanup_block_page(spi, gpio_mem, 0, 0);
    write_enable_un_protect(spi, gpio_mem);
    read_init_block(spi, gpio_mem, 0);

    uint8_t pagebf[2048] = { 0x00,};
    page_ptr pg = (page_ptr)pagebf;
    pg->magic = MAGIC;
    pg->flags = 0xFFFFFFFF;

    // write config 
    snprintf(
        (char*)&pagebf[sizeof(*pg)],
        sizeof(pagebf) - (1 + sizeof(*pg)),
        "%s",
        CONFIG_JSON
    );
    status = DEVICE_RESET;
    while(1) {
        ret = write_block_page(
            spi,
            gpio_mem,
            &status,
            0,
            0,
            pagebf,
            sizeof(pagebf) 
        );
        if (ret == WRITE_DONE) { break; }
        usleep(10);
    }

    // read config 
    status = DEVICE_RESET;
    memset(pagebf, 0, sizeof(pagebf));
    while(1) {
        ret = read_by_block_page(
            spi,
            gpio_mem,
            &status,
            0,
            0,
            pagebf,
            sizeof(pagebf)
        );
        if (ret == READ_WITH_DATA) { break; }
        usleep(10);
    }
    pg = (page_ptr)pagebf;
    ASSERT_EQ(pg->magic, MAGIC);
    ASSERT_EQ(pg->flags, 0xFFFFFFFF);
    ASSERT_EQ(memcmp(CONFIG_JSON, &pagebf[sizeof(*pg)], strlen(CONFIG_JSON)), 0);
    std::cout << (const char*)&pagebf[sizeof(*pg)] << std::endl;
    
    release_gpio_spi(spi, gpio, gpio_mem);
}


