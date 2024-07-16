#include "shift_register.h"

// SIPO
// Max freq. 41.666MHz
// Recommended freq:
// 74HC164 - 10MHz
// 74HC595 - 41.666MHz
void init_out_shift_register(ShiftRegister *shiftRegister, uint offset, float clock) {
    PIO pio = shiftRegister->pio;
    uint sm = shiftRegister->sm;

    clock *= 3;
    float clockDiv = (float) clock_get_hz(clk_sys) / clock;
    pio_sm_config c = shift_register_program_get_default_config(offset);

    gpio_init(shiftRegister->updateData);
    gpio_set_dir(shiftRegister->updateData, true);
    gpio_put(shiftRegister->updateData, 0);

    pio_gpio_init(pio, shiftRegister->dataPin);
    pio_gpio_init(pio, shiftRegister->clockPin);

    sm_config_set_out_pins(&c, shiftRegister->dataPin, 1);
    pio_sm_set_consecutive_pindirs(pio, shiftRegister->sm, shiftRegister->dataPin, 1, true);
    sm_config_set_sideset_pins(&c, shiftRegister->clockPin);
    pio_sm_set_consecutive_pindirs(pio, shiftRegister->sm, shiftRegister->clockPin, 1, true);

    sm_config_set_in_shift(&c, false, true, 8);
    sm_config_set_out_shift(&c, true, true, 8);

    sm_config_set_clkdiv(&c, clockDiv);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// PISO
// Max freq. 41.666MHz
// Recommended freq:
// 74HC165 - 10MHz
void init_in_shift_register(ShiftRegister *shiftRegister, uint offset, float clock) {
    PIO pio = shiftRegister->pio;
    uint sm = shiftRegister->sm;

    clock *= 3;
    float clockDiv = (float) clock_get_hz(clk_sys) / clock;
    pio_sm_config c = shift_register_program_get_default_config(offset);

    gpio_init(shiftRegister->updateData);
    gpio_set_dir(shiftRegister->updateData, true);
    gpio_put(shiftRegister->updateData, 1);

    pio_gpio_init(pio, shiftRegister->dataPin);
    pio_gpio_init(pio, shiftRegister->clockPin);

    sm_config_set_in_pins(&c, shiftRegister->dataPin);
    pio_sm_set_consecutive_pindirs(pio, sm, shiftRegister->dataPin, 1, false);
    sm_config_set_sideset_pins(&c, shiftRegister->clockPin);
    pio_sm_set_consecutive_pindirs(pio, sm, shiftRegister->clockPin, 1, true);

    sm_config_set_in_shift(&c, false, true, 8);
    sm_config_set_out_shift(&c, true, true, 8);

    sm_config_set_clkdiv(&c, clockDiv);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// 0bABCDEFGH -> output
void __time_critical_func(write_to_shift_register)(ShiftRegister *shiftRegister, uint8_t *dataArray) {
    for (uint8_t i = shiftRegister->registerCount; i > 0; i--) {
        pio_sm_put_blocking(shiftRegister->pio, shiftRegister->sm, dataArray[i-1]);
        pio_sm_get(shiftRegister->pio, shiftRegister->sm);
    }

    uint32_t SM_STALL_MASK = 1u << (PIO_FDEBUG_TXSTALL_LSB + shiftRegister->sm);
    shiftRegister->pio->fdebug = SM_STALL_MASK;
    while(!(shiftRegister->pio->fdebug & SM_STALL_MASK)) {
        tight_loop_contents();
        pio_sm_get(shiftRegister->pio, shiftRegister->sm);
    }

    for(int i = 0; i < 2; i++);
    gpio_put(shiftRegister->updateData, 1);
    for(int i = 0; i < 1; i++);
    gpio_put(shiftRegister->updateData, 0);
}

// 0bHGFEDCBA <- input
void __time_critical_func(read_from_shift_register)(ShiftRegister *shiftRegister, uint8_t dataArray[]) {
    gpio_put(shiftRegister->updateData, 0);
    for(int i = 0; i < 1; i++);
    gpio_put(shiftRegister->updateData, 1);

    for (uint8_t i = 0; i < shiftRegister->registerCount; i++) {
        pio_sm_put(shiftRegister->pio, shiftRegister->sm, 0xFF);
        dataArray[i] = pio_sm_get_blocking(shiftRegister->pio, shiftRegister->sm);
    }
}

void print_bits(uint32_t data, uint8_t dataSize) {
    for (uint8_t i = 0; i < dataSize; i++) {
        printf("%d", (data >> (dataSize-1-i)) & 1);
    }
}

void shift_register_example() {
    ShiftRegister shiftRegisterInput;
    ShiftRegister shiftRegisterOutput;
    PIO shiftRegisterPIO = pio0;

    // Loads program to the specified PIO's memory
    uint offset = pio_add_program(shiftRegisterPIO, &shift_register_program);

    shiftRegisterOutput.pio = shiftRegisterPIO;
    shiftRegisterOutput.sm = pio_claim_unused_sm(shiftRegisterOutput.pio, true);
    shiftRegisterOutput.dataPin = 10;
    shiftRegisterOutput.clockPin = 11;
    shiftRegisterOutput.updateData = 12;
    shiftRegisterOutput.registerCount = 1;
    init_out_shift_register(&shiftRegisterOutput, offset, 41.666 * 1000 * 1000);

    shiftRegisterInput.pio = shiftRegisterPIO;
    shiftRegisterInput.sm = pio_claim_unused_sm(shiftRegisterInput.pio, true);
    shiftRegisterInput.dataPin = 13;
    shiftRegisterInput.clockPin = 14;
    shiftRegisterInput.updateData = 15;
    shiftRegisterInput.registerCount = 1;
    init_in_shift_register(&shiftRegisterInput, offset, 1 * 1000 * 1000);

    gpio_set_dir(16, false);
    gpio_pull_down(16);
    while (true) {
        sleep_ms(500);
        if (!gpio_get(16)) {
            continue;
        }

        uint8_t data_out[18] = {0b10101010, 0b01010101};
        uint8_t data_in[9];

        write_to_shift_register(&shiftRegisterOutput, &data_out[0]);
        write_to_shift_register(&shiftRegisterOutput, &data_out[1]);
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
}
