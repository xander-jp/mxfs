/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of erase operation.
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "drivers_def.h"

int mxfs_erase(
    mxfs_inst_ptr inst,
    uint16_t block
)
{
    return(
        mxfs_driver_erase(
            inst,
            block)); 
}

