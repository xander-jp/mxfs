CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

SET(PRJ mxfs-rp2040)
SET(PICO_BOARD pico_w)
SET(HOME $ENV{HOME})
SET(PICO_SDK_FETCH_FROM_GIT on)
INCLUDE(pico_sdk_import.cmake)

PROJECT(${PRJ} C CXX ASM)
SET(CMAKE_C_STANDARD 11)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")

IF (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
ENDIF()
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__RP2040__")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__RP2040__")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__DRIVER_INFO__ -D__DRIVER_DEBUG__")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__DRIVER_INFO__ -D__DRIVER_DEBUG__")

INCLUDE_DIRECTORIES("../../inc")
INCLUDE_DIRECTORIES("../../src/drivers/inc")

FILE(GLOB C_SOURCE_FILES ../../src/*.cc)
FILE(GLOB C_SOURCE_TEST_FILES ../../src/test/rp2040.cc)
FILE(GLOB C_SOURCE_DRIVER_FILES ../../src/drivers/rp2040/*.cc)

ADD_EXECUTABLE(
    ${PRJ}
    ${C_SOURCE_TEST_FILES}
    ${C_SOURCE_FILES}
    ${C_SOURCE_DRIVER_FILES}
)


PROJECT(${PRJ} VERSION 1.0.0)

TARGET_LINK_LIBRARIES(
    ${PRJ}
    pico_stdlib
    hardware_uart
    hardware_spi
    tinyusb_host
    tinyusb_board
    pico_cyw43_arch_lwip_threadsafe_background
)

pico_sdk_init()
pico_enable_stdio_usb(${PRJ} 0)
pico_enable_stdio_uart(${PRJ} 1)
pico_add_extra_outputs(${PRJ})

ADD_COMPILE_OPTIONS(
    -Wall
    -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
    -Wno-unused-function # we have some for the docs that aren't called
    -Wno-maybe-uninitialized
)
