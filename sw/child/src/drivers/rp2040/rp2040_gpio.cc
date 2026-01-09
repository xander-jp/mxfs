/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of GPIO operation for rp2040 env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "rp2040_def.h"

int mxfs_driver_create_gpio(
    mxfs_inst_ptr inst
)
{
    DRIVER_DEBUG("mxfs_driver_create_gpio(%p)", inst);
    // CSPIN -> high
    gpio_init(CS_PIN);
    gpio_set_dir(CS_PIN, GPIO_OUT);
    gpio_put(CS_PIN, 1);

    // WPPIN -> must be low
    gpio_init(WP_PIN);
    gpio_set_dir(WP_PIN, GPIO_OUT);
    gpio_put(WP_PIN, 0);

    // HLD_PIN -> must be high
    gpio_init(HLD_PIN);
    gpio_set_dir(HLD_PIN, GPIO_OUT);
    gpio_put(HLD_PIN, 1);

    // blink initialization.(GPIO)
    for(int n = 0; n < 5; n++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(100);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(100);
    }
    DRIVER_INFO("mxfs_driver_create_gpio..");

    return(OK);
}

int mxfs_driver_release_gpio(
    mxfs_inst_ptr inst
)
{
    DRIVER_INFO("mxfs_driver_release_gpio..");

    return(OK);
}

