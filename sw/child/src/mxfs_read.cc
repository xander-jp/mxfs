/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of read operation.
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "drivers_def.h"

int mxfs_read(
    mxfs_inst_ptr inst,
    uint8_t layer,
    uint8_t* buffer,
    uint16_t bufferlen,
    mxfs_timerange_ptr timerange
)
{
    uint32_t result_offset = 0;
    int ret = OK;

    if (!bufferlen || !buffer) {
        return(ERR);
    }
    // preparing read buffer on MCU
#ifndef __RP2040__
    uint8_t* rbf = (uint8_t*)MALLOC(PAGE_SIZE);
#else
    static uint8_t __read_buffer[PAGE_SIZE] = { 0x00, };
    uint8_t* rbf = __read_buffer;
#endif
    if (!rbf) {
        return(ERR);
    }
    //
    if (mxfs_driver_read(
            inst,
            timerange->block,
            timerange->page,
            rbf,
            PAGE_SIZE) != OK)
    {
#ifndef __RP2040__
        FREE(rbf);
#endif
        return(ERR);
    }

    for(auto offset = 0; offset < PAGE_SIZE;) {
        compress_ptr cp = (compress_ptr)&rbf[offset];
        
        if (cp->magic != MAGIC) {
            DRIVER_INFO(
                "magic ..(%08X) offset: %d", cp->magic, offset
            );
            break;
        }
        offset += sizeof(*cp);
        if (cp->type == LAYER_ANTP_SPEED ||
            cp->type == LAYER_ANTP_POWER ||
            cp->type == LAYER_ANTP_CADENCE)
        {
            if (cp->type == layer) {
                // no-compress
                memcpy(
                    &buffer[result_offset],
                    (cp+1),
                    ANTP_RECORD_SIZE
                );
                DRIVER_DEBUG(
                    "mxfs_read(ANTP[SPEED,POWER,CADENCE]) : [%d : %d , %4u, %4u, %4u]"
                    "%02X %02X %02X %02X",
                    cp->type,
                    layer,
                    offset,
                    result_offset,
                    bufferlen,
                    buffer[result_offset + 0],
                    buffer[result_offset + 1],
                    buffer[result_offset + 2],
                    buffer[result_offset + 3]
                );
                result_offset += ANTP_RECORD_SIZE;
            }
            offset += cp->ant.length;
        } else if (cp->type == LAYER_GYRO ||
                    cp->type == LAYER_ACCELEROMETER)
        {
            if (cp->type == layer) {
                for(auto n = 0; n < cp->gyr.count; n++) {
                    memcpy(
                        &buffer[result_offset],
                        &(((int8_t*)(cp+1))[n*GYRO_RECORD_SIZE]),
                        GYRO_RECORD_SIZE
                    );
                    result_offset += GYRO_RECORD_SIZE;
                }
            }
            offset += (cp->gyr.count * GYRO_RECORD_SIZE);
        } else if (cp->type == LAYER_TEMPERATURE)
        {
            if (cp->type == layer) {
                for(auto n = 0; n < cp->tmp.count; n++) {
                    memcpy(
                        &buffer[result_offset],
                        &(((int16_t*)(cp+1))[n]),
                        TEMPERATURE_RECORD_SIZE
                    );
                    result_offset += TEMPERATURE_RECORD_SIZE;
                }
            }
            offset += (cp->tmp.count * TEMPERATURE_RECORD_SIZE);
        } else if (cp->type == LAYER_CONFIG)
        {
            if (cp->type == layer) {
                // json string
                memcpy(
                    &buffer[result_offset],
                    (cp+1),
                    MIN(cp->tmp.base, bufferlen)
                );
                result_offset += MIN(cp->tmp.base, bufferlen);
            }
            // simple json string, a config-json can be included per page.
            offset += cp->tmp.base;
            break;
        }
    }
    ret = result_offset;
    DRIVER_DEBUG("read(%u : %d)", result_offset, ret);
#ifndef __RP2040__
    FREE(rbf);
#endif
    return(ret);
}
