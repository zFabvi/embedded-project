#include "unity.h"
#include "conversions-api.h"

void setUp(void) {}
void tearDown(void) {}

void test_calc_pot_volts(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, calc_pot_volts(0));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.3f, calc_pot_volts(4095));
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 1.65f, calc_pot_volts(2048));
}

void test_calc_ntc_temp(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, calc_ntc_temp(0));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, calc_ntc_temp(4095));
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 25.0f, calc_ntc_temp(2048));
}

void test_format_sensor_data(void) {
    char buf[100];
    format_sensor_data(1500, 1.21f, 2048, 25.0f, buf);
    TEST_ASSERT_EQUAL_STRING("POT_RAW: 1500 -> POT: 1.21 V | TEMP_RAW: 2048 -> TEMP: 25.0 C\r\n", buf);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_calc_pot_volts);
    RUN_TEST(test_calc_ntc_temp);
    RUN_TEST(test_format_sensor_data);
    return UNITY_END();
}