/******************************************************************************/
/*! @brief      mxfs filesystem driver implementation of test operation(rp2040-basic)
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "mxfs_def.h"
#include <stdio.h>
#include "tusb_config.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "hardware/uart.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"

#include "bsp/board.h"
#include "tusb.h"
#include "pico/util/queue.h"


int main(
    int argc,
    char* argv[]
){
    stdio_init_all();
    srand(time(NULL));
    TEST_INFO("start .. (%s)", "mxfs on rp2040");
    mxfs_timerange_t tr = {0, 0, BLOCK_START_DATA_AREA, 0 };

    // wifi initialize
    if (cyw43_arch_init()) {
        TEST_INFO("Wi-Fi init failed");
        return(-1);
    }
    cyw43_arch_enable_sta_mode();

    const uint16_t TEST_BLOCK = (BLOCK_START_DATA_AREA+789);

    // ------
    // clear block 1-0
    // mxfs, 1st boot.
    // ------
    TEST_INFO("clear block 1-0, mxfs, 1st boot.");
    mxfs_inst_ptr inst = NULL;
    if (mxfs_create_instance(&inst, NULL) != OK) {
        PANIC("mxfs_create_instance");
    }
    for (auto block = int(TEST_BLOCK); block < (TEST_BLOCK+2); block++) {
        if (mxfs_erase(inst, (uint16_t)block) != OK) {
            PANIC("failed, mxfs_erase");
        }
    }
    for (uint16_t block = TEST_BLOCK; block < (TEST_BLOCK+2); block++) {
        uint8_t rbf[32] = { 0x00, };
        tr = { 0, 0, block, 0 };
        if (mxfs_read(inst, LAYER_DIRECT, rbf, sizeof(rbf), &tr) != OK) {
            PANIC("mxfs_read");
        }
        TEST_INFO("read block: %3d", TEST_BLOCK);
    }

    // ------
    // write test text to block at TEST_BLOCK
    // 2nd boot.
    // ------
    TEST_INFO("write to block 1-0, 2nd boot.");
    const char *TESTSTR = "test";
    tr = {0, 0, TEST_BLOCK, 0 };
    if (mxfs_write(inst, LAYER_DIRECT, (uint8_t*)TESTSTR, strlen(TESTSTR), &tr) != OK) {
        PANIC("failed, mxfs_write");
    }
    // ------
    // check block at TEST_BLOCK 
    // 3rd boot.
    // ------
    TEST_INFO("check block 1-0, 3rd boot.");
    {
        uint8_t rbf[2112] = { 0x00, };
        tr = { 0, 0, TEST_BLOCK, 0 };
        int ret;
        if ((ret = mxfs_read(inst, LAYER_DIRECT, rbf, sizeof(rbf), &tr)) == 0) {
            PANIC("mxfs_read, %d", ret);
        }
        TEST_INFO(
            "%02x%02x%02x%02x %02x%02x%02x%02x : %02x%02x%02x%02x %02x%02x%02x%02x :"
            "%02x%02x%02x%02x %02x%02x%02x%02x : %02x%02x%02x%02x %02x%02x%02x%02x", 
            rbf[0], rbf[1], rbf[2], rbf[3], rbf[4], rbf[5], rbf[6], rbf[7],
            rbf[8], rbf[9], rbf[10], rbf[11], rbf[12], rbf[13], rbf[14], rbf[15],
            rbf[16], rbf[17], rbf[18], rbf[19], rbf[20], rbf[21], rbf[22], rbf[23],
            rbf[24], rbf[25], rbf[26], rbf[27], rbf[28], rbf[29], rbf[30], rbf[31]
        );
        if (memcmp(rbf, TESTSTR, strlen(TESTSTR)) != 0) {
            PANIC("invalid written text.");
        }
    }
    // check append, and flash to TEST_BLOCK + 1
    inst->mxfs_current_block = TEST_BLOCK + 1;
    inst->mxfs_current_page = 1;

    int16_t gyro[3*10] = { 0, };

    for(auto n = 0;n < sizeof(gyro)/sizeof(gyro[0]); n++) {
        gyro[n] = (int16_t)(sin(rand()) * 15.f);
    }

    for(auto n = 0; n < sizeof(gyro)/sizeof(gyro[0]);n+=3) {
        if (mxfs_append(
            inst,
            LAYER_GYRO,
            12345,
            (uint8_t*)&gyro[n],
            sizeof(gyro[0]) * 3) != OK)
        {
            PANIC("mxfs_append ..");
        }
    }

    if (mxfs_flush(inst) != OK) {
        PANIC("mxfs_flush");
    }

    mxfs_timerange_t timerange[1] = { 0, };

    int timerange_count = mxfs_timerange(
        inst,
        LAYER_GYRO,
        timerange,
        1
    );
    if (timerange_count != 1) {
        PANIC("timerange_count != 1");
    }

    int16_t compare[3*10] = { 0, };

#ifndef __RP2040__

    int readlen = mxfs_read(
        inst,
        LAYER_GYRO,
        (uint8_t*)&compare[0],
        sizeof(compare),
        &timerange[0]
    );
    if (readlen != sizeof(compare)) {
        PANIC("readlen != sizeof(compare), %d ... %d", readlen, sizeof(compare));
    }
    for(int n = 0;n < sizeof(gyro)/sizeof(gyro[0]); n++) {
        if (compare[n] != gyro[n]) {
            PANIC("compare[n] != gyro[n] (%d)", n);
        }
        TEST_INFO("%4d - %4d", compare[n], gyro[n]);
    }
#endif
    unsigned cnt = 0;
    struct timeval tv, ptv;
    gettimeofday(&ptv, NULL);

    while(true) {
        gettimeofday(&tv, NULL);

        // 1sec for debbug.
        if (tv.tv_sec != ptv.tv_sec) {
            cnt++;
            TEST_INFO("..%4u", cnt);
            ptv = tv;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, cnt%2==0?1:0);
        }

        sleep_ms(10);
    }

    // cleanup.
    if (mxfs_release_instance(inst) != OK) {
        PANIC("mxfs_release_instance");
    }
    cyw43_arch_deinit();

    return(0);
}


