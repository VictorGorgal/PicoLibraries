add_library(shift_register STATIC shift_register.c shift_register.h)

pico_generate_pio_header(shift_register
        ${CMAKE_CURRENT_LIST_DIR}/shift_register.pio
)

target_link_libraries(shift_register
        pico_stdlib
        hardware_pio
)

target_include_directories(shift_register PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
