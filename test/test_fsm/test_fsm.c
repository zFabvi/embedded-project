#include <string.h>
#include "unity.h"
#include "fsm-api.h"

static uint16_t test_adc_buf[2];
static struct SystemContext test_ctx;

void setUp(void) {
    test_adc_buf[0] = 2000;
    test_adc_buf[1] = 2000;

    memset(&test_ctx, 0, sizeof(test_ctx));
    test_ctx.adc_buffer = test_adc_buf;
    test_ctx.error_msg = "Unknown error";

    system_fsm_init();
}

void tearDown(void) {}

void test_fsm_post_to_waiting_when_sensors_valid(void) {
    TEST_ASSERT_EQUAL_UINT8(STATE_POST, system_fsm_get_state());
    system_fsm_run(&test_ctx);
    TEST_ASSERT_EQUAL_UINT8(STATE_WAITING, system_fsm_get_state());
}

void test_fsm_post_to_fatal_when_sensor_disconnected(void) {
    test_adc_buf[0] = 0;
    system_fsm_run(&test_ctx);
    TEST_ASSERT_EQUAL_UINT8(STATE_FATAL, system_fsm_get_state());
}

void test_fsm_waiting_to_listening_and_paused_on_button(void) {
    system_fsm_run(&test_ctx);
    TEST_ASSERT_EQUAL_UINT8(STATE_WAITING, system_fsm_get_state());

    test_ctx.btn_pressed = 1;
    system_fsm_run(&test_ctx);
    TEST_ASSERT_EQUAL_UINT8(STATE_LISTENING, system_fsm_get_state());

    test_ctx.btn_pressed = 1;
    system_fsm_run(&test_ctx);
    TEST_ASSERT_EQUAL_UINT8(STATE_PAUSED, system_fsm_get_state());
}

void test_fsm_listening_to_warning_after_5s_threshold(void) {
    system_fsm_run(&test_ctx);
    test_ctx.btn_pressed = 1;
    system_fsm_run(&test_ctx);

    test_adc_buf[0] = 1000;
    test_adc_buf[1] = 1600;

    test_ctx.tick_ms = 100;
    system_fsm_run(&test_ctx);
    TEST_ASSERT_EQUAL_UINT8(STATE_LISTENING, system_fsm_get_state());

    test_ctx.tick_ms = 5100;
    system_fsm_run(&test_ctx);
    TEST_ASSERT_EQUAL_UINT8(STATE_WARNING, system_fsm_get_state());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fsm_post_to_waiting_when_sensors_valid);
    RUN_TEST(test_fsm_post_to_fatal_when_sensor_disconnected);
    RUN_TEST(test_fsm_waiting_to_listening_and_paused_on_button);
    RUN_TEST(test_fsm_listening_to_warning_after_5s_threshold);
    return UNITY_END();
}