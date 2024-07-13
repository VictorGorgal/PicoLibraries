#include "hardware/adc.h"

typedef struct C744051 {
    uint8_t common;  // Pico's analog pin connected to chip
    uint8_t S0;
    uint8_t S1;
    uint8_t S2;

    uint32_t pin_mask;
} C744051;

/**
 * @brief Initializes the Pico's ADC.
 * Should only be run once on startup
 */
void init_744051_adc() {
    adc_init();
}

/**
 * @brief Initializes the 744051
 *
 * @param[in] common Pico's analog pin connected to chip
 * @param[in] S0 Address pin S0
 * @param[in] S1 Address pin S1
 * @param[in] S2 Address pin S2
 *
 * @return Struct used by other library functions
 */
C744051 init_744051(uint8_t common, uint8_t S0, uint8_t S1, uint8_t S2) {
    C744051 c744051;

    c744051.common = common;
    c744051.S0 = S0;
    c744051.S1 = S1;
    c744051.S2 = S2;

    c744051.pin_mask = 0;
    c744051.pin_mask |= (0b1 << S0);
    c744051.pin_mask |= (0b1 << S1);
    c744051.pin_mask |= (0b1 << S2);
    
    adc_gpio_init(common);
    gpio_init_mask(c744051.pin_mask);
    gpio_set_dir_masked(c744051.pin_mask, 0xFFFFFFFF);
    gpio_put_masked(c744051.pin_mask, 0);

    return c744051;
}

/**
 * @brief Converts the pin number into the analog input select
 *
 * @param[in] analog_pin Analog pin number on the Pico.
 *
 * @return The input select used in the adc_select_input() function.
 */
uint8_t _analog_pin_to_input_select(uint8_t analog_pin) {
    return analog_pin - 26;
}

/**
 * @brief Selects which adc to read from.
 * 
 * This function should be run before the read functions if another analog input was selected
 * or if state is unknown
 *
 * @param[in] c744051 744051 struct.
 */
void select_input_744051(C744051 c744051, uint8_t channel) {
    if (adc_get_selected_input() != _analog_pin_to_input_select(c744051.common)) {
        adc_select_input(_analog_pin_to_input_select(c744051.common));
    }

    uint32_t state_mask = 0;
    state_mask |= (((channel & 0b001)) << c744051.S0);
    state_mask |= (((channel & 0b010) >> 1) << c744051.S1);
    state_mask |= (((channel & 0b100) >> 2) << c744051.S2);
    gpio_put_masked(c744051.pin_mask, state_mask);
}

/**
 * @brief Reads analog value from specified chip pin
 *
 * @param[in] c744051 744051 struct.
 * @param[in] channel 744051's channel pin to read from.
 * @param[in] samples How many samples to do for each pin.
 *
 * @return The analog value.
 */
uint16_t __time_critical_func(read_744051_channel)(C744051 c744051, uint8_t channel, uint8_t samples) {
    if (samples <= 1) {
        return adc_read();
    }

    uint32_t sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += adc_read();
    }
    return sum / samples;
}

/**
 * @brief Reads analog value from multiple pins from chip and saves to the given array
 *
 * @param[in] c744051 744051 struct.
 * @param[in] read_pin_mask mask of which pins to read from.
 * @param[in] samples How many samples to do for each pin.
 * @param[in] data uint16_t array.
 */
void read_744051(C744051 c744051, uint8_t read_pin_mask, uint8_t samples, uint16_t *data) {
    uint8_t data_counter = 0;
    for (int channel = 0; channel < 8; channel++) {
        if ((read_pin_mask >> channel) & 0b1) {
            select_input_744051(c744051, channel);
            data[data_counter++] = read_744051_channel(c744051, channel, samples);
        }
    }
}

double adc_to_voltage(int adc_value) {
    return (adc_value * 3.3) / (4096 - 1);
}

void C744051_example() {
    init_744051_adc();
    C744051 c744051 = init_744051(26, 10, 11, 12);

    gpio_init(16);
    gpio_set_dir(16, false);
    gpio_pull_down(16);
    while (true) {
        sleep_ms(500);
        if (!gpio_get(16)) {
            continue;
        }

        uint16_t data[6];
        select_input_744051(c744051, 0);
        read_744051(c744051, 0b00111111, 3, data);
        select_input_744051(c744051, 0);

        for (int i = 0; i < 5; i++) {
            printf("%f, ", adc_to_voltage(data[i]));
        }
        printf("%f\n", adc_to_voltage(data[5]));
    }
}
