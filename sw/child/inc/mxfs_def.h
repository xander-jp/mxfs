/******************************************************************************/
/*! @brief      mxfs filesystem defined
    @date       created(Sep 29, 2023)
    @par        Copyright 2023 dsugisawa. Released Under the MIT license.
******************************************************************************/
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <getopt.h> 
#include <fcntl.h> 
#include <math.h>
#ifndef __RP2040__
#include <sysexits.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h> 
#endif

#ifdef __LINUX__
#include <stdarg.h>
#elif __RP2040__
#include <stdarg.h>
#endif
//
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <sys/time.h>

// stl
#include <mutex>
#include <thread>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <deque>


typedef void (*mxfs_callback)(void* inst, int count);

enum {
    NO = 0,
    YES = 1,
    OK = 0,
    ERR = -1,
    NEED_MORE_BUFFER = -2,
    MAGIC = 0xdeadc0de,
    PAGE_SIZE = 2048,
#ifndef __RP2040__
    MXFS_BUFFER_SIZE = (PAGE_SIZE << 3),
#else
    MXFS_BUFFER_SIZE = (PAGE_SIZE << 2),
#endif
    BLOCK_COUNT = 1024,
    PAGE_COUNT = 64,
    ANTP_RECORD_SIZE = 18,
    GYRO_RECORD_SIZE = 6,
    TEMPERATURE_RECORD_SIZE = 2,
    //
    BLOCK_VALID = 0,
    BLOCK_INVALID = (1<<0),
    BLOCK_START_DATA_AREA = 1,
    //
    PAGE_VALID = 0,
    PAGE_INVALID = (1<<0),
    PAGE_IN_ANTP_SPEED = (1<<2),
    PAGE_IN_ANTP_POWER = (1<<3),
    PAGE_IN_ANTP_CADENCE = (1<<4),
    PAGE_IN_GYRO = (1<<5),
    PAGE_IN_ACCELEROMETER = (1<<6),
    PAGE_IN_TEMPERATURE = (1<<7),
    //
    LAYER_MIN  = 0,
    LAYER_ANTP_SPEED = LAYER_MIN,
    LAYER_ANTP_POWER,
    LAYER_ANTP_CADENCE,
    LAYER_ANTP_MAX,
    LAYER_GYRO = LAYER_ANTP_MAX,
    LAYER_ACCELEROMETER,
    LAYER_DIM3_MAX,
    LAYER_TEMPERATURE = LAYER_DIM3_MAX,
    LAYER_MAX,
    LAYER_CONFIG = LAYER_MAX,
    LAYER_DIRECT = LAYER_MAX,
    LAYER_DIM_0 = 0,    // dx
    LAYER_DIM_1,        // dy
    LAYER_DIM_2,        // dz
    LAYER_DIM_MAX,

};

typedef struct mxfs_timerange {
    uint64_t start_time;
    uint64_t last_time;
    uint16_t block;
    uint16_t page;
    uint16_t record_count;
    uint16_t padd;
} mxfs_timerange_t, *mxfs_timerange_ptr;


typedef struct mxfs_page_status {
    uint32_t flags;
    uint64_t start_time;
    uint64_t last_time;
} mxfs_page_status_t, *mxfs_page_status_ptr;

typedef struct mxfs_block_status {
    uint32_t flags;
    uint64_t start_time;
#ifdef __LINUX__
    mxfs_page_status_t pages[PAGE_COUNT];
#else
    uint8_t pages[PAGE_COUNT];
#endif
} mxfs_block_status_t, *mxfs_block_status_ptr;

typedef struct mxfs_inst {
#ifdef __LINUX__
    int spi;
    int gpio;
    uint32_t* gpio_mem;
#endif
    uint16_t mxfs_current_block;
    uint16_t mxfs_invalid_block;
    uint16_t mxfs_current_page;
    uint8_t* mxfs_buffer[LAYER_MAX];
    uint16_t mxfs_length[LAYER_MAX];
    uint16_t mxfs_count[LAYER_MAX];
    mxfs_block_status_t blocks[BLOCK_COUNT];
} mxfs_inst_t, *mxfs_inst_ptr;

typedef struct compress {
    uint32_t magic:32;  // 0xdeadbeaf
    uint32_t time:32;   // system time
    uint32_t comp:1;    // 0:no-compress , 1:compress differencial,
    uint32_t type:5;    // 0: DC,
                        // 1: ANTP(SPD),    2: ANTP(PWR), 
                        // 3: ANTP(CAD),    4: GYRO,
                        // 5: ACCELEROMETER,6: TEMPERATURE
    uint32_t step:16;   // gyro average sampling rate(ms)
    uint32_t msec:10;   // sytem time milisecond
    union {
        struct {
            uint32_t length:9; // ANT data length
            uint32_t padd:23;
        } ant;
        struct {
            uint32_t dim:3;     // 0: x, 1: y, 2: z
            uint32_t count:11;  // count: 1024
            uint32_t base:16;   // base value
            uint32_t padd:2;
        } gyr;
        struct {
            uint32_t base:16;   // base value
            uint32_t count:16;  // count: 1024
        } tmp;
    };
} __attribute__((packed)) compress_t, *compress_ptr;


