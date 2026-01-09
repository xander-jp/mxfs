/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of read operation for macOS env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "fake_def.h"

// instanciate
fakeflashmem_ptr fakeflashmem_instance = NULL;

int mxfs_driver_read(
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
        buffer,
        fakeflashmem_instance->blocks[block].pages[page].data,
        MIN(bufferlen, PAGE_SIZE)
    );
    return(OK);
}

