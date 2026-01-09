/******************************************************************************/
/*! @brief      mxfs filesystem driver interfaces
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#pragma once

#include "mxfs_def.h"

#ifndef MIN
#define MIN(a,b) (a<b?a:b)
#endif

extern "C" {
    int mxfs_driver_read(
        mxfs_inst_ptr inst,
        uint16_t block,
        uint16_t page,
        uint8_t* buffer,
        uint16_t bufferlen
    );
    int mxfs_driver_write(
        mxfs_inst_ptr inst,
        uint16_t block,
        uint16_t page,
        uint8_t* buffer,
        uint16_t bufferlen
    );
    int mxfs_driver_erase(
        mxfs_inst_ptr inst,
        uint16_t block
    );
    // SPI
    int mxfs_driver_create_spi(
        mxfs_inst_ptr inst
    );
    int mxfs_driver_release_spi(
        mxfs_inst_ptr inst
    );
    // GPIO
    int mxfs_driver_create_gpio(
        mxfs_inst_ptr inst
    );
    int mxfs_driver_release_gpio(
        mxfs_inst_ptr inst
    );
};
