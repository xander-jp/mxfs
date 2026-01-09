/******************************************************************************/
/*! @brief      mxfs filesystem driver defined for macOS env
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#pragma once

#include "drivers_def.h"

/*
fake SPI/NAND Flash Memory
memory instanciate
*/

typedef struct fakepage {
    uint8_t data[PAGE_SIZE];
} fakepage_t, *fakepage_ptr;

typedef struct fakeblock {
    fakepage_t pages[PAGE_COUNT];
} fakeblock_t, *fakeblock_ptr;

typedef struct fakeflashmem {
    fakeblock_t blocks[BLOCK_COUNT];
} fakeflashmem_t, *fakeflashmem_ptr;

extern fakeflashmem_ptr fakeflashmem_instance;

