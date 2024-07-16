cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(PicoLibraries)

pico_sdk_init()

add_subdirectory(744051)
add_subdirectory(shift_register)
add_subdirectory(AHT21)
add_subdirectory(flash_lib)

add_executable(PicoLibraries
        main.c
)

target_link_libraries(PicoLibraries
        pico_stdlib
        744051
        shift_register
        AHT21
        flash_lib
)

add_compile_definitions(PICO_FLASH_SIZE_BYTES=4*1024*1024)

pico_enable_stdio_usb(PicoLibraries 1)
pico_enable_stdio_uart(PicoLibraries 0)

pico_add_extra_outputs(PicoLibraries)
