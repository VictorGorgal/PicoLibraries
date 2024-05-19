#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/watchdog.h"
#include "shift_register_in.pio.h"
#include "shift_register_out.pio.h"

ShiftRegister shiftRegister;
ShiftRegister shiftRegister2;

void test_shift_register() {
    uint8_t data_in[shiftRegister.registerCount];
    for (int i = 0; i < shiftRegister.registerCount; i++) {
        data_in[i] = 0;
    }

    read_from_shift_register(&shiftRegister2, data_in);
    write_to_shift_register(&shiftRegister, data_in);
}

void init_shift_registers() {
    shiftRegister.pio = pio0;
    shiftRegister.sm = pio_claim_unused_sm(shiftRegister.pio, true);
    shiftRegister.dataPin = 12;
    shiftRegister.clockPin = 13;
    shiftRegister.registerCount = 2;

    uint offset = pio_add_program(shiftRegister.pio, &shift_register_out_program);
    init_out_shift_register(&shiftRegister, offset, 1 * 1000);

    shiftRegister2.pio = pio0;
    shiftRegister2.sm = pio_claim_unused_sm(shiftRegister2.pio, true);
    shiftRegister2.dataPin = 14;
    shiftRegister2.clockPin = 15;
    shiftRegister2.registerCount = 2;

    uint offset2 = pio_add_program(shiftRegister2.pio, &shift_register_in_program);
    init_in_shift_register(&shiftRegister2, offset2, 1 * 1000);
}

int main() {
    stdio_init_all();

    // Watchdog
    watchdog_enable(2000, true);

    init_shift_registers();
    
    while (true) {
        sleep_ms(500);
        test_shift_register();
        watchdog_update();
    }
}
