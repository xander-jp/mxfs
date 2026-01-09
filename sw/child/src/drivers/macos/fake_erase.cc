/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of erase operation for macOS env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "fake_def.h"

int mxfs_driver_erase(
    mxfs_inst_ptr inst,
    uint16_t block
)
{
    if (!fakeflashmem_instance) {
        return(ERR);
    }
    for(auto page = 0; page < PAGE_COUNT; page++) {
        memset(
            fakeflashmem_instance->blocks[block].pages[page].data,
            0xFF,
            PAGE_SIZE
        );
    }
    return(OK);
}

