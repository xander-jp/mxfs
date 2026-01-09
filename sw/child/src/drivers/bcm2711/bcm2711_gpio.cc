/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of GPIO operation for BCM2711 env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "bcm2711_def.h"

int mxfs_driver_create_gpio(
    mxfs_inst_ptr inst
)
{
    inst->gpio = open(GPIO, O_RDWR|O_SYNC);
    if (inst->gpio > 0) {
    	inst->gpio_mem = (uint32_t*)mmap(NULL, GPIO_MASK, PROT_READ|PROT_WRITE, MAP_SHARED, inst->gpio, GPIO_BASE);
    } else {
        return(ERR);
    }
    printf("generated GPIO file-descriptor, BCM2711..(%p)\n", inst->gpio);

    // WP   -> out(GPIO:5), Low
    inst->gpio_mem[GPFSEL0] &=~(7 << 15);
    inst->gpio_mem[GPFSEL0] |= (1 << 15);    // output
    inst->gpio_mem[GPCLR0]  =  (1 << 5);     // low

    // HOLD -> out(GPIO:6), High
    inst->gpio_mem[GPFSEL0] &=~(7 << 18);
    inst->gpio_mem[GPFSEL0] |= (1 << 18);    // output
    inst->gpio_mem[GPSET0]  =  (1 << 6);     // high

    // CS   -> out(GPIO:22)
    inst->gpio_mem[GPFSEL2] &=~(7 << 6);
    inst->gpio_mem[GPFSEL2] |= (1 << 6);     // output
    inst->gpio_mem[GPSET0]  =  (1 << 22);    // high

    // SCLK -> (GPIO:11), ALT0
    inst->gpio_mem[GPFSEL1] &=~(7 << 3);
    inst->gpio_mem[GPFSEL1] |= (4 << 3);     // ALT0:SPI0_SCLK
    inst->gpio_mem[GPSET0]  =  (1 << 11);
    // MOSI -> (GPIO:10), ALT0
    inst->gpio_mem[GPFSEL1] &=~(7 << 0);
    inst->gpio_mem[GPFSEL1] |= (4 << 0);     // ALT0:SPI0_MOSI
    inst->gpio_mem[GPSET0]  =  (1 << 10);
    // MISO -> (GPIO:9), ALT0
    inst->gpio_mem[GPFSEL0] &=~(7 << 27);
    inst->gpio_mem[GPFSEL0] |= (4 << 27);    // ALT0:SPI0_MISO
    inst->gpio_mem[GPSET0]  =  (1 << 9);

    return(OK);
}

int mxfs_driver_release_gpio(
    mxfs_inst_ptr inst
)
{
    close(inst->gpio);
    munmap(inst->gpio_mem, GPIO_MASK);
    printf("close GPIO..(%p)\n", inst->gpio);

    return(OK);
}

