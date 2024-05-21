#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/watchdog.h"
#include "shift_register/shift_register.c"

ShiftRegister shiftRegister;
ShiftRegister shiftRegister2;

void print_bits(uint32_t data, uint8_t dataSize) {
    for (uint8_t i = 0; i < dataSize; i++) {
        printf("%d", (data >> (dataSize-1-i)) & 1);
    }
}

void test_shift_register() {
    uint32_t data_in[1];

    read_from_shift_register(&shiftRegister2, data_in);

    printf("------------\n");
    // for (int i = 0; i < 1; i++) {
    print_bits(data_in[0], 32);
    // }
    printf("\n");

    // data_in[0] = 0b11110111;
    // data_in[1] = 0b00011000;
    // data_in[2] = 0b10101011;
    // write_to_shift_register(&shiftRegister, data_in);
}

void init_shift_registers() {
    shiftRegister.pio = pio0;
    shiftRegister.sm = pio_claim_unused_sm(shiftRegister.pio, true);
    shiftRegister.dataPin = 12;
    shiftRegister.clockPin = 13;
    shiftRegister.registerCount = 1;

    uint offset = pio_add_program(shiftRegister.pio, &shift_register_out_program);
    init_out_shift_register(&shiftRegister, offset, 20 * 1000 * 1000);

    shiftRegister2.pio = pio0;
    shiftRegister2.sm = pio_claim_unused_sm(shiftRegister2.pio, true);
    shiftRegister2.dataPin = 14;
    shiftRegister2.clockPin = 15;
    shiftRegister2.registerCount = 1;

    uint offset2 = pio_add_program(shiftRegister2.pio, &shift_register_in_program);
    init_in_shift_register(&shiftRegister2, offset2, 5 * 1000 * 1000);
}

int main() {
    stdio_init_all();

    // Watchdog
    watchdog_enable(5000, true);

    gpio_init(24);
    gpio_set_dir(24, false);
    gpio_set_pulls(24, true, false);

    init_shift_registers();
    while (true) {
        sleep_ms(500);
        if (!gpio_get(24)) {
            test_shift_register();
        }
        watchdog_update();
    }
}
