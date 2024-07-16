add_library(flash_lib STATIC flash_lib.c flash_lib.h)

target_link_libraries(flash_lib
        pico_stdlib
        hardware_flash
)

target_include_directories(flash_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR})
