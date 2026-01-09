/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of timerange operation.
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "drivers_def.h"

int mxfs_timerange(
    mxfs_inst_ptr inst,
    uint8_t layer,
    mxfs_timerange_ptr timerange,
    uint16_t timerangecount
)
{
    if (!timerange || !timerangecount) {
        return(ERR);
    }
    uint32_t flags[] = {
        PAGE_IN_ANTP_SPEED,
        PAGE_IN_ANTP_POWER,
        PAGE_IN_ANTP_CADENCE,
        PAGE_IN_GYRO,
        PAGE_IN_ACCELEROMETER,
        PAGE_IN_TEMPERATURE,
    };
    int cnt = 0;
    for (auto block = int(BLOCK_START_DATA_AREA); block < BLOCK_COUNT; block++) {
        for (auto page = 0; page < PAGE_COUNT; page++) {
#ifdef __LINUX__
            if (inst->blocks[block].pages[page].flags & (PAGE_VALID | flags[layer])) {
                if (cnt < timerangecount) {
                    timerange[cnt].block = block;
                    timerange[cnt].page = page;
                    timerange[cnt].start_time = inst->blocks[block].pages[page].start_time;
                    timerange[cnt].last_time = inst->blocks[block].pages[page].start_time;
                } else {
                    return(NEED_MORE_BUFFER);
                }
                cnt++;
            }
#else
            // first check if page has valid data by reading magic
            uint8_t rbf[16] = { 0x00, };
            if (mxfs_driver_read(inst, block, page, rbf, sizeof(rbf)) != OK) {
                continue;
            }
            compress_ptr cp = (compress_ptr)rbf;
            if (cp->magic != MAGIC) {
                continue;
            }
            // check page flags set by mxfs_flush
            if (inst->blocks[block].pages[page] & flags[layer]) {
                if (cnt < timerangecount) {
                    timerange[cnt].block = block;
                    timerange[cnt].page = page;
                    timerange[cnt].start_time = 0;
                    timerange[cnt].last_time = 0;
                } else {
                    return(NEED_MORE_BUFFER);
                }
                cnt++;
            }
#endif
        }
    }
    return(cnt);
}

int mxfs_timerange_by_block(
    mxfs_inst_ptr inst,
    uint8_t layer,
    uint16_t block,
    mxfs_timerange_ptr timerange,
    uint16_t timerangecount
)
{
    if (!timerange || !timerangecount) {
        return(ERR);
    }
    if (block < BLOCK_START_DATA_AREA) {
        return(ERR);
    }
    int cnt = 0;
#ifdef __LINUX__
    PANIC("not implemented.");
#endif
    for (auto page = 0; page < PAGE_COUNT; page++) {
        uint64_t last_time = 0;
        uint64_t start_time = 0;
        uint16_t record_count = 0;
        uint8_t rbf[2112] = { 0x00, };
        // BLOCK_INVALID -> used.( invalid using)
        if (inst->blocks[block].flags & BLOCK_INVALID) {
            if (mxfs_driver_read(inst, block, page, rbf, sizeof(rbf)) != OK) {
                PANIC("failed. mxfs_driver_read(%p)", inst);
                return(ERR);
            }
            for(auto offset = 0; offset < sizeof(rbf);) {
                compress_ptr cp = (compress_ptr)&rbf[offset];
                if (cp->magic != MAGIC) {
                    break;
                }
                offset += sizeof(*cp);
                DRIVER_DEBUG(
                    "(%02X : %02X) offset: %d", layer, cp->type, offset
                );
                //
                switch(cp->type) {
                case LAYER_ANTP_SPEED:
                case LAYER_ANTP_POWER:
                case LAYER_ANTP_CADENCE:
                    if (cp->type == layer) {
                        if (start_time == 0) {
                            start_time = ((((uint64_t)cp->time) * 1000) + (uint64_t)(cp->msec));
                        }
                        last_time = ((((uint64_t)cp->time) * 1000) + (uint64_t)(cp->msec));
                        record_count ++;
                    }
                    offset += cp->ant.length;
                    break;
                case LAYER_GYRO:
                case LAYER_ACCELEROMETER:
                    if (cp->type == layer) {
                        if (start_time == 0) {
                            start_time = (((cp->time * 1000) + cp->msec));
                        }
                        last_time = (((cp->time * 1000) + cp->msec)) +
                                    ((uint64_t)cp->gyr.count * (uint64_t)cp->step);
                        record_count += cp->gyr.count;
                    }
                    offset += (cp->gyr.count * sizeof(int16_t) * 3);
                    break;
                case LAYER_TEMPERATURE:
                    if (cp->type == layer) {
                        if (start_time == 0) {
                            start_time = (((cp->time * 1000) + cp->msec));
                        }
                        last_time = (((cp->time * 1000) + cp->msec)) +
                                    ((uint64_t)cp->tmp.count * (uint64_t)cp->step);
                        record_count += cp->tmp.count;
                    }
                    offset += (cp->tmp.count * sizeof(int16_t));
                    break;
                }
            }
        }
        if (record_count > 0) {
            if (cnt < timerangecount) {
                timerange[cnt].block = block;
                timerange[cnt].page = page;
                timerange[cnt].start_time = start_time;
                timerange[cnt].last_time = last_time;
                timerange[cnt].record_count = record_count;
            } else {
                return(NEED_MORE_BUFFER);
            }
            cnt++;
        }
    }
    return(cnt);
}

