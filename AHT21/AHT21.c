#include "hardware/i2c.h"

const uint8_t AHT21_ADDRESS = 0x38;
const uint8_t INIT_COMMAND = 0x71;
const uint8_t MEASURE_COMMAND[3] = {0xAC, 0x33, 0x00};
i2c_inst_t *_i2c_channel;

float _humidityFromRawData(uint8_t *rawData) {
    uint32_t raw_humidity = (rawData[1] << 12) | (rawData[2] << 4) | ((rawData[3] & 0xF0) >> 4);
    return ((float)raw_humidity / (float)(1 << 20)) * 100;
}

float _temperatureFromRawData(uint8_t *rawData) {
    uint32_t raw_temperature = ((rawData[3] & 0x0F) << 16) | (rawData[4] << 8) | (rawData[5]);
    return ((float)raw_temperature / (float)(1 << 20)) * 200 - 50;
}

// Wait at least 100ms after power up to initiale the AHT21
void AHT21_init(i2c_inst_t *i2c_channel) {
    _i2c_channel = i2c_channel;
    uint8_t src = 0x71;
    uint8_t answer[1];
    i2c_write_blocking(_i2c_channel, 0x38, &src, 1, false);
    i2c_read_blocking(_i2c_channel, 0x38, answer, 1, false);
}

// Wait at least 10ms after init to start measurement
void AHT21_startMeasurement() {
    i2c_write_blocking(_i2c_channel, AHT21_ADDRESS, MEASURE_COMMAND, 3, false);
}

// Wait at least 80ms after start measurement to read it
void AHT21_readMeasurement(float *data) {
    uint8_t rawData[7];
    i2c_read_blocking(_i2c_channel, AHT21_ADDRESS, rawData, 7, false);
    data[0] = _humidityFromRawData(rawData);
    data[1] = _temperatureFromRawData(rawData);
}

// Wait at least 10ms after init to start measurement
void AHT21_getMeasurementBlocking(float *data) {
    AHT21_startMeasurement();
    sleep_ms(80);
    AHT21_readMeasurement(data);
}

// Pins 4, 5 -> i2c
// Pin 16 -> trigger measurement button
void AHT21_example() {
    // Configure button
    gpio_set_dir(16, false);
    gpio_pull_down(16);

    // Configure i2c
    i2c_init(i2c0, 400000);
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_pull_up(5);

    // Init AHT21
    sleep_ms(100);
    AHT21_init(i2c0);

    while (true) {
        sleep_ms(10);

        // Only measure if button is pressed
        if (!gpio_get(16)) {
            continue;
        }

        // Gets measurement
        float data[2];
        AHT21_getMeasurementBlocking(data);
        printf("H:%.2f%%, T:%.2fC\n", data[0], data[1]);
    }
}
