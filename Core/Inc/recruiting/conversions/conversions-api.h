#ifndef CONVERSIONS_API_H
#define CONVERSIONS_API_H

#include <stdint.h>

float calc_pot_volts(uint16_t raw_pot);
float calc_ntc_temp(uint16_t raw_ntc);
void format_sensor_data(uint16_t raw_pot, float pot_v, uint16_t raw_ntc, float temp_c, char* buffer);

#endif