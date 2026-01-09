/******************************************************************************/
/*! @brief      Unit tests for GPIO pin control
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "spinand_def.h"
#include "gtest/gtest.h"

extern void init_gpio_spi(int* pspi, int* pgpio, uint32_t** ppgpio_mem);
extern void release_gpio_spi(int spi, int gpio, uint32_t* gpio_mem);


TEST(__GPIO, WP_HOLD_CS) {
    uint32_t* gpio_mem = NULL;
    int spi, gpio;
    init_gpio_spi(&spi, &gpio, &gpio_mem);

    if (gpio_mem) {
        // WP   -> out(GPIO:5), Low
        gpio_mem[GPFSEL0] &=~(7 << 15);
        gpio_mem[GPFSEL0] |= (1 << 15);
        gpio_mem[GPCLR0]  =  (1 << 5);  // setup low,0
        usleep(1000);
        uint32_t lvl0 = gpio_mem[GPLEV0];
        EXPECT_EQ(lvl0&(1<<5), 0);      // must be low,0

        // HOLD -> out(GPIO:6), High
        gpio_mem[GPFSEL0] &=~(7 << 18);
        gpio_mem[GPFSEL0] |= (1 << 18); // output
        gpio_mem[GPSET0]  =  (1 << 6);  // high
        usleep(1000);
        lvl0 = gpio_mem[GPLEV0];
        EXPECT_EQ(lvl0&(1<<6), (1<<6)); // must be high

        // CS   -> out(GPIO:22)
        gpio_mem[GPFSEL2] &=~(7 << 6);
        gpio_mem[GPFSEL2] |= (1 << 6);     // output
        gpio_mem[GPSET0]  =  (1 << 22);    // high
        usleep(1000);
        lvl0 = gpio_mem[GPLEV0];
        EXPECT_EQ(lvl0&(1<<22), (1<<22)); // must be high
    }
    release_gpio_spi(spi, gpio, gpio_mem);
}



