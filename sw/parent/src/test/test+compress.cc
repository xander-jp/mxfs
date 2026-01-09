/******************************************************************************/
/*! @brief      Unit tests for data compression algorithm
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#include "spinand_def.h"
#include "gtest/gtest.h"

TEST(__COMPRESS, Algoritm) {

    srand(time(NULL));

    int32_t TRGTCNT = 2048;
    int32_t BASE = 1000;
    int32_t velocity = BASE;
    int16_t src_dt[TRGTCNT];
    int16_t src[TRGTCNT];

    // 100 Hz times N sec
    for(auto n = 0;n < TRGTCNT; n++) {
        // -1 <-> 1.
        auto dy = sin(rand());

        // compress rate: 8 bits(-127 <-> 127)
        src_dt[n] = (int16_t)(dy * 127.f);
        velocity += src_dt[n];
        src[n] = velocity; 
        // printf("delt: %d , curr: %d\n", src_dt[n], src[n]);
    }

    compress_t cp{ (int32_t)0xdeadbeaf, 0, 0, 0, 0, src[0]};

    printf("src data size .. %d, ", sizeof(src));
    printf("compress (%d)\n", sizeof(cp));

    int32_t index = 0;
    int32_t offset = 0;
    std::vector<char> comp(TRGTCNT * sizeof(src[0])); 

    // compress header
    memcpy(
        comp.data() + 0,
        &cp,
        sizeof(cp)
    ); 
    offset += sizeof(cp);
    
    // compress differencial
    cp.type = 1;        // defferencial
    cp.width = 1;       // 8bit
    cp.wdcnt = 496;     // limit, 1984
    cp.base = src[0];   // base value
    
    // differencial header
    memcpy(
        comp.data() + offset,
        &cp,
        sizeof(cp)
    ); 
    offset += sizeof(cp);
    
    velocity = src[0];

    for(auto n = 1;n < TRGTCNT; n++) {
        velocity = src[n] - src[n-1];     
        // printf("delt: %d , curr: %d, prev: %d, offset: %d\n", velocity, src[n], src[n-1], offset);
        
        int8_t differencial = (int8_t)velocity;
        *(comp.data() + offset) =  differencial;
        offset += sizeof(differencial);
        index = n;
        if (offset >= ((int32_t)cp.wdcnt * 4)) {
            printf("%d, %d, %d\n",
                (int32_t)cp.wdcnt,
                offset,
                ((int32_t)cp.wdcnt * 4)
            );
            break;
        }
    }
    
    printf(
        "src size: %d -> compressed: %d(%5.2lf)\n",
        sizeof(src[0]) * index,
        offset,
        ((float)(offset)) / ((float)(sizeof(src[0]) * TRGTCNT))
    );
    
    // test compressed values,
    offset = 0;
    index = 0;
    memcpy(
        &cp,
        comp.data() + offset,
        sizeof(cp)
    );
    velocity = cp.base;
    ASSERT_EQ(cp.base, src[0]);
    ASSERT_EQ(cp.magic, 0xdeadbeaf);
    ASSERT_EQ(cp.type, 0);
    ASSERT_EQ(cp.width, 0);
    ASSERT_EQ(cp.wdcnt, 0);
    offset += sizeof(cp);
    memcpy(
        &cp,
        comp.data() + offset,
        sizeof(cp)
    );
    ASSERT_EQ(cp.base, src[0]);
    ASSERT_EQ(cp.magic, 0xdeadbeaf);
    ASSERT_EQ(cp.type&1, 1);
    ASSERT_EQ(cp.width&1, 1);
    ASSERT_EQ(cp.wdcnt, 496);
    offset += sizeof(cp);
    for(index = 1;offset < ((int32_t)cp.wdcnt*4); offset++, index++) {
        velocity += (int8_t)(*(comp.data() + offset));
        ASSERT_EQ(velocity, src[index]);
    }
}


