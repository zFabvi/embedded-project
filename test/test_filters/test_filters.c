#include "unity.h"
#include "filters-api.h"

void setUp(void) {
    filter_reset_mvavg();
}

void tearDown(void) {}

void test_filter_mvavg_smoothing(void) {
    uint16_t pot = 1000;
    uint16_t ntc = 2000;
    filter_apply_mvavg(&pot, &ntc);
    TEST_ASSERT_EQUAL_UINT16(1000, pot);
    TEST_ASSERT_EQUAL_UINT16(2000, ntc);

    pot = 2000;
    ntc = 4000;
    filter_apply_mvavg(&pot, &ntc);
    TEST_ASSERT_EQUAL_UINT16(1500, pot);
    TEST_ASSERT_EQUAL_UINT16(3000, ntc);
}

void test_filter_noise_saturation(void) {
    uint16_t pot = 4090;
    uint16_t ntc = 4090;
    for (int i = 0; i < 20; i++) {
        filter_apply_noise(&pot, &ntc);
        TEST_ASSERT_LESS_OR_EQUAL_UINT16(4095, pot);
        TEST_ASSERT_LESS_OR_EQUAL_UINT16(4095, ntc);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_filter_mvavg_smoothing);
    RUN_TEST(test_filter_noise_saturation);
    return UNITY_END();
}