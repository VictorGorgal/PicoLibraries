#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/watchdog.h"
#include "hardware/i2c.h"
#include "shift_register/shift_register.c"
#include "AHT21/AHT21.c"

ShiftRegister shiftRegisterInput;
ShiftRegister shiftRegisterOutput;

void print_bits(uint32_t data, uint8_t dataSize) {
    for (uint8_t i = 0; i < dataSize; i++) {
        printf("%d", (data >> (dataSize-1-i)) & 1);
    }
}

void test_shift_register() {
    uint8_t data_out[4] = {0b11111010, 0b11001010, 0b11011010};
    uint8_t data_in[4];

    write_to_shift_register(&shiftRegisterOutput, data_out, true);
    read_from_shift_register(&shiftRegisterInput, data_in);

    printf("\n");
    print_bits(data_in[0], 8);
    printf(", ");
    print_bits(data_in[1], 8);
    printf(", ");
    print_bits(data_in[2], 8);
    printf(", ");
    print_bits(data_in[3], 8);
    printf("\n");
}

void init_shift_registers() {
    shiftRegisterInput.pio = pio0;
    shiftRegisterInput.sm = pio_claim_unused_sm(shiftRegisterInput.pio, true);
    shiftRegisterInput.dataPin = 10;
    shiftRegisterInput.clockPin = 11;
    shiftRegisterInput.updateData = 12;
    shiftRegisterInput.registerCount = 3;

    uint offset = pio_add_program(shiftRegisterInput.pio, &shift_register_in_program);
    init_in_shift_register(&shiftRegisterInput, offset, 8 * 1000 * 1000);

    shiftRegisterOutput.pio = pio0;
    shiftRegisterOutput.sm = pio_claim_unused_sm(shiftRegisterOutput.pio, true);
    shiftRegisterOutput.dataPin = 13;
    shiftRegisterOutput.clockPin = 14;
    shiftRegisterOutput.updateData = 15;
    shiftRegisterOutput.registerCount = 3;

    uint offset2 = pio_add_program(shiftRegisterOutput.pio, &shift_register_out_program);
    init_out_shift_register(&shiftRegisterOutput, offset2, 8 * 1000 * 1000);
}

int main() {
    stdio_init_all();

    // Watchdog
    // watchdog_enable(5000, true);

    // init_shift_registers();
    // while (true) {
    //     sleep_ms(500);
    //     printf(".");
    //     if (!gpio_get(24)) {
    //         test_shift_register();
    //     }
    //     watchdog_update();
    // }

    
    AHT21_example();

    while (true) {
        watchdog_update();
    }
}
