/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of write operation for macOS env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "fake_def.h"

int mxfs_driver_write(
    mxfs_inst_ptr inst,
    uint16_t block,
    uint16_t page,
    uint8_t* buffer,
    uint16_t bufferlen
)
{
    if (!fakeflashmem_instance) {
        return(ERR);
    }
    memcpy(
        fakeflashmem_instance->blocks[block].pages[page].data,
        buffer,
        MIN(bufferlen, PAGE_SIZE)
    );
    
    return(OK);
}

