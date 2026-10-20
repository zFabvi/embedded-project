#ifndef FILTERS_H
#define FILTERS_H

#include <stdint.h>

void filter_reset_mvavg(void);
void filter_apply_mvavg(uint16_t* pot_val, uint16_t* ntc_val);
void filter_apply_noise(uint16_t* pot_val, uint16_t* ntc_val);

#endif