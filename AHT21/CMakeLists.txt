add_library(AHT21 STATIC AHT21.c AHT21.h)

target_link_libraries(AHT21
        pico_stdlib
        hardware_i2c
)

target_include_directories(AHT21 PUBLIC ${CMAKE_CURRENT_LIST_DIR})
