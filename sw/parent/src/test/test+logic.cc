/******************************************************************************/
/*! @brief      Unit tests for SPI and GPIO initialization
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "spinand_def.h"
#include "gtest/gtest.h"

// global variables
extern int __testing_bytes;

// file local variables,
static int __reference_counter = 0;
static int __spi = 0;
static int __gpio = 0;
static uint32_t* __gpio_mem = NULL;


void init_spi(int spi) {
    uint32_t wr_mode = 0;
    uint8_t bits_per_word = SPI_BITS;
    uint32_t max_speed = SPI_SPEED;
    ASSERT_EQ(ioctl(spi, SPI_IOC_WR_MODE32, &wr_mode) >= 0, true);
    ASSERT_EQ(ioctl(spi, SPI_IOC_RD_MODE32, &wr_mode) >= 0, true);
    ASSERT_EQ(ioctl(spi, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) >= 0, true);
    ASSERT_EQ(ioctl(spi, SPI_IOC_RD_BITS_PER_WORD, &bits_per_word) >= 0, true);
    ASSERT_EQ(ioctl(spi, SPI_IOC_WR_MAX_SPEED_HZ, &max_speed) >= 0, true);
    ASSERT_EQ(ioctl(spi, SPI_IOC_RD_MAX_SPEED_HZ, &max_speed) >= 0, true);
}


void init_gpio_spi(int* pspi, int* pgpio, uint32_t** ppgpio_mem) {
    __reference_counter++;
    if (__spi > 0 && __gpio > 0 && __gpio_mem != NULL) {
        (*pspi) = __spi;
        (*pgpio) = __gpio;
        (*ppgpio_mem) = __gpio_mem;
        return;
    }
    __gpio = open(GPIO, O_RDWR|O_SYNC);
    ASSERT_EQ(__gpio>0, true);
    if (__gpio > 0) {
    	__gpio_mem = (uint32_t*)mmap(NULL, GPIO_MASK, PROT_READ|PROT_WRITE, MAP_SHARED, __gpio, GPIO_BASE);
    }
    __spi = open(SPI0, O_RDWR);
    ASSERT_EQ(__spi>0, true);
    if (__spi > 0 && __gpio > 0 && __gpio_mem != NULL) {
        init_spi(__spi);
        // software reset.
        uint8_t cmd[8] = { DEVICE_RESET, };
        __gpio_mem[GPCLR0] =  (1 << 22);
        spi_write(__spi, "reset", cmd, 1);
        __gpio_mem[GPSET0] =  (1 << 22);

        usleep(1000);

        // device id
        cmd[0] = JEDEC_ID;
        cmd[1] = 0;
        __gpio_mem[GPCLR0] =  (1 << 22);
        spi_write(__spi, "device id", cmd, 2);
        memset(cmd, 0, sizeof(cmd));
        spi_read(__spi, cmd, 3);
        __gpio_mem[GPSET0] =  (1 << 22);

        ASSERT_EQ(cmd[0], 0xEF);
        ASSERT_EQ(cmd[1], 0xAA);
        ASSERT_EQ(cmd[2], 0x21);
        printf(
            "device(%02X %02X %02X)\n",
            cmd[0],
            cmd[1],
            cmd[2]
        );
        (*pspi) = __spi;
        (*pgpio) = __gpio;
        (*ppgpio_mem) = __gpio_mem;
    }
}

void release_gpio_spi(int spi, int gpio, uint32_t* gpio_mem) {
    __reference_counter--;
    if (__reference_counter == 0){
        close(__spi);
        __spi = 0;
        close(__gpio);
        __gpio = 0;
        munmap(__gpio_mem, GPIO_MASK);
        __gpio_mem = NULL;
    }
}

void write_enable_un_protect(int spi, uint32_t* gpio_mem) {
    uint8_t cmd[8] = { 0, };
    uint8_t protect = 0xFF, config = 0xFF, status = 0xFF;
    // 
    while(1) {
        spi_read_registers(
            spi,
            gpio_mem,
            &protect,
            &config,
            &status
        );
        if ((status & STATUS_REGISTER_3_BUSY)) {
            usleep(10);
            continue; 
        }
        // un-protect
        cmd[0] = WRITE_STATUS_REGISTER_L;
        cmd[1] = STATUS_REGISTER_1_ADDR;
        cmd[2] = 0;
        gpio_mem[GPCLR0] =  (1 << 22);
        spi_write(spi, "un-protect", cmd, 3);
        gpio_mem[GPSET0] =  (1 << 22);

        break;
    }
    //
    while(1) {
        spi_read_registers(
            spi,
            gpio_mem,
            &protect,
            &config,
            &status
        );
        if ((status & STATUS_REGISTER_3_BUSY)) {
            usleep(10);
            continue; 
        }
        // write enable
        cmd[0] = WRITE_ENABLE;
        gpio_mem[GPCLR0] =  (1 << 22);
        spi_write(spi, "write enable", cmd, 1);
        gpio_mem[GPSET0] =  (1 << 22);

        break;
    }
}

void cleanup_block_page(int spi, uint32_t* gpio_mem, unsigned block, unsigned page) {
    uint8_t status = DEVICE_RESET;
    // clean test block.
    while(1) {
        int ret = erase_block(
            spi,
            gpio_mem,
            &status,
            block
        );
        if (ret == ERASE_DONE) { break; }
        usleep(10);
    }
}

void read_init_block(int spi, uint32_t* gpio_mem, unsigned block) {
    uint8_t status = DEVICE_RESET;
    // read initialize
    while(1) {
        int ret = read_initialize(
            spi,
            gpio_mem,
            &status,
            block
        );
        if (ret == READ_WITH_DATA) {
            break;
        } else if (ret == READ_INVALID_BLOCK) {
            ASSERT_TRUE("Invalid Block");
        }
        usleep(10);
    }
}


TEST(__LOGIC, InitializeAllRead) {
    int spi = 0, gpio = 0;
    uint32_t* gpio_mem = NULL;
    unsigned block = 0, page = 0;
    int ret;
    uint8_t status = DEVICE_RESET;

    init_gpio_spi(&spi, &gpio, &gpio_mem);

    // read initialize
    for (block = 0; block < 1024; block++) {
        read_init_block(spi, gpio_mem, block);
    }
    release_gpio_spi(spi, gpio, gpio_mem);
}

TEST(__LOGIC, EmptyPage) {
    // GTEST_SKIP() << "Skipping Empty Page.";
    int spi = 0, gpio = 0;
    uint32_t* gpio_mem = NULL;
    unsigned block = 971, page = 31;
    int ret;
    uint8_t status = DEVICE_RESET;

    init_gpio_spi(&spi, &gpio, &gpio_mem);
    cleanup_block_page(spi, gpio_mem, block, page);
    write_enable_un_protect(spi, gpio_mem);
    read_init_block(spi, gpio_mem, block);

    uint8_t pagebf[2048] = { 0x00,};
    page_ptr pg = (page_ptr)pagebf;
    pg->magic = MAGIC;
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t prev = ((uint64_t)tv.tv_sec * 1000000) + (uint64_t)(tv.tv_usec);

    // generate valid/empty page
    while(1) {
        ret = write_block_page(
            spi,
            gpio_mem,
            &status,
            block,
            page,
            pagebf,
            __testing_bytes
        );
        if (ret == WRITE_DONE) { break; }
        usleep(10);
    }
    gettimeofday(&tv, NULL);
    uint64_t now = ((uint64_t)tv.tv_sec * 1000000) + (uint64_t)(tv.tv_usec);

    printf("write (%2d Bytes) %8d\n", __testing_bytes, (int)(now - prev));

    // read valid/empty page, and test
    memset(pagebf, 0, sizeof(pagebf));
    while(1) {
        ret = read_by_block_page(
            spi,
            gpio_mem,
            &status,
            block,
            page,
            pagebf,
            __testing_bytes 
        );
        if (ret == READ_WITH_DATA) { break; }
        usleep(10);
    }
    gettimeofday(&tv, NULL);
    prev = ((uint64_t)tv.tv_sec * 1000000) + (uint64_t)(tv.tv_usec);
    
    printf("read (%2d Bytes) %8d\n", __testing_bytes, (int)(prev - now));
    pg = (page_ptr)pagebf;
    ASSERT_EQ(pg->magic, MAGIC);
    ASSERT_EQ(pg->flags, 0);

    release_gpio_spi(spi, gpio, gpio_mem);
}


