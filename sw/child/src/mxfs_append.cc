/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of append operation.
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "drivers_def.h"

int mxfs_append(
    mxfs_inst_ptr inst,
    uint8_t layer,
    uint64_t tmm,
    uint8_t* buffer,
    uint16_t bufferlen
)
{
    // validate
    switch(layer) {
    case LAYER_ANTP_SPEED:
    case LAYER_ANTP_POWER:
    case LAYER_ANTP_CADENCE:
        if (bufferlen != ANTP_RECORD_SIZE) { return(ERR); }
        break;
    case LAYER_GYRO:
    case LAYER_ACCELEROMETER:
        if (bufferlen != GYRO_RECORD_SIZE) { return(ERR); }
        break;
    case LAYER_TEMPERATURE:
        if (bufferlen != TEMPERATURE_RECORD_SIZE) { return(ERR); }
        break;
    default:
        return(ERR);
    }
    uint16_t current_length = 0;
    for(int n = LAYER_MIN; n < LAYER_MAX; n++) {
        switch(n) {
        case LAYER_ANTP_SPEED:
        case LAYER_ANTP_POWER:
        case LAYER_ANTP_CADENCE:
            // no-compression
            current_length += (inst->mxfs_count[n] * ANTP_RECORD_SIZE);
            current_length += (sizeof(compress_t) * inst->mxfs_count[n]);
            break;
        case LAYER_GYRO:
        case LAYER_ACCELEROMETER:
            // no-compression
            current_length += (inst->mxfs_count[n] * LAYER_DIM_MAX * sizeof(int16_t));
            if (inst->mxfs_count[n] > 0) {
                current_length += sizeof(compress_t);
            }
            break;
        case LAYER_TEMPERATURE:
            // no-compression
            current_length += (inst->mxfs_count[n] * sizeof(int16_t));
            if (inst->mxfs_count[n] > 0) {
                current_length += sizeof(compress_t);
            }
            break;
        }
    }
    current_length += bufferlen;
    if (current_length > (PAGE_SIZE - 64)) {
        DRIVER_INFO(
            "auto flush block: %u, page: %u, (%u, %u), time:%llu",
            inst->mxfs_current_block,
            inst->mxfs_current_page,
            current_length,
            bufferlen,
            tmm 
        );

        mxfs_flush(
            inst
        );
        inst->mxfs_length[layer] = 0;
        inst->mxfs_count[layer] = 0;
    }
    // buffering with timestamp
    uint32_t lpos = inst->mxfs_length[layer];
    if ((lpos + sizeof(tmm) + bufferlen) > MXFS_BUFFER_SIZE) {
        DRIVER_ERROR(
            "need more buffer, pos: %4u, count: %4u/len: %4u",
            lpos, inst->mxfs_count[layer], bufferlen
        );
        return(NEED_MORE_BUFFER);
    }
    DRIVER_DEBUG(
        ">> layer: %2u/time: %llu , pos: %4u, count: %4u/len: %4u",
        layer, tmm, lpos, inst->mxfs_count[layer], bufferlen
    );
    memcpy(
        &inst->mxfs_buffer[layer][lpos],
        &tmm,
        sizeof(tmm)
    );
    lpos += sizeof(tmm);
    memcpy(
        &inst->mxfs_buffer[layer][lpos],
        buffer,
        bufferlen
    );
    inst->mxfs_length[layer] += sizeof(tmm);
    inst->mxfs_length[layer] += bufferlen;
    inst->mxfs_count[layer]++;
    return(OK);
}
