cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(PicoLibraries)

pico_sdk_init()

add_executable(PicoLibraries
        main.c
)

include(${CMAKE_CURRENT_LIST_DIR}/shift_register/CMakeLists.txt)

target_link_libraries(PicoLibraries
        pico_stdlib
        hardware_pio
        hardware_dma
)

pico_enable_stdio_usb(PicoLibraries 1)
pico_enable_stdio_uart(PicoLibraries 0)

pico_add_extra_outputs(PicoLibraries)
