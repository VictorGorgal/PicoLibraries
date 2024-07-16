#ifndef AHT21_H
#define AHT21_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define AHT21_ADDRESS 0x38
#define AHT21_INIT_COMMAND 0x71
const uint8_t MEASURE_COMMAND[3] = {0xAC, 0x33, 0x00};
i2c_inst_t *_i2c_channel;

void AHT21_init(i2c_inst_t *i2c_channel);
void AHT21_startMeasurement();
void AHT21_readMeasurement(float *data);
void AHT21_getMeasurementBlocking(float *data);
void AHT21_example();

#endif
