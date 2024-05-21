#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "shift_register_in.pio.h"
#include "shift_register_out.pio.h"

volatile uint32_t data = 0;
int dma_chan = 0;

typedef struct ShiftRegister {
    PIO pio;
    uint sm;
    uint8_t registerCount;
    uint8_t dataPin;
    uint8_t clockPin;
} ShiftRegister;


// SIPO
// Max freq. 30MHz
void init_out_shift_register(ShiftRegister *shiftRegister, uint offset, float clock) {
    clock *= shift_register_out_program.length;
    float clockDiv = (float) clock_get_hz(clk_sys) / clock;
    pio_sm_config c = shift_register_out_program_get_default_config(offset);

    pio_gpio_init(shiftRegister->pio, shiftRegister->dataPin);
    pio_gpio_init(shiftRegister->pio, shiftRegister->clockPin);

    sm_config_set_out_pins(&c, shiftRegister->dataPin, 1);
    pio_sm_set_consecutive_pindirs(shiftRegister->pio, shiftRegister->sm, shiftRegister->dataPin, 1, true);
    sm_config_set_sideset_pins(&c, shiftRegister->clockPin);
    pio_sm_set_consecutive_pindirs(shiftRegister->pio, shiftRegister->sm, shiftRegister->clockPin, 1, true);

    sm_config_set_out_shift(&c, true, true, 8);

    sm_config_set_clkdiv(&c, clockDiv);
    pio_sm_init(shiftRegister->pio, shiftRegister->sm, offset, &c);
    pio_sm_set_enabled(shiftRegister->pio, shiftRegister->sm, true);
}

// PISO
// Max freq. 10MHz
void init_in_shift_register(ShiftRegister *shiftRegister, uint offset, float clock) {
    PIO pio = shiftRegister->pio;
    uint sm = shiftRegister->sm;

    clock *= 5;
    float clockDiv = (float) clock_get_hz(clk_sys) / clock;
    pio_sm_config c = shift_register_in_program_get_default_config(offset);

    pio_gpio_init(pio, shiftRegister->dataPin);
    pio_gpio_init(pio, shiftRegister->clockPin);

    sm_config_set_in_pins(&c, shiftRegister->dataPin);
    pio_sm_set_consecutive_pindirs(pio, sm, shiftRegister->dataPin, 1, false);
    sm_config_set_sideset_pins(&c, shiftRegister->clockPin);
    pio_sm_set_consecutive_pindirs(pio, sm, shiftRegister->clockPin, 1, true);

    sm_config_set_in_shift(&c, false, true, 8);

    sm_config_set_clkdiv(&c, clockDiv);
    pio_sm_init(pio, sm, offset, &c);

    dma_channel_config dc = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&dc, false);
    channel_config_set_write_increment(&dc, false);
    channel_config_set_dreq(&dc, pio_get_dreq(shiftRegister->pio, shiftRegister->sm, false));
    channel_config_set_transfer_data_size( &dc, DMA_SIZE_8 );

    dma_channel_configure(dma_chan, &dc,
            &data,                    // Destination pointer (local var)
            &shiftRegister->pio->rxf[shiftRegister->sm],           // Source pointer (receive from PIO FIFO)
            1,                                  // Number of transfers
            true);
            
    pio_sm_set_enabled(pio, sm, true);
}

void write_to_shift_register(ShiftRegister *shiftRegister, uint32_t *dataArray) {
    for (uint8_t i = 0; i < shiftRegister->registerCount; i++) {
        pio_sm_put_blocking(shiftRegister->pio, shiftRegister->sm, dataArray[i]);
    }
}

void read_from_shift_register(ShiftRegister *shiftRegister, uint32_t dataArray[]) {
    pio_sm_put_blocking(shiftRegister->pio, shiftRegister->sm, 1);
    dma_channel_wait_for_finish_blocking(dma_chan);
    dataArray[0] = data;
    dma_channel_start(dma_chan);
}
