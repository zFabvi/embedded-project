#ifndef FILTERS_API_H
#define FILTERS_API_H

#include <stdint.h>

void filter_reset_mvavg(void);
void filter_apply_mvavg(uint16_t* pot_val, uint16_t* ntc_val);
uint32_t xorshift32(void);
void filter_apply_noise(uint16_t* pot_val, uint16_t* ntc_val);

#endif