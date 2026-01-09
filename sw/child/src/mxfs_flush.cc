/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of flush operation.
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "drivers_def.h"

static uint8_t _work_buffer[PAGE_SIZE];

int mxfs_flush(
    mxfs_inst_ptr inst
)
{
    uint64_t tmm, start_tmm = 0, last_tmm = 0;
    uint16_t current_length = 0;
    uint16_t gyro_count = 0;
    uint16_t antp_count = 0;

    for(int n = LAYER_MIN; n < LAYER_MAX; n++) {
        switch(n) {
        case LAYER_ANTP_SPEED:
        case LAYER_ANTP_POWER:
        case LAYER_ANTP_CADENCE:
            current_length += (ANTP_RECORD_SIZE * inst->mxfs_count[n]);
            antp_count += inst->mxfs_count[n];
            break;
        case LAYER_GYRO:
        case LAYER_ACCELEROMETER:
            current_length += (inst->mxfs_count[n] * LAYER_DIM_MAX * sizeof(int16_t));
            gyro_count += inst->mxfs_count[n];
            break;
        case LAYER_TEMPERATURE:
            current_length += (inst->mxfs_count[n] * sizeof(int16_t));
            gyro_count += inst->mxfs_count[n];
            break;
        }        
    }
    if (current_length == 0) {
        return(OK);
    }
    // preparing write buffer on MCU
    uint8_t* work_buffer = _work_buffer;
    uint16_t work_len = 0;
    uint32_t page_flags = 0;
    uint32_t page_flags_array[] = {
        PAGE_IN_ANTP_SPEED, PAGE_IN_ANTP_POWER, PAGE_IN_ANTP_CADENCE,
        PAGE_IN_GYRO, PAGE_IN_ACCELEROMETER, PAGE_IN_TEMPERATURE,
    };
    if (!work_buffer) {
        return(ERR);
    }
    // -------------
    // copy to write-buffer in MCU, ANTP - Spped, Power, Cadence
    for(uint32_t layer = LAYER_ANTP_SPEED; layer < LAYER_ANTP_MAX; layer++) {
        for(auto n = 0; n < inst->mxfs_length[layer]; n+=(ANTP_RECORD_SIZE+sizeof(tmm))) {
            page_flags |= page_flags_array[layer];
            memcpy(
                &tmm,
                &inst->mxfs_buffer[layer][n + 0],
                sizeof(tmm)
            );
            if (!start_tmm) { start_tmm = tmm; }
            last_tmm = tmm;
            compress_t cp{ (uint32_t)MAGIC, (uint32_t)(tmm/1000), 0, (uint32_t)layer, };
            // 10bit
            cp.msec = ((uint32_t)(tmm%1000)) & 0x03FF;
            cp.ant.length = ANTP_RECORD_SIZE;
            memcpy(
                work_buffer + work_len,
                &cp,
                sizeof(cp)
            );
            work_len += sizeof(cp);
            memcpy(
                work_buffer + work_len,
                &inst->mxfs_buffer[layer][n + sizeof(tmm)],
                ANTP_RECORD_SIZE
            );
            work_len += ANTP_RECORD_SIZE;
        }
        inst->mxfs_length[layer] = 0;
        inst->mxfs_count[layer] = 0;
    }
    // -------------
    // copy to write-buffer in MCU - Gyro ,Accelerometer
    for(int layer = LAYER_GYRO; layer < LAYER_DIM3_MAX; layer++) {
        if (inst->mxfs_length[layer] > 0) {
            uint32_t step = (GYRO_RECORD_SIZE+sizeof(tmm));
            page_flags |= page_flags_array[layer];
            memcpy(
                &tmm,
                &inst->mxfs_buffer[layer][0],
                sizeof(tmm)
            );
            for(auto n = 0; n < inst->mxfs_length[layer]; n+=step) {
                memcpy(
                    &last_tmm,
                    &inst->mxfs_buffer[layer][n],
                    sizeof(last_tmm)
                );
            }
            DRIVER_INFO(
                "[gyro(%d)] --(%u) time: %llu - %llu, len: %u, count: %u, work_len: %u, time_step: %u(debug:%u)",
                layer,
                step,
                tmm, last_tmm,
                inst->mxfs_length[layer],
                inst->mxfs_count[layer],
                work_len,
                (uint32_t)((last_tmm - tmm) / (inst->mxfs_length[layer]/step)) & 0x3FF
            );
            // header for gyro, accelerometer
            compress_t cp{ (uint32_t)MAGIC, (uint32_t)(tmm/1000), 1, (uint32_t)layer, };
            cp.msec = (uint32_t)(tmm%1000) & 0x03FF;
            cp.step = (uint32_t)((last_tmm - tmm) / (inst->mxfs_length[layer]/step)) & 0xFFFF;
            if ((uint32_t)((last_tmm - tmm) / (inst->mxfs_length[layer]/step)) > 0xFFFF) {
                cp.step = 0xFFFF;
            }
            cp.gyr.dim = 3;
            cp.gyr.count = (inst->mxfs_length[layer]/step);
            cp.gyr.base = 0;

            memcpy(
                work_buffer + work_len,
                &cp,
                sizeof(cp)
            );
            work_len += sizeof(cp);

            for(auto n = 0; n < inst->mxfs_length[layer]; n+=step) {
                memcpy(
                    work_buffer + work_len,
                    &inst->mxfs_buffer[layer][n + sizeof(tmm)],
                    GYRO_RECORD_SIZE
                );
                DRIVER_DEBUG(
                    "\t[tmp]offset: %d, work_len: %u",
                    n, work_len
                );
                work_len += GYRO_RECORD_SIZE;
            }
            inst->mxfs_length[layer] = 0;
            inst->mxfs_count[layer] = 0;
        }
    }
    // -------------
    // copy to write-buffer in MCU - Temperature
    if (inst->mxfs_length[LAYER_TEMPERATURE] > 0) {
        uint32_t step = (TEMPERATURE_RECORD_SIZE+sizeof(tmm));
        page_flags |= page_flags_array[LAYER_TEMPERATURE];
        memcpy(
            &tmm,
            &inst->mxfs_buffer[LAYER_TEMPERATURE][0],
            sizeof(tmm)
        );
        for(auto n = step; n < inst->mxfs_length[LAYER_TEMPERATURE]; n+=step) {
            memcpy(
                &last_tmm,
                &inst->mxfs_buffer[LAYER_TEMPERATURE][n],
                sizeof(last_tmm)
            );
        }
        DRIVER_INFO(
            "[temp] --(%u) time: %llu - %llu, len: %u, count: %u, work_len: %u, time_step: %u(debug:%u)",
            step,
            tmm, last_tmm,
            inst->mxfs_length[LAYER_TEMPERATURE],
            inst->mxfs_count[LAYER_TEMPERATURE],
            work_len,
            (uint32_t)((last_tmm - tmm) / (inst->mxfs_length[LAYER_TEMPERATURE]/step)) & 0x3FF
        );
        // header for temperature
        compress_t cp{ (uint32_t)MAGIC, (uint32_t)(tmm/1000), 1, (uint32_t)LAYER_TEMPERATURE, };
        cp.msec = (uint32_t)(tmm%1000) & 0x03FF;
        cp.tmp.count = (inst->mxfs_length[LAYER_TEMPERATURE]/step);
        cp.tmp.base = 0;
        cp.step = (uint32_t)((last_tmm - tmm) / (inst->mxfs_length[LAYER_TEMPERATURE]/step)) & 0xFFFF;
        if ((uint32_t)((last_tmm - tmm) / (inst->mxfs_length[LAYER_TEMPERATURE]/step)) > 0xFFFF) {
            cp.step = 0xFFFF;
        }
        memcpy(
            work_buffer + work_len,
            &cp,
            sizeof(cp)
        );
        work_len += sizeof(cp);

        for(auto n = 0; n < inst->mxfs_length[LAYER_TEMPERATURE]; n+=step) {
            memcpy(
                work_buffer + work_len,
                &inst->mxfs_buffer[LAYER_TEMPERATURE][n + sizeof(tmm)],
                TEMPERATURE_RECORD_SIZE
            );
            DRIVER_DEBUG(
                "\t[tmp]offset: %d, work_len: %u",
                n, work_len
            );
            work_len += TEMPERATURE_RECORD_SIZE;
        }
        inst->mxfs_length[LAYER_TEMPERATURE] = 0;
        inst->mxfs_count[LAYER_TEMPERATURE] = 0;
    }
    if (inst->blocks[inst->mxfs_current_block].flags & BLOCK_INVALID) {
        // erase first.
        inst->blocks[inst->mxfs_current_block].flags &= ~BLOCK_INVALID;
        if (mxfs_driver_erase(
                inst,
                inst->mxfs_current_block) != OK)
        {
            DRIVER_ERROR("failed, mxfs_driver_erase, %d", inst->mxfs_current_block);
            return(ERR);
        }
        DRIVER_INFO("auto erase with block(%d)", (int)inst->mxfs_current_block);
    }
    int ret = mxfs_driver_write(
                inst,
                inst->mxfs_current_block,
                inst->mxfs_current_page,
                work_buffer,
                work_len);

    if (ret == OK) {
        auto pblock = inst->mxfs_current_block;
        auto ppage = inst->mxfs_current_page;
#ifdef __LINUX__
        inst->blocks[pblock].pages[ppage].start_time = start_tmm;
        inst->blocks[pblock].pages[ppage].last_time = last_tmm;
        inst->blocks[pblock].pages[ppage].flags = page_flags;
#else
        inst->blocks[pblock].pages[ppage] = page_flags;
#endif
        // increment page in current,
        // and increment block in current when needed.
        inst->mxfs_current_page ++;
        if (inst->mxfs_current_page >= PAGE_COUNT) {
            inst->mxfs_current_page = 0;
            inst->mxfs_current_block ++;
            if (inst->mxfs_current_block >= BLOCK_COUNT) {
                inst->mxfs_current_block = BLOCK_START_DATA_AREA;
            }
            // erase mark
            inst->blocks[inst->mxfs_current_block].flags |= BLOCK_INVALID;
        }
        DRIVER_INFO(
            "block: %d, page: %d to block: %d, page: %d, compress:(%u -> %u, gyro: %u, antp: %u, flags: %08x), time:%llu",
            pblock,
            ppage,
            inst->mxfs_current_block,
            inst->mxfs_current_page,
            current_length,
            work_len,
            gyro_count,
            antp_count,
            page_flags,
            start_tmm
        );
    } else {
        DRIVER_ERROR("failed, mxfs_driver_write %d", ret);
    }
    return(ret);
}
