#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "shift_register_in.pio.h"
#include "shift_register_out.pio.h"

uint8_t data = 0;
int dma_chan1 = 0;

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
}

// 0bHGFEDCBA <- input
void read_from_shift_register(ShiftRegister *shiftRegister, uint8_t dataArray[]) {
    gpio_put(shiftRegister->updateData, 0);
    gpio_put(shiftRegister->updateData, 1);

    for (uint8_t i = 0; i < shiftRegister->registerCount; i++) {
        pio_sm_put(shiftRegister->pio, shiftRegister->sm, 0xFF);
        dataArray[i] = pio_sm_get_blocking(shiftRegister->pio, shiftRegister->sm);
    }
}
