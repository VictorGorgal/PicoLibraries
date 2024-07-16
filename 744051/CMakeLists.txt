add_library(744051 STATIC 744051.c 744051.h)

target_link_libraries(744051
        pico_stdlib
        hardware_adc
)

target_include_directories(744051 PUBLIC ${CMAKE_CURRENT_LIST_DIR})
