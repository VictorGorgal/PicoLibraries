#include "744051.h"
#include "hardware/adc.h"
#include <stdio.h>

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
 * @param[in] c744051 Pointer to c744051 struct to initialize
 * @param[in] common Pico's analog pin connected to chip
 * @param[in] disable Disable pin, used if more than one IC is connected to the same analog input, otherwise pass NO_DISABLE_PIN
 * @param[in] S0 Address pin S0
 * @param[in] S1 Address pin S1
 * @param[in] S2 Address pin S2
 */
void init_744051(C744051 *c744051, uint8_t common, uint8_t disable, uint8_t S0, uint8_t S1, uint8_t S2) {
    c744051->common = common;
    c744051->disable = disable;
    c744051->S0 = S0;
    c744051->S1 = S1;
    c744051->S2 = S2;

    c744051->pin_mask = 0;
    c744051->pin_mask |= (0b1 << S0);
    c744051->pin_mask |= (0b1 << S1);
    c744051->pin_mask |= (0b1 << S2);
    
    adc_gpio_init(common);

    gpio_init_mask(c744051->pin_mask);
    gpio_set_dir_masked(c744051->pin_mask, 0xFFFFFFFF);
    gpio_put_masked(c744051->pin_mask, 0);

    if (disable != NO_DISABLE_PIN) {
        gpio_init(disable);
        gpio_set_dir(disable, true);
        gpio_put(disable, true);
    }
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
 * @brief Selects which Pico's adc to read from.
 * 
 * This function should be run before the read functions if another analog input was selected
 * or if state is unknown
 *
 * @param[in] c744051 744051 struct.
 */
void _select_adc_input_744051(C744051 c744051) {
    if (adc_get_selected_input() != _analog_pin_to_input_select(c744051.common)) {
        adc_select_input(_analog_pin_to_input_select(c744051.common));
    }
}

/**
 * @brief Selects which channel to read from.
 * 
 * This function should be run before the read functions if another channel was selected prior
 * or if state is unknown
 *
 * @param[in] c744051 744051 struct.
 * @param[in] channel Channel to select (0-7).
 */
void _select_input_744051(C744051 c744051, uint8_t channel) {
    uint32_t state_mask = 0;
    state_mask |= (((channel & 0b001)) << c744051.S0);
    state_mask |= (((channel & 0b010) >> 1) << c744051.S1);
    state_mask |= (((channel & 0b100) >> 2) << c744051.S2);
    gpio_put_masked(c744051.pin_mask, state_mask);
}

/**
 * @brief Reads single analog value from one 744051 IC.
 * 
 * This function automatically averages multiple readings if passed more than 1 sample.
 * Defaults to 1 sample if passed 0.
 *
 * @param[in] c744051 744051 struct.
 * @param[in] channel 744051's channel to read from (0-7).
 * @param[in] samples How many samples to do for each pin.
 *
 * @return The analog value.
 */
uint16_t _read_single_744051_channel(uint8_t samples) {
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
 * @brief Reads one or more analog values from one 744051 IC and saves to the given array
 *
 * @param[in] c744051 744051 struct.
 * @param[in] read_pin_mask mask of which pins to read from.
 * @param[in] samples How many samples to do for each pin.
 * @param[in] data uint16_t array of length 8.
 * @param[in] extra_precision Due to the ADC's capacitance, there is a small error of +-20mV due to the speed in which this function
 * operates at, enabling this option increases the time of measurement for more accuracy, pair this with high sample.
 */
void read_744051_masked(C744051 c744051, uint8_t read_pin_mask, uint8_t samples, uint16_t *data, bool extra_precision) {
    _select_adc_input_744051(c744051);

    uint8_t data_counter = 0;
    for (int channel = 0; channel < 8; channel++) {
        if ((read_pin_mask >> channel) & 0b1) {
            _select_input_744051(c744051, channel);

            if (extra_precision) {
                sleep_us(5);
            }

            data[data_counter++] = _read_single_744051_channel(samples);
        }
    }
}

/**
 * @brief Reads all analog values from multiple 744051 ICs and saves to the given array
 * 
 * The ICs can be wired in two ways, multiple ICs to the same analog input or to different analog inputs:
 * - If there are more than one IC connected to the same analog input, both must have a dedicated disable pin.
 * - If there is only one IC on a single analog input, that specific IC can tie the disable to ground.
 * 
 * This function automatically detects which way they are connected and measure all channels, it is possible
 * to mix the different connection types.
 *
 * @param[in] c744051 744051 struct array withh all IC's to read from.
 * @param[in] ic_count Size of the 744051 struct array.
 * @param[in] samples How many samples to do for each pin.
 * @param[in] data uint16_t array of length 8 * ic count (example, if ic_count = 3, length must be 8 * 3 = 24).
 * @param[in] extra_precision Due to the ADC's capacitance, there is a small error of +-20mV due to the speed in which this function
 * operates at, enabling this option increases the time of measurement for more accuracy, pair this with high sample.
*/
void read_multiple_744051(C744051 *c744051, uint8_t ic_count, uint8_t samples, uint16_t *data, bool extra_precision) {
    for (uint8_t channel = 0; channel < 8; ++channel) {
        _select_input_744051(c744051[0], channel);

        for (uint8_t i = 0; i < ic_count; ++i) {
            C744051 ic = c744051[i];

            if (ic.disable != NO_DISABLE_PIN) {
                gpio_put(ic.disable, 0);
            }

            if (extra_precision) {
                sleep_us(5);
            }

            _select_adc_input_744051(ic);
            
            uint16_t adc_value = adc_read();
            data[i * 8 + channel] = adc_value;

            if (ic.disable != NO_DISABLE_PIN) {
                gpio_put(ic.disable, 1);
            }
        }
    }
}

double adc_to_voltage(int adc_value) {
    return (adc_value * 3.27) / (4096 - 1);
}

void C744051_example() {
    init_744051_adc();

    C744051 c744051[3];
    init_744051(&c744051[0], 26, 13, 10, 11, 12);
    init_744051(&c744051[1], 26, 14, 10, 11, 12);
    init_744051(&c744051[2], 28, NO_DISABLE_PIN, 10, 11, 12);

    gpio_init(18);
    gpio_set_dir(18, false);
    gpio_pull_down(18);
    while (true) {
        sleep_ms(500);
        if (!gpio_get(18)) {
            continue;
        }

        uint16_t data[24];
        read_multiple_744051(c744051, 3, 1, data, false);

        for (int j = 0; j < 3; j++) {
            for (int i = 0; i < 7; i++) {
                printf("%.2f ", adc_to_voltage(data[8*j + i]));
            }
            printf("%.2f\n", adc_to_voltage(data[8*j + 7]));
        }
        printf("\n");
    }
}
