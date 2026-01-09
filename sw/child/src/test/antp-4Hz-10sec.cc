/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of test operation(ANTP-4Hz-10sec)
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "gtest/gtest.h"

TEST(ANTP, Antp4Hz10sec) {
    srand(time(NULL));
    uint32_t debug_time = 1234567;
    mxfs_inst_ptr inst = NULL;
    ASSERT_EQ(mxfs_create_instance(&inst, NULL), OK);
    mxfs_erase(inst, (uint16_t)BLOCK_START_DATA_AREA);

    uint8_t antp_speed[ANTP_RECORD_SIZE];
    uint8_t antp_power[ANTP_RECORD_SIZE];
    uint8_t antp_cadence[ANTP_RECORD_SIZE];

    for(auto m = 0; m < ANTP_RECORD_SIZE; m++) {
        antp_speed[m] = (int8_t)(sin(rand()) * 255.f);
        antp_power[m] = (int8_t)(sin(rand()) * 255.f);
        antp_cadence[m] = (int8_t)(sin(rand()) * 255.f);
    }
    for(auto l = 0; l < 10; l++) {
        for(auto n = 0; n < 4/* 4Hz */;n++) {
            ASSERT_EQ(mxfs_append(
                inst,
                LAYER_ANTP_SPEED,
                debug_time + l,
                (uint8_t*)antp_speed,
                sizeof(antp_speed)
            ), OK);
        }
        for(auto n = 0; n < 4/* 4Hz */;n++) {
            ASSERT_EQ(mxfs_append(
                inst,
                LAYER_ANTP_POWER,
                debug_time + l,
                (uint8_t*)antp_power,
                sizeof(antp_power)
            ), OK);
        }
        for(auto n = 0; n < 4/* 4Hz */;n++) {
            ASSERT_EQ(mxfs_append(
                inst,
                LAYER_ANTP_CADENCE,
                debug_time + l,
                (uint8_t*)antp_cadence,
                sizeof(antp_cadence)
            ), OK);
        }
    }
    ASSERT_EQ(mxfs_flush(inst), OK);

    mxfs_timerange_ptr timerange = (mxfs_timerange_ptr)malloc(sizeof(mxfs_timerange_t) * 9);

    auto timerange_count = mxfs_timerange(
        inst,
        LAYER_ANTP_SPEED,
        &timerange[0],
        2
    );
    ASSERT_EQ(timerange_count, 2);
    for(int n = 0;n < 2;n++) {
        TEST_INFO(
            "speed: timerange(%d, %u, %u, %u, %u, %u)",
            n,
            timerange[n].block,
            timerange[n].page,
            timerange[n].last_time,
            timerange[n].start_time,
            timerange[n].record_count
        );
    }

    timerange_count = mxfs_timerange(
        inst,
        LAYER_ANTP_POWER,
        &timerange[2],
        2
    );
    ASSERT_EQ(timerange_count, 2);
    for(int n = 0;n < timerange_count;n++) {
        TEST_INFO(
            "power: timerange(%d, %u, %u, %u, %u, %u)",
            n,
            timerange[n].block,
            timerange[n].page,
            timerange[n].last_time,
            timerange[n].start_time,
            timerange[n].record_count
        );
    }

    timerange_count = mxfs_timerange(
        inst,
        LAYER_ANTP_CADENCE,
        &timerange[4],
        3
    );
    ASSERT_EQ(timerange_count, 3);
    for(int n = 0;n < timerange_count;n++) {
        TEST_INFO(
            "cadence: timerange(%d, %u, %u, %u, %u, %u)",
            n,
            timerange[n].block,
            timerange[n].page,
            timerange[n].last_time,
            timerange[n].start_time,
            timerange[n].record_count
        );
    }

    // 10 sec x 4Hz = 40
    uint8_t compare[40][ANTP_RECORD_SIZE];

    // speed(0)
    auto readlen = mxfs_read(
        inst,
        LAYER_ANTP_SPEED,
        (uint8_t*)&compare[0],
        sizeof(compare),
        &timerange[0]
    );

    TEST_INFO(
        "speed: mxfs_read(%d, %u)",
        readlen,
        sizeof(compare)
    );
    
    // speed(1)
    readlen = mxfs_read(
        inst,
        LAYER_ANTP_SPEED,
        (uint8_t*)&compare[0],
        sizeof(compare),
        &timerange[1]
    );

    TEST_INFO(
        "speed: mxfs_read(%d, %u)",
        readlen,
        sizeof(compare)
    );

    ASSERT_EQ(readlen, sizeof(compare)/2);
    for(auto n = 0;n < 20; n++) {
        ASSERT_EQ(memcmp(compare[n], antp_speed, ANTP_RECORD_SIZE), 0);
        TEST_INFO("antp speed(%02x, %02x, %02x, %02x, ... %02x, %02x, %02x, %02x)",
            compare[n][0],
            compare[n][1],
            compare[n][2],
            compare[n][3],
            compare[n][14],
            compare[n][15],
            compare[n][16],
            compare[n][17]
        );
    }

    // power
    readlen = mxfs_read(
        inst,
        LAYER_ANTP_POWER,
        (uint8_t*)&compare[0],
        sizeof(compare),
        &timerange[2]
    );
    ASSERT_EQ(readlen, sizeof(compare)/2);
    for(auto n = 0;n < 20; n++) {
        ASSERT_EQ(memcmp(compare[n], antp_power, ANTP_RECORD_SIZE), 0);
        TEST_INFO("antp power(%02x, %02x, %02x, %02x, ... %02x, %02x, %02x, %02x)",
            compare[n][0],
            compare[n][1],
            compare[n][2],
            compare[n][3],
            compare[n][14],
            compare[n][15],
            compare[n][16],
            compare[n][17]
        );
    }
    

    // cleanup.
    ASSERT_EQ(mxfs_release_instance(inst), OK);
    free(timerange);
}
