cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/pico-sdk)
    message(STATUS "PICO_SDK_PATH not specified, using ${PICO_SDK_PATH}")
endif()

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(pico_dev C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# Add subdirectories
add_subdirectory(examples/blinky)

# Common compile options
add_compile_options(-Wall
        -Wno-format
        -Wno-unused-function
        -Wno-maybe-uninitialized
        )

# Enable debug output
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()