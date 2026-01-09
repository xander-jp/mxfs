/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of test operation(gyro-acc-tempereture-auto-flush)
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "gtest/gtest.h"

TEST(Basic, Gyro_Acc_Temp_AutoFlush) {
    uint32_t debug_time = 3456;
    mxfs_inst_ptr inst = NULL;

    ASSERT_EQ(mxfs_create_instance(&inst, NULL), OK);

    srand(time(NULL));
    uint32_t stm = (uint32_t)time(NULL), etm;

    int16_t gyro[3], acc[3], tmp[1];
    mxfs_erase(inst, (uint16_t)BLOCK_START_DATA_AREA);


    for(auto n = 0;n < 3; n++) { gyro[n] = (int16_t)(sin(rand()) * 15.f); }
    for(auto n = 0;n < 3; n++) { acc[n] = (int16_t)(sin(rand()) * 15.f); }
    tmp[0] = (int16_t)(sin(rand()) * 15.f);

    for(auto n = 0; n < 300; n++) {
        ASSERT_EQ(mxfs_append(
            inst,
            LAYER_GYRO,
            debug_time + (n/100),
            (uint8_t*)gyro,
            sizeof(gyro)
        ), OK);

        ASSERT_EQ(mxfs_append(
            inst,
            LAYER_ACCELEROMETER,
            debug_time + (n/100),
            (uint8_t*)acc,
            sizeof(acc)
        ), OK);

        ASSERT_EQ(mxfs_append(
            inst,
            LAYER_TEMPERATURE,
            debug_time + (n/100),
            (uint8_t*)tmp,
            sizeof(tmp[0])
        ), OK);
    }

    mxfs_timerange_ptr timerange = (mxfs_timerange_ptr)malloc(sizeof(mxfs_timerange_t) * 5);

    auto timerange_count = mxfs_timerange(
        inst,
        LAYER_TEMPERATURE,
        &timerange[0],
        2
    );
    ASSERT_EQ(timerange_count, 2);

    timerange_count = mxfs_timerange(
        inst,
        LAYER_GYRO,
        &timerange[2],
        2
    );
    ASSERT_EQ(timerange_count, 2);

    timerange_count = mxfs_timerange(
        inst,
        LAYER_ACCELEROMETER,
        &timerange[4],
        2
    );
    ASSERT_EQ(timerange_count, 2);

    TEST_INFO("gyro(%u, %u, %u)/acc(%u, %u, %u)/tmp(%u, %u, %u)",
        timerange[0].block, timerange[0].page, timerange[0].start_time,
        timerange[1].block, timerange[1].page, timerange[1].start_time,
        timerange[2].block, timerange[2].page, timerange[2].start_time);

    int16_t compare[276*3];

    auto readlen = mxfs_read(
        inst,
        LAYER_TEMPERATURE,
        (uint8_t*)compare,
        sizeof(compare),
        &timerange[0]
    );
    ASSERT_EQ(readlen, sizeof(compare) / 3 / sizeof(compare[0]));
    ASSERT_EQ(compare[0], tmp[0]);
    TEST_INFO("tmp(%d, %d)", tmp[0], compare[0]);

    readlen = mxfs_read(
        inst,
        LAYER_GYRO,
        (uint8_t*)compare,
        sizeof(compare),
        &timerange[1]
    );
    ASSERT_EQ(readlen, sizeof(compare) / sizeof(compare[0]));
    ASSERT_EQ(compare[0], gyro[0]);
    TEST_INFO("gyro(%d, %d, %d)(%d, %d, %d)",
        gyro[0], gyro[1], gyro[2],
        compare[0], compare[1], compare[2]);

    // cleanup.
    ASSERT_EQ(mxfs_release_instance(inst), OK);
    free(timerange);
}
