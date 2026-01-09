/******************************************************************************/
/*! @brief      Unit test entry point with GoogleTest framework
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "spinand_def.h"
#include "gtest/gtest.h"

extern void init_gpio_spi(int* pspi, int* pgpio, uint32_t** ppgpio_mem);

int __testing_bytes = 32;

//
class CustomEnvironment :public ::testing::Environment {
public:
    virtual ~CustomEnvironment() {}
    virtual void SetUp() {
        printf("spinand test\n");
        int spi, gpio;
        uint32_t* gpio_mem;

        init_gpio_spi(
            &spi,
            &gpio,
            &gpio_mem
        );
    }
    virtual void TearDown() {
    }
};
//
int main(int argc, char* argv[]){  
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    if (argc > 1) {
        __testing_bytes = atoi(argv[1]);
    }
    return RUN_ALL_TESTS();
}


