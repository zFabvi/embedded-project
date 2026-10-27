#include "filters-api.h"
#include "filters.h"
#include <stdint.h>
#include <string.h>

static uint16_t pot_history[MVAVG_SIZE] = {0}; /*!< Circular buffer gathering the last 150 POT values */
static uint16_t ntc_history[MVAVG_SIZE] = {0}; /*!< Circular buffer gathering the last 150 NTC values */

static uint32_t rnd_state = 314159265; /*!< Xorshift32 pseudorandom number generator seed */

/*! 
 * \brief Rolling sum of POT & NTC values.
 * 
 * Max possible value is 4095 (12-bit ADC) * 150 (window size) = 614250.
 * A uint32_t is required to prevent overflow during calculations.
 */
static uint32_t pot_sum = 0;
static uint32_t ntc_sum = 0;

static uint16_t mvavg_index = 0; /*!< Current write index for the circular buffers */
static uint16_t mvavg_filled = 0; /*!< Number of valid samples currently stored in the buffers */

/*!
 * \brief Resets the internal state of the moving average filter.
 * 
 * Clears the current sums, circular buffer index, and fill counter. 
 * Call this before starting a new data acquisition session.
 */
void filter_reset_mvavg(void) {
    pot_sum = 0;
    ntc_sum = 0;
    mvavg_index = 0;
    mvavg_filled = 0;
    memset(pot_history, 0, sizeof(pot_history));
    memset(ntc_history, 0, sizeof(ntc_history));
}

/*!
 * \brief Applies a moving average (rolling sum) filter to the sensor data.
 * 
 * Replaces the oldest sample with the new one in the circular buffer 
 * and updates the sum. Modifies the passed values by reference, overwriting 
 * them with the calculated average.
 * 
 * \param pot_val Pointer to the current potentiometer value.
 * \param ntc_val Pointer to the current NTC thermistor value.
 */
void filter_apply_mvavg(uint16_t* pot_val, uint16_t* ntc_val) {

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

/*!
 * \brief Generates a high-performance 32-bit pseudorandom number.
 * 
 * Replaces the standard rand() function. Uses the Xorshift algorithm, 
 * relying exclusively on bitwise operations (shift and XOR) to minimize 
 * clock cycle consumption.
 * 
 * \return uint32_t The generated pseudorandom number.
 */
uint32_t xorshift32(void) {
    rnd_state ^= rnd_state << 13;
    rnd_state ^= rnd_state >> 17;
    rnd_state ^= rnd_state << 5;
    return rnd_state;
}

/*!
 * \brief Injects artificial noise (spikes) into the sensor signals.
 * 
 * With a 75% probability, adds random positive spikes to the input 
 * values (max +1000 for POT, max +750 for NTC). The resulting values 
 * are saturated to 4095 to respect the 12-bit ADC limit.
 * 
 * \param pot_val Pointer to the potentiometer original value.
 * \param ntc_val Pointer to the NTC thermistor original value.
 */
void filter_apply_noise(uint16_t* pot_val, uint16_t* ntc_val) {
    int spike[2] = {0, 0};
    if ((xorshift32() % 100) < 75) { // 75%
        spike[0] = xorshift32() % 1000;
        spike[1] = xorshift32() % 750; 
    }

    int noisy_pot = *pot_val + spike[0];
    if (noisy_pot > 4095) noisy_pot = 4095;
    *pot_val = (uint16_t)noisy_pot;

    int noisy_ntc = *ntc_val + spike[1]; 
    if (noisy_ntc > 4095) noisy_ntc = 4095;
    *ntc_val = (uint16_t)noisy_ntc;
}