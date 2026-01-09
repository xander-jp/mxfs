/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of SPI operation for macOS env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "fake_def.h"

int mxfs_driver_create_spi(
    mxfs_inst_ptr inst
)
{
    // fake memory
    fakeflashmem_instance = (fakeflashmem_ptr)malloc(sizeof(fakeflashmem_t));
    if (!fakeflashmem_instance) {
        return(ERR);
    }
    memset(fakeflashmem_instance, 0xFF/* valid */, sizeof(fakeflashmem_t));
    TEST_INFO("generated fake memory..(%p:%lu)", fakeflashmem_instance, sizeof(fakeflashmem_t));
    return(OK);
}

int mxfs_driver_release_spi(
    mxfs_inst_ptr inst
)
{
    TEST_INFO("release fake memory..(%p)\n", fakeflashmem_instance);
    // fake memory
    if (fakeflashmem_instance) {
        free(fakeflashmem_instance);
    }
    fakeflashmem_instance = NULL;

    return(OK);
}

