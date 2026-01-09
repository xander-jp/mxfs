/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of test operation(gyro-100)
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "gtest/gtest.h"

TEST(Gyro, Gyro100) {

    uint32_t debug_time = 2345;
    mxfs_inst_ptr inst = NULL;

    ASSERT_EQ(mxfs_create_instance(&inst, NULL), OK);

    srand(time(NULL));

    int16_t gyro[3*100];
    mxfs_erase(inst, (uint16_t)BLOCK_START_DATA_AREA);

    for(auto n = 0;n < sizeof(gyro)/sizeof(gyro[0]); n++) { gyro[n] = (int16_t)(sin(rand()) * 15.f); }

    for(auto n = 0; n < sizeof(gyro)/sizeof(gyro[0]);n+=3) {
        ASSERT_EQ(mxfs_append(
            inst,
            LAYER_GYRO,
            debug_time,
            (uint8_t*)&gyro[n],
            sizeof(gyro[0]) * 3
        ), OK);
    }
    ASSERT_EQ(mxfs_flush(inst), OK);

    mxfs_timerange_ptr timerange = (mxfs_timerange_ptr)malloc(sizeof(mxfs_timerange_t) * 4);

    auto timerange_count = mxfs_timerange(
        inst,
        LAYER_GYRO,
        &timerange[0],
        1
    );
    ASSERT_EQ(timerange_count, 1);

    int16_t compare[3*100];

    auto readlen = mxfs_read(
        inst,
        LAYER_GYRO,
        (uint8_t*)&compare[0],
        sizeof(compare),
        &timerange[0]
    );
    ASSERT_EQ(readlen, sizeof(compare));
    for(auto n = 0;n < sizeof(gyro)/sizeof(gyro[0]); n++) {
        ASSERT_EQ(compare[n], gyro[n]);
    }
    TEST_INFO("gyro(%d, %d)", gyro[0], compare[0]);

    // cleanup.
    ASSERT_EQ(mxfs_release_instance(inst), OK);
    free(timerange);
}
