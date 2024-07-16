#ifndef SHIFT_REGISTER_H
#define SHIFT_REGISTER_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "shift_register.pio.h"

typedef struct ShiftRegister {
    PIO pio;
    uint sm;
    uint8_t registerCount;
    uint8_t dataPin;
    uint8_t clockPin;
    uint8_t updateData;
} ShiftRegister;

void init_out_shift_register(ShiftRegister *shiftRegister, uint offset, float clock);
void init_in_shift_register(ShiftRegister *shiftRegister, uint offset, float clock);
void write_to_shift_register(ShiftRegister *shiftRegister, uint8_t *dataArray);
void read_from_shift_register(ShiftRegister *shiftRegister, uint8_t dataArray[]);
void shift_register_example();

#endif
