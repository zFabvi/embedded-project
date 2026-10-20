#include "../../Inc/recruiting/filters.h"
#include <stdint.h>
#include <stdlib.h>

#define MVAVG_SIZE 150

// gathering the last 150 values
static uint16_t pot_history[MVAVG_SIZE] = {0};
static uint16_t ntc_history[MVAVG_SIZE] = {0};

// we need to store the sum of the values
// |--> where the max value is 4095 * 150
// |--> so we use uint32
static uint32_t pot_sum = 0;
static uint32_t ntc_sum = 0;

static uint16_t mvavg_index = 0;
static uint16_t mvavg_filled = 0;

void filter_reset_mvavg(void) {
    pot_sum = 0;
    ntc_sum = 0;
    mvavg_index = 0;
    mvavg_filled = 0;
}

void filter_apply_mvavg(uint16_t* pot_val, uint16_t* ntc_val) {
    pot_sum -= pot_history[mvavg_index];
    ntc_sum -= ntc_history[mvavg_index];

    pot_sum -= pot_history[mvavg_index];
    ntc_sum -= ntc_history[mvavg_index];

    pot_history[mvavg_index] = *pot_val;
    ntc_history[mvavg_index] = *ntc_val;

    pot_sum += *pot_val;
    ntc_sum += *ntc_val;

    mvavg_index = (mvavg_index + 1) % MVAVG_SIZE;

    if (mvavg_filled < MVAVG_SIZE) mvavg_filled++;

    *pot_val = (uint16_t)(pot_sum / mvavg_filled);
    *ntc_val = (uint16_t)(ntc_sum / mvavg_filled);
}

void filter_apply_noise(uint16_t* pot_val, uint16_t* ntc_val) {
    int spike[2] = {0, 0};
    if ((rand() % 100) < 75) { // 75%
        spike[0] = rand() % 1000;
        spike[1] = rand() % 750; 
    }

    int noisy_pot = *pot_val + spike[0];
    if (noisy_pot > 4095) noisy_pot = 4095;
    *pot_val = (uint16_t)noisy_pot;

    int noisy_ntc = *ntc_val + spike[1]; 
    if (noisy_ntc > 4095) noisy_ntc = 4095;
    *ntc_val = (uint16_t)noisy_ntc;
}