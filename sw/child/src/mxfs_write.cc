/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of write operation.
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "drivers_def.h"

int mxfs_write(
    mxfs_inst_ptr inst,
    uint8_t layer,
    uint8_t* buffer,
    uint16_t bufferlen,
    mxfs_timerange_ptr timerange
)
{
    if (!bufferlen || !buffer) {
        return(ERR);
    }
    if (layer != LAYER_DIRECT) {
        return(ERR);
    }
    // preparing write buffer on MCU
#ifndef __RP2040__
    uint8_t* wbf = (uint8_t*)MALLOC(PAGE_SIZE);
#else
    static uint8_t __write_buffer[PAGE_SIZE] = { 0x00, };
    uint8_t* wbf = __write_buffer;
#endif
    compress_ptr cp = (compress_ptr)wbf;
    cp->magic = MAGIC;
    cp->type = LAYER_DIRECT;
    cp->tmp.base = bufferlen;
    memcpy(
        (cp+1), 
        buffer,
        bufferlen 
    );

    int ret = mxfs_driver_write(
            inst,
            timerange->block,
            timerange->page,
            wbf,
            bufferlen + sizeof(*cp));
#ifndef __RP2040__
    FREE(wbf);
#endif
    return(ret);
}
