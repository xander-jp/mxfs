/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of test operation(ANTP-cadence-10)
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "gtest/gtest.h"

TEST(ANTP, AntpCadence10) {
    srand(time(NULL));
    uint32_t debug_time = 1234;
    mxfs_inst_ptr inst = NULL;
    ASSERT_EQ(mxfs_create_instance(&inst, NULL), OK);
    mxfs_erase(inst, (uint16_t)BLOCK_START_DATA_AREA);

    uint8_t antp[10][ANTP_RECORD_SIZE];

    for(auto n = 0;n < sizeof(antp)/sizeof(antp[0]); n++) {
        for(auto m = 0; m < ANTP_RECORD_SIZE; m++) {
            antp[n][m] = (int8_t)(sin(rand()) * 255.f);
        }
    }
    for(auto n = 0; n < sizeof(antp)/sizeof(antp[0]);n++) {
        ASSERT_EQ(mxfs_append(
            inst,
            LAYER_ANTP_CADENCE,
            debug_time,
            (uint8_t*)&antp[n],
            sizeof(antp[0])
        ), OK);
    }
    ASSERT_EQ(mxfs_flush(inst), OK);

    mxfs_timerange_ptr timerange = (mxfs_timerange_ptr)malloc(sizeof(mxfs_timerange_t) * 4);
    auto timerange_count = mxfs_timerange(
        inst,
        LAYER_ANTP_CADENCE,
        &timerange[0],
        1
    );
    ASSERT_EQ(timerange_count, 1);
    TEST_INFO("antp cadence time (%u : %u , %u)",
        debug_time,
        timerange[0].start_time,
        timerange[0].last_time);


    uint8_t compare[10][ANTP_RECORD_SIZE];

    auto readlen = mxfs_read(
        inst,
        LAYER_ANTP_CADENCE,
        (uint8_t*)&compare[0],
        sizeof(compare),
        &timerange[0]
    );
    ASSERT_EQ(readlen, sizeof(compare));
    for(auto n = 0;n < sizeof(antp)/sizeof(antp[0]); n++) {
        ASSERT_EQ(memcmp(compare[n], antp[n], ANTP_RECORD_SIZE), 0);
        TEST_INFO("antp cadence(%02x, %02x, %02x, %02x, ... %02x, %02x, %02x, %02x)",
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
