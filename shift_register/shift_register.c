#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "shift_register_in.pio.h"
#include "shift_register_out.pio.h"

// ToDo check if wrapping read and write with time_crit_function solves the unreliable first timing
// ToDo check if DMA can help with read stopping between bytes
// ToDo implement bigger buffer if DMA cant help with above
// ToDo try to use read PIO code with write function using pio_sm_get() to clear in buffer
// ToDo clean code

typedef struct ShiftRegister {
    PIO pio;
    uint sm;
    uint8_t registerCount;
    uint8_t dataPin;
    uint8_t clockPin;
    uint8_t updateData;
} ShiftRegister;

// uint load_shift_register_program(PIO pio) {
//     pio_add_program(pio, &shift_register_out_program);
//     return pio_claim_unused_sm(pio, true);
// }

// SIPO
// Max freq. 41.666MHz
// Recommended freq:
// 74HC164 - 10MHz
// 74HC595 - ??MHz
void init_out_shift_register(ShiftRegister *shiftRegister, uint offset, float clock) {
    PIO pio = shiftRegister->pio;
    uint sm = shiftRegister->sm;

    clock *= 3;
    float clockDiv = (float) clock_get_hz(clk_sys) / clock;
    pio_sm_config c = shift_register_out_program_get_default_config(offset);

    gpio_init(shiftRegister->updateData);
    gpio_set_dir(shiftRegister->updateData, true);
    gpio_put(shiftRegister->updateData, 0);

    pio_gpio_init(pio, shiftRegister->dataPin);
    pio_gpio_init(pio, shiftRegister->clockPin);

    sm_config_set_out_pins(&c, shiftRegister->dataPin, 1);
    pio_sm_set_consecutive_pindirs(pio, shiftRegister->sm, shiftRegister->dataPin, 1, true);
    sm_config_set_sideset_pins(&c, shiftRegister->clockPin);
    pio_sm_set_consecutive_pindirs(pio, shiftRegister->sm, shiftRegister->clockPin, 1, true);

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
    pio_sm_config c = shift_register_in_program_get_default_config(offset);

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
void write_to_shift_register(ShiftRegister *shiftRegister, uint8_t *dataArray, bool blocking) {
    for (uint8_t i = 0; i < shiftRegister->registerCount; i++) {
        pio_sm_put(shiftRegister->pio, shiftRegister->sm, dataArray[i]);
    }

    if (!blocking) {
        return;
    }

    uint32_t SM_STALL_MASK = 1u << (PIO_FDEBUG_TXSTALL_LSB + shiftRegister->sm);
    shiftRegister->pio->fdebug = SM_STALL_MASK;

    while(!(shiftRegister->pio->fdebug & SM_STALL_MASK)) {
        tight_loop_contents();
    }

    sleep_us(1);
    gpio_put(shiftRegister->updateData, 1);
    sleep_us(1);
    gpio_put(shiftRegister->updateData, 0);
}

// 0bHGFEDCBA <- input
void read_from_shift_register(ShiftRegister *shiftRegister, uint8_t dataArray[]) {
    gpio_put(shiftRegister->updateData, 0);
    sleep_us(1);
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

    shiftRegisterOutput.pio = pio0;
    shiftRegisterOutput.sm = pio_claim_unused_sm(shiftRegisterOutput.pio, true);
    shiftRegisterOutput.dataPin = 10;
    shiftRegisterOutput.clockPin = 11;
    shiftRegisterOutput.updateData = 12;
    shiftRegisterOutput.registerCount = 3;

    uint offset2 = pio_add_program(shiftRegisterOutput.pio, &shift_register_out_program);
    init_out_shift_register(&shiftRegisterOutput, offset2, 1 * 1000 * 1000);

    shiftRegisterInput.pio = pio0;
    shiftRegisterInput.sm = pio_claim_unused_sm(shiftRegisterInput.pio, true);
    shiftRegisterInput.dataPin = 13;
    shiftRegisterInput.clockPin = 14;
    shiftRegisterInput.updateData = 15;
    shiftRegisterInput.registerCount = 3;

    uint offset = pio_add_program(shiftRegisterInput.pio, &shift_register_in_program);
    init_in_shift_register(&shiftRegisterInput, offset, 1 * 1000 * 1000);

    gpio_set_dir(16, false);
    gpio_pull_down(16);
    while (true) {
        sleep_ms(500);
        watchdog_update();
        printf(".");
        if (!gpio_get(16)) {
            continue;
        }

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
}