extern "C" {
    int mxfs_create_instance(
        mxfs_inst_ptr* inst,
        mxfs_callback callback
    );
    int mxfs_release_instance(
        mxfs_inst_ptr inst
    );
    int mxfs_append(
        mxfs_inst_ptr inst,
        uint8_t layer,
        uint64_t time,
        uint8_t* buffer,
        uint16_t bufferlen
    );
    int mxfs_flush(
        mxfs_inst_ptr inst
    );
    int mxfs_erase(
        mxfs_inst_ptr inst,
        uint16_t block
    );
    int mxfs_timerange(
        mxfs_inst_ptr inst,
        uint8_t layer,
        mxfs_timerange_ptr timerange,
        uint16_t timerangecount
    );
    int mxfs_timerange_by_block(
        mxfs_inst_ptr inst,
        uint8_t layer,
        uint16_t block,
        mxfs_timerange_ptr timerange,
        uint16_t timerangecount
    );
    int mxfs_read(
        mxfs_inst_ptr inst,
        uint8_t layer,
        uint8_t* buffer,
        uint16_t bufferlen,
        mxfs_timerange_ptr timerange
    );
    int mxfs_write(
        mxfs_inst_ptr inst,
        uint8_t layer,
        uint8_t* buffer,
        uint16_t bufferlen,
        mxfs_timerange_ptr timerange
    );
};

#ifndef DRIVER_INFO
#define DRIVER_INFO(...)  DRIVER_INFO_(__FILENAME__, __LINE__, __VA_ARGS__, "dummy")
#define DRIVER_INFO_(func, line, format, ...) PRINT___("[INF]", 0, func, line, format "%.0s", __VA_ARGS__)
inline static void PRINT___(const char* prefix, const int err, const char *funcname , const int line, const char *format, ...) {
#ifdef __DRIVER_INFO__
    int err_stop = 0;
    int tmperr = errno;
    char msg_bf[512] = {0};
    if (strcmp(prefix, "[INF]") == 0) {
        fprintf(stderr, "\x1b[34m");
    } else if (strcmp(prefix, "[DBG]") == 0) {
        fprintf(stderr, "\x1b[32m");
    } else if (strcmp(prefix, "[ERR]") == 0) {
        fprintf(stderr, "\x1b[31m");
    } else if (strcmp(prefix, "[TEST]") == 0) {
        fprintf(stderr, "\x1b[43m");
    } else if (strcmp(prefix, "[PANIC]") == 0) {
        fprintf(stderr, "\x1b[41m");
        err_stop = 1;
    }
    fprintf(stderr, "%s", prefix);
    fprintf(stderr, "\x1b[0m");
    fprintf(stderr, "%s:%d : ", funcname, line);
    va_list ap;
    va_start(ap, format);
    vsnprintf(msg_bf, sizeof(msg_bf)-1,format, ap);
    fprintf(stderr, "%s\n", msg_bf);
    va_end(ap);
    if (err_stop) {
        fprintf(stderr, "PANIC in %s [%d](%d:%s)\n", funcname, line, tmperr, strerror(tmperr));
        abort();
    }
#endif  // __DRIVER_INFO__
}
#endif

#ifndef DRIVER_DEBUG
#define DRIVER_DEBUG(...)  DRIVER_DEBUG_(__FILENAME__, __LINE__, __VA_ARGS__, "dummy")
#ifdef __DRIVER_DEBUG__
#define DRIVER_DEBUG_(func, line, format, ...) PRINT___("[DBG]", 0, func, line, format "%.0s", __VA_ARGS__)
#else
#define DRIVER_DEBUG_(func, line, format, ...) void()
#endif
#endif

#ifndef DRIVER_ERROR
#define DRIVER_ERROR(...)  DRIVER_ERROR_(__FILENAME__, __LINE__, __VA_ARGS__, "dummy")
#define DRIVER_ERROR_(func, line, format, ...) PRINT___("[ERR]", 0, func, line, format "%.0s", __VA_ARGS__)
#endif

#ifndef TEST_INFO
#define TEST_INFO(...)  TEST_INFO_(__FILENAME__, __LINE__, __VA_ARGS__, "dummy")
#define TEST_INFO_(func, line, format, ...) PRINT___("[TEST]", 0, func, line, format "%.0s", __VA_ARGS__)
#endif

#ifndef MOD_INFO
#define MOD_INFO(...)  MOD_INFO_(__FILENAME__, __LINE__, __VA_ARGS__, "dummy")
#define MOD_INFO_(func, line, format, ...) PRINT___("[INF]", 0, func, line, format "%.0s", __VA_ARGS__)
#endif

#ifndef MOD_ERROR
#define MOD_ERROR(...)  MOD_ERROR_(__FILENAME__, __LINE__, __VA_ARGS__, "dummy")
#define MOD_ERROR_(func, line, format, ...) PRINT___("[ERR]", 0, func, line, format "%.0s", __VA_ARGS__)
#endif

#ifndef MOD_DEBUG
#define MOD_DEBUG(...)  MOD_DEBUG_(__FILENAME__, __LINE__, __VA_ARGS__, "dummy")
#ifdef __DRIVER_DEBUG__
#define MOD_DEBUG_(func, line, format, ...) PRINT___("[DBG]", 0, func, line, format "%.0s", __VA_ARGS__)
#else
#define MOD_DEBUG_(func, line, format, ...) void()
#endif
#endif



#ifndef PANIC
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define PANIC(...)  PANIC_(__FILENAME__, __LINE__, __VA_ARGS__, "dummy")
#define PANIC_(func, line, format, ...) PRINT___("[PANIC]", 1, func, line, format "%.0s", __VA_ARGS__)
#endif

static inline void* MALLOC(int s) {
    void* p = malloc(s);
    DRIVER_INFO(">> malloc : %u/%p", (unsigned)s, p);
    return(p);
}

static inline void FREE(void* p) {
    DRIVER_INFO("<< free : %u/%p", (unsigned)0, p);
    free(p);
}
