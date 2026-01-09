/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of instanciation operation.
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "drivers_def.h"

static int singleton = 0;
#ifndef __RP2040__
static pthread_mutex_t singleton_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

int mxfs_create_instance(
    mxfs_inst_ptr* inst,
    mxfs_callback callback
)
{
#ifndef __RP2040__
    pthread_mutex_lock(&singleton_lock);
#endif
    if (singleton != 0) {
        return(ERR);
    }
    singleton++;
    DRIVER_INFO("singleton %d", singleton);
#ifndef __RP2040__
    pthread_mutex_unlock(&singleton_lock);
#endif
    DRIVER_INFO("mxfs instance size: %u", sizeof(mxfs_inst_t));
    (*inst) = (mxfs_inst_ptr)MALLOC(sizeof(mxfs_inst_t));
    if (!(*inst)) {
        return(ERR);
    }
    memset((*inst), 0, sizeof((**inst)));
    (*inst)->mxfs_current_block = BLOCK_START_DATA_AREA;
    (*inst)->mxfs_current_page = 0;

    for(int n = LAYER_MIN; n < LAYER_MAX; n++) {
        (*inst)->mxfs_buffer[n] = (uint8_t*)MALLOC((int)MXFS_BUFFER_SIZE);
        if (!(*inst)->mxfs_buffer[n]) {
            PANIC("MALLOC(%p)", inst);
            return(ERR);
        }
        DRIVER_INFO(">>>> %p, %p", inst, *inst);
        DRIVER_INFO(
            "allocate first stage buffer(%p, %p, %p/ %d : %d)",
            inst, *inst,
            (*inst)->mxfs_buffer[n],
            n,
            (int)MXFS_BUFFER_SIZE
        );
        memset((*inst)->mxfs_buffer[n], 0xFF, MXFS_BUFFER_SIZE);
        (*inst)->mxfs_length[n] = 0;
        (*inst)->mxfs_count[n] = 0;
    }

    if (mxfs_driver_create_gpio((*inst)) != OK) {
        PANIC("mxfs_create_gpio(%p)", inst);
        return(ERR);
    }
    if (mxfs_driver_create_spi((*inst)) != OK) {
        PANIC("mxfs_create_spi(%p)", inst);
        return(ERR);
    }
    // is block, page available
    for (unsigned block = BLOCK_START_DATA_AREA; block < BLOCK_COUNT; block++) {
        uint8_t rbf[16] = { 0x00, };
        if (mxfs_driver_read((*inst), block, 0, rbf, sizeof(rbf)) != OK) {
            (*inst)->blocks[block].flags = BLOCK_INVALID;
            (*inst)->blocks[block].start_time = 0;
#ifdef __LINUX__
            (*inst)->blocks[block].pages[0].flags = PAGE_INVALID;
            (*inst)->mxfs_invalid_block ++;
#else
            memset(
                (*inst)->blocks[block].pages,
                0x00,
                sizeof((*inst)->blocks[block].pages)
            );
#endif
            continue;
        }
        compress_ptr cp = (compress_ptr)rbf;
        if (block < 2) {
            DRIVER_INFO(
                "block is %s(%4u), %08x, time: %u",
                (cp->magic==MAGIC?"invalid":"valid"),
                block,
                cp->magic,
                cp->time
            );
        }
        // avilable / un-available,
        (*inst)->blocks[block].flags = (cp->magic==MAGIC?BLOCK_INVALID:BLOCK_VALID);
        (*inst)->blocks[block].start_time = cp->time;
#ifdef __LINUX__
        (*inst)->blocks[block].pages[0].flags = PAGE_VALID;
#else
        memset(
            (*inst)->blocks[block].pages,
            0xFF,
            sizeof((*inst)->blocks[block].pages)
        );
#endif
        if (callback) {
            callback((*inst), block);
        }
    }
    // find current block, and page
    uint32_t start_tmm = 0;
    for (unsigned block = BLOCK_START_DATA_AREA; block < BLOCK_COUNT; block++) {
#ifdef __LINUX__
        if ((*inst)->blocks[block].pages[0].flags & PAGE_VALID) {
            if ((*inst)->blocks[block].pages[0].start_time > start_tmm) {
                start_tmm = (*inst)->blocks[block].pages[0].start_time;
                (*inst)->mxfs_current_block = block;
                (*inst)->mxfs_current_page = 0;
            }
        }
#else
        if ((*inst)->blocks[block].flags & BLOCK_INVALID) {
            if ((*inst)->blocks[block].start_time > start_tmm) {
                start_tmm = (*inst)->blocks[block].start_time;
                (*inst)->mxfs_current_block = block;
                (*inst)->mxfs_current_page = 0;
            }
        } else {
            (*inst)->mxfs_current_block = block;
            (*inst)->mxfs_current_page = 0;
            break;
        }
#endif
    }
    DRIVER_INFO(
        "current block: %u, page: %u, start: %u",
        (*inst)->mxfs_current_block,
        (*inst)->mxfs_current_page,
        start_tmm
    );
    return(OK);
}

int mxfs_release_instance(
    mxfs_inst_ptr inst
)
{
    if (!inst) {
        return(ERR);
    }
#ifndef __RP2040__
    pthread_mutex_lock(&singleton_lock);
#endif
    if (singleton == 0) {
        PANIC("singleton %d", singleton);
        return(ERR);
    }
    singleton--;
#ifndef __RP2040__
    pthread_mutex_unlock(&singleton_lock);
#endif
    if (mxfs_driver_release_gpio(inst) != OK) {
        PANIC("mxfs_release_gpio(%p)", inst);
        return(ERR);
    }
    if (mxfs_driver_release_spi(inst) != OK) {
        PANIC("mxfs_release_spi(%p)", inst);
        return(ERR);
    }
    for(int n = LAYER_MIN; n < LAYER_MAX; n++) {
        if (inst->mxfs_buffer[n]) {
            FREE(inst->mxfs_buffer[n]);
        }
        inst->mxfs_buffer[n] = NULL;
        inst->mxfs_length[n] = 0;
        inst->mxfs_count[n] = 0;
    }
    FREE(inst);

    return(OK);
}


