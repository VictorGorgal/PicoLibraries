cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

set(PROJECT_NAME "PicoLibraries" CACHE STRING "Change to projects name")

project(${PROJECT_NAME})

pico_sdk_init()

add_executable(${PROJECT_NAME}
        main.c
)

# Add libraries
include(${CMAKE_CURRENT_LIST_DIR}/shift_register/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/AHT21/CMakeLists.txt)

target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        hardware_pio
        hardware_dma
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})
