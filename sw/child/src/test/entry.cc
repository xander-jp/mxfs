/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of test entry point.
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include "gtest/gtest.h"

int main(int argc, char* argv[]){  
    testing::InitGoogleTest(&argc, argv);
    TEST_INFO("start .. (%s)", argv[0]);
    return RUN_ALL_TESTS();
}
