/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of test operation(gyro-1000)
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "gtest/gtest.h"

TEST(Gyro, Gyro1000) {
    mxfs_inst_ptr inst = NULL;
    uint32_t debug_time = 2345;
    ASSERT_EQ(mxfs_create_instance(&inst, NULL), OK);

    srand(time(NULL));

    int16_t gyro[3*1000];
    int16_t compare[3*1000];

    mxfs_erase(inst, (uint16_t)BLOCK_START_DATA_AREA);


    ASSERT_EQ(sizeof(gyro)/sizeof(gyro[0]), 3*1000);

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
    
    


    mxfs_timerange_ptr timerange = (mxfs_timerange_ptr)malloc(sizeof(mxfs_timerange_t) * 5);
    

    auto timerange_count = mxfs_timerange(
        inst,
        LAYER_GYRO,
        &timerange[0],
        4
    );
    ASSERT_EQ(timerange_count, 4);


    TEST_INFO(
        "time[0] %u, %u/ time[1] %u, %u",
        timerange[0].block, timerange[0].page,
        timerange[1].block, timerange[1].page
    );
    
    auto readlen = mxfs_read(
        inst,
        LAYER_GYRO,
        (uint8_t*)&compare[0],
        1968,
        &timerange[0]
    );
    ASSERT_EQ(readlen, 1968);
    



    for(auto n = 0;n < (readlen/GYRO_RECORD_SIZE); n+=3) {
        // TEST_INFO("gyro<%d> %d, %d, %d / %d, %d, %d",
        //     n, gyro[n], gyro[n+1], gyro[n+2],
        //     compare[n], compare[n+1], compare[n+2]);
        ASSERT_EQ(compare[n], gyro[n]);
        ASSERT_EQ(compare[n+1], gyro[n+1]);
        ASSERT_EQ(compare[n+2], gyro[n+2]);
    }
    readlen = mxfs_read(
        inst,
        LAYER_GYRO,
        (uint8_t*)&compare[0],
        1968,
        &timerange[1]
    );
    ASSERT_EQ(readlen, 1968);
    
    for(auto n = 0;n < (readlen/GYRO_RECORD_SIZE); n+=3) {
        auto gn = ((1968/2) + n);   // offset = 3864 / 6 * 3

        // TEST_INFO("gyro<%d> %d, %d, %d / %d, %d, %d",
        //     n, gyro[gn], gyro[gn+1], gyro[gn+2],
        //     compare[n], compare[n+1], compare[n+2]);
        ASSERT_EQ(compare[n], gyro[gn]);
        ASSERT_EQ(compare[n+1], gyro[gn+1]);
        ASSERT_EQ(compare[n+2], gyro[gn+2]);
    }

    // cleanup.
    ASSERT_EQ(mxfs_release_instance(inst), OK);
    free(timerange);
}
