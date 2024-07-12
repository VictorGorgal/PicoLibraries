#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/watchdog.h"
#include "hardware/i2c.h"
#include "shift_register/shift_register.c"
#include "AHT21/AHT21.c"

int main() {
    stdio_init_all();

    // Watchdog
    watchdog_enable(5000, true);

    shift_register_example();

    // AHT21_example();
}
