#ifndef C744051_H
#define C744051_H

#include "pico/stdlib.h"

#define NO_DISABLE_PIN 255

typedef struct C744051 {
    uint8_t common;  // Pico's analog pin connected to chip
    uint8_t disable;  // Enable pin, only used when multiple ic's are in a parallel configuration
    uint8_t S0;
    uint8_t S1;
    uint8_t S2;

    uint32_t pin_mask;
} C744051;

void init_744051_adc();
void init_744051(C744051 *c744051, uint8_t common, uint8_t disable, uint8_t S0, uint8_t S1, uint8_t S2);
void read_744051_masked(C744051 c744051, uint8_t read_pin_mask, uint8_t samples, uint16_t *data, bool extra_precision);
void read_multiple_744051(C744051 *c744051, uint8_t ic_count, uint8_t samples, uint16_t *data, bool extra_precision);
double adc_to_voltage(int adc_value);
void C744051_example();

#endif
