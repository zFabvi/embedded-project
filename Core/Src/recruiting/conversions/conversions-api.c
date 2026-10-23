#include "conversions-api.h"
#include "conversions.h"
#include <stdio.h>
#include <math.h>

/*!
 * \brief Converts the raw ADC value to voltage for the potentiometer.
 * \param raw_pot 12-bit digital value (0-4095) read from the ADC.
 * \return Calculated voltage in Volts (0.0f - 3.3f).
 */
float calc_pot_volts(uint16_t raw_pot) {
    return (raw_pot * 3.3f) / 4095.0f;
}

/*!
 * \brief Calculates the temperature in degrees Celsius from the raw NTC value.
 * \param raw_ntc 12-bit digital value (0-4095) read from the ADC.
 * \return Temperature in Celsius. Returns 0.0f if the sensor is disconnected or shorted.
 */
float calc_ntc_temp(uint16_t raw_ntc) {
    if (raw_ntc == 0 || raw_ntc >= 4095) {
        return 0.0f; 
    }

    const float R_REF = 10000.0f;
    const float R0 = 10000.0f;
    const float BETA = 3950.0f;
    const float T0 = 298.15f;

    float r_ntc = R_REF * ((float)raw_ntc / (4095.0f - (float)raw_ntc));
    float temp_k = 1.0f / ((1.0f / T0) + (1.0f / BETA) * logf(r_ntc / R0));
    
    return temp_k - 273.15f;
}

/*!
 * \brief Formats the sensor data (both RAW and converted) into a printable string.
 * \param raw_pot Raw ADC value of the potentiometer.
 * \param pot_v Converted voltage of the potentiometer.
 * \param raw_ntc Raw ADC value of the NTC.
 * \param temp_c Converted temperature in Celsius.
 * \param buffer Pointer to the character array for the output string.
 */
void format_sensor_data(uint16_t raw_pot, float pot_v, uint16_t raw_ntc, float temp_c, char* buffer) {
    int pot_v_int = (int)pot_v;
    int pot_v_dec = (int)((pot_v - pot_v_int) * 100);

    int temp_c_int = (int)temp_c;
    int temp_c_dec = (int)(fabsf(temp_c - temp_c_int) * 10);

    sprintf(buffer, "POT_RAW: %u -> POT: %d.%02d V | TEMP_RAW: %u -> TEMP: %d.%d C\r\n", 
            raw_pot, pot_v_int, pot_v_dec, raw_ntc, temp_c_int, temp_c_dec);
}