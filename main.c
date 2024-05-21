#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/watchdog.h"
#include "shift_register/shift_register.c"

ShiftRegister shiftRegisterInput;
ShiftRegister shiftRegisterOutput;

void print_bits(uint32_t data, uint8_t dataSize) {
    for (uint8_t i = 0; i < dataSize; i++) {
        printf("%d", (data >> (dataSize-1-i)) & 1);
    }
}

void test_shift_register() {
    uint8_t data_out[1] = {0b11010001};
    uint8_t data_in[1];

    write_to_shift_register(&shiftRegisterOutput, data_out, true);
    read_from_shift_register(&shiftRegisterInput, data_in);

    printf("\n");
    print_bits(data_in[0], 8);
    printf(", 0x%08x\n", data_in[0]);
}

void init_shift_registers() {
    shiftRegisterInput.pio = pio0;
    shiftRegisterInput.sm = pio_claim_unused_sm(shiftRegisterInput.pio, true);
    shiftRegisterInput.dataPin = 10;
    shiftRegisterInput.clockPin = 11;
    shiftRegisterInput.updateData = 12;
    shiftRegisterInput.registerCount = 1;

    uint offset = pio_add_program(shiftRegisterInput.pio, &shift_register_in_program);
    init_in_shift_register(&shiftRegisterInput, offset, 10 * 1000 * 1000);

    shiftRegisterOutput.pio = pio0;
    shiftRegisterOutput.sm = pio_claim_unused_sm(shiftRegisterOutput.pio, true);
    shiftRegisterOutput.dataPin = 13;
    shiftRegisterOutput.clockPin = 14;
    shiftRegisterOutput.updateData = 15;
    shiftRegisterOutput.registerCount = 1;

    uint offset2 = pio_add_program(shiftRegisterOutput.pio, &shift_register_out_program);
    init_out_shift_register(&shiftRegisterOutput, offset2, 10 * 1000 * 1000);
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
        printf(".");
        if (!gpio_get(24)) {
            test_shift_register();
        }
        watchdog_update();
    }
}
