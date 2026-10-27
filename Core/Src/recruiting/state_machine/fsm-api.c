#include "fsm-api.h"
#include "fsm.h"

#include <stdio.h>
#include <string.h>

#include "tim.h"
#include "usart.h"
#include "conversions-api.h"
#include "filters-api.h"

/*! \brief Static instance of the FSM handler. */
static struct FSMHandler fsm_handler;

/*! \brief Internal timestamps. */
static uint32_t state_entry_tick = 0;
static uint32_t last_tx_tick = 0;
static uint32_t threshold_start_tick = 0;
static uint8_t  threshold_active = 0;
static uint32_t btn_hold_start_tick = 0;
static uint8_t  btn_holding = 0;

/*!
 * \brief Sets the PWM duty cycle on TIM1 channels.
 * \param board_led_duty Duty cycle for PA5 (TIM1_CH1) in range 0..1000.
 * \param ext_led_duty   Duty cycle for PA9 (TIM1_CH2) in range 0..1000.
 */
static void update_leds_pwm(uint16_t board_led_duty, uint16_t ext_led_duty) {
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, board_led_duty);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, ext_led_duty);
}

/*!
 * \brief Computes the external LED breathing PWM value (2000ms period, 0-100% duty cycle).
 * \param tick_ms Current millisecond counter.
 * \return uint16_t Compare value in range 0..1000.
 */
static uint16_t calc_breathing_duty(uint32_t tick_ms) {
    uint32_t phase = tick_ms % 2000U;
    if (phase < 1000U) {
        return (uint16_t)phase; /*!< From 0 to 999 ms */
    }
    return (uint16_t)(2000U - phase); /*!< From 1000 to 1999 ms */
}

/*!
 * \brief Transmits a string over UART2 and redirects to STATE_FATAL on HAL error.
 * \param ctx Pointer to the SystemContext structure.
 * \param msg Null-terminated string to transmit.
 * \return uint8_t 1 if transmission succeeded, 0 on error.
 */
static uint8_t uart_send_checked(struct SystemContext *ctx, const char *msg) {
    if (HAL_UART_Transmit(&huart2, (const uint8_t *)msg, strlen(msg), 100) != HAL_OK) {
        ctx->error_msg = "HAL Error: UART Transmit failed";
        system_fsm_transition_to(STATE_FATAL);
        return 0;
    }
    return 1;
}

/*!
 * \brief Handles non-blocking CLI character reception and command parsing in Waiting and Paused states.
 * \param ctx Pointer to the SystemContext structure.
 */
static void process_cli(struct SystemContext *ctx) {
    uint8_t rx_byte = 0;
    HAL_StatusTypeDef status = HAL_UART_Receive(&huart2, &rx_byte, 1, 0);

    if (status == HAL_OK) {
        if (HAL_UART_Transmit(&huart2, &rx_byte, 1, 10) != HAL_OK) {
            ctx->error_msg = "HAL Error: UART Echo Transmit failed";
            system_fsm_transition_to(STATE_FATAL);
            return;
        }

        if (rx_byte != '\r' && rx_byte != '\n') {
            if (ctx->cli_index < (sizeof(ctx->cli_buffer) - 1U)) {
                ctx->cli_buffer[ctx->cli_index] = (char)rx_byte;
                ctx->cli_index++;
            }
        } else {
            ctx->cli_buffer[ctx->cli_index] = '\0';

            if (strcmp(ctx->cli_buffer, "noflt") == 0) {
                ctx->active_filter = 0;
                uart_send_checked(ctx, "\r\nFILTERS REMOVED (RAW MODE)\r\n");
            } else if (strcmp(ctx->cli_buffer, "mvavg") == 0) {
                ctx->active_filter = 1;
                filter_reset_mvavg();
                uart_send_checked(ctx, "\r\nMOVING AVERAGE FILTER ACTIVATED\r\n");
            } else if (strcmp(ctx->cli_buffer, "noise") == 0) {
                ctx->active_filter = 2;
                uart_send_checked(ctx, "\r\nRANDOM SPIKE NOISE ACTIVATED\r\n");
            } else if (strcmp(ctx->cli_buffer, "run") == 0) {
                uart_send_checked(ctx, "\r\nSTARTING LISTENING MODE...\r\n");
                system_fsm_transition_to(STATE_LISTENING);
            } else if (ctx->cli_index > 0) {
                uart_send_checked(ctx, "\r\nUnknown command\r\n");
            }

            ctx->cli_index = 0;
            memset(ctx->cli_buffer, 0, sizeof(ctx->cli_buffer));
        }
    } else if (status != HAL_TIMEOUT && status != HAL_BUSY) {
        ctx->error_msg = "HAL Error: UART Receive failed";
        system_fsm_transition_to(STATE_FATAL);
    }
}

void transition_to_cli_state(void *data) {
    struct SystemContext *ctx = (struct SystemContext *)data;
    if (ctx == NULL) {
        return;
    }
    ctx->btn_pressed = 0;
    ctx->cli_index = 0;
    memset(ctx->cli_buffer, 0, sizeof(ctx->cli_buffer));

    const char banner[] =
        "\r\n[----------------------- CLI MODE -----------------------]\r\n"
        "COMMANDS: 'noflt' to remove filters,\r\n"
        "          'mvavg' to apply moving average (150 elements),\r\n"
        "          'noise' to apply random spike noise (75% prob),\r\n"
        "          'run'   to enter Listening mode\r\n\n";
    HAL_UART_Transmit(&huart2, (const uint8_t *)banner, strlen(banner), 200);
}

void transition_to_listening(void *data) {
    struct SystemContext *ctx = (struct SystemContext *)data;
    if (ctx == NULL) {
        return;
    }
    ctx->btn_pressed = 0;
    last_tx_tick = ctx->tick_ms;
    threshold_active = 0;

    const char banner[] = "\r\n[---------------------- LISTENING MODE ----------------------]\r\n\n";
    HAL_UART_Transmit(&huart2, (const uint8_t *)banner, strlen(banner), 200);
}

void transition_to_warning(void *data) {
    struct SystemContext *ctx = (struct SystemContext *)data;
    if (ctx == NULL) {
        return;
    }
    ctx->btn_pressed = 0;
    state_entry_tick = ctx->tick_ms;
    last_tx_tick = ctx->tick_ms;
    btn_holding = 0;
    /* Turn off both LEDs in Warning state */
    update_leds_pwm(0, 0);
}

void transition_to_fatal(void *data) {
    struct SystemContext *ctx = (struct SystemContext *)data;
    if (ctx == NULL) {
        return;
    }
    ctx->btn_pressed = 0;
    last_tx_tick = ctx->tick_ms;
    /* Turn off external LED */
    update_leds_pwm(0, 0);
}

/*! \brief Allowed transitions from STATE_POST. */
static const struct Transition post_transitions[] = {
    { .to = STATE_WAITING, .function = transition_to_cli_state },
    { .to = STATE_FATAL,   .function = transition_to_fatal }
};

/*! \brief Allowed transitions from STATE_WAITING. */
static const struct Transition waiting_transitions[] = {
    { .to = STATE_LISTENING, .function = transition_to_listening },
    { .to = STATE_FATAL,     .function = transition_to_fatal }
};

/*! \brief Allowed transitions from STATE_LISTENING. */
static const struct Transition listening_transitions[] = {
    { .to = STATE_WARNING, .function = transition_to_warning },
    { .to = STATE_PAUSED,  .function = transition_to_cli_state },
    { .to = STATE_FATAL,   .function = transition_to_fatal }
};

/*! \brief Allowed transitions from STATE_PAUSED. */
static const struct Transition paused_transitions[] = {
    { .to = STATE_LISTENING, .function = transition_to_listening },
    { .to = STATE_FATAL,     .function = transition_to_fatal }
};

/*! \brief Allowed transitions from STATE_WARNING. */
static const struct Transition warning_transitions[] = {
    { .to = STATE_WAITING, .function = transition_to_cli_state },
    { .to = STATE_FATAL,   .function = transition_to_fatal }
};


/*!
 * \brief Master state table defining all states, routines, defaults, and transition links.
 */
static const struct State system_states[STATE_COUNT] = {
    [STATE_POST] = {
        .id = STATE_POST,
        .function = state_post_fn,
        .next_default = STATE_POST,
        .transitions = post_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(post_transitions)
    },
    [STATE_WAITING] = {
        .id = STATE_WAITING,
        .function = state_waiting_fn,
        .next_default = STATE_WAITING,
        .transitions = waiting_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(waiting_transitions)
    },
    [STATE_LISTENING] = {
        .id = STATE_LISTENING,
        .function = state_listening_fn,
        .next_default = STATE_LISTENING,
        .transitions = listening_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(listening_transitions)
    },
    [STATE_PAUSED] = {
        .id = STATE_PAUSED,
        .function = state_paused_fn,
        .next_default = STATE_PAUSED,
        .transitions = paused_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(paused_transitions)
    },
    [STATE_WARNING] = {
        .id = STATE_WARNING,
        .function = state_warning_fn,
        .next_default = STATE_WARNING,
        .transitions = warning_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(warning_transitions)
    },
    [STATE_FATAL] = {
        .id = STATE_FATAL,
        .function = state_fatal_fn,
        .next_default = STATE_FATAL,
        .transitions = NULL,
        .num_transitions = 0
    }
};

void state_post_fn(void *data) {
    struct SystemContext *ctx = (struct SystemContext *)data;
    if (ctx == NULL || ctx->adc_buffer == NULL) {
        return;
    }

    uint16_t raw_pot = ctx->adc_buffer[0];
    uint16_t raw_ntc = ctx->adc_buffer[1];

    /*!< Check if potentiometer or NTC thermistor is disconnected (0) or shorted (>= 4095)
     PS. This same check was implemented in filters-api.c to avoid it most of the times */
    if (raw_pot == 0 || raw_pot >= 4095) {
        ctx->error_msg = "POST failed: Potentiometer out of range (disconnected or shorted)";
        system_fsm_transition_to(STATE_FATAL);
    } else if (raw_ntc == 0 || raw_ntc >= 4095) {
        ctx->error_msg = "POST failed: NTC thermistor out of range (disconnected or shorted)";
        system_fsm_transition_to(STATE_FATAL);
    } else {
        system_fsm_transition_to(STATE_WAITING);
    }
}

void state_waiting_fn(void *data) {
    struct SystemContext *ctx = (struct SystemContext *)data;
    if (ctx == NULL) {
        return;
    }

    /*!< Board LED: 500ms period, 15% duty cycle (75ms ON, 425ms OFF) */
    uint16_t board_duty = ((ctx->tick_ms % 500U) < 75U) ? FSM_PWM_MAX_DUTY : 0U;
    /*!< External LED: breathing effect (2s period, 0-100%) */
    uint16_t ext_duty = calc_breathing_duty(ctx->tick_ms);
    update_leds_pwm(board_duty, ext_duty);

    if (ctx->btn_pressed) {
        ctx->btn_pressed = 0;
        system_fsm_transition_to(STATE_LISTENING);
        return;
    }

    process_cli(ctx);
}

void state_listening_fn(void *data) {
    struct SystemContext *ctx = (struct SystemContext *)data;
    if (ctx == NULL || ctx->adc_buffer == NULL) {
        return;
    }

    if (ctx->btn_pressed) {
        ctx->btn_pressed = 0;
        system_fsm_transition_to(STATE_PAUSED);
        return;
    }

    uint16_t raw_pot = ctx->adc_buffer[0];
    uint16_t raw_ntc = ctx->adc_buffer[1];

    if (raw_pot == 0 || raw_pot >= 4095 || raw_ntc == 0 || raw_ntc >= 4095) {
        ctx->error_msg = "Sensor fault detected during Listening state";
        system_fsm_transition_to(STATE_FATAL);
        return;
    }

    if (ctx->active_filter == 1) {
        filter_apply_mvavg(&raw_pot, &raw_ntc);
    } else if (ctx->active_filter == 2) {
        filter_apply_noise(&raw_pot, &raw_ntc);
    }

    /*!< (thermistor - potentiometer) difference + external LED PWM */
    int32_t diff = (int32_t)raw_ntc - (int32_t)raw_pot;
    uint16_t ext_duty = 0U;

    if (diff <= 0) {
        ext_duty = 0U;
    } else if ((uint32_t)diff >= FSM_DIFF_THRESHOLD) {
        ext_duty = FSM_PWM_MAX_DUTY;
    } else {
        ext_duty = (uint16_t)(((uint32_t)diff * FSM_PWM_MAX_DUTY) / FSM_DIFF_THRESHOLD);
    }

    /*!< Board LED: 200ms period, 80% duty cycle (160ms ON, 40ms OFF) */
    uint16_t board_duty = ((ctx->tick_ms % 200U) < 160U) ? FSM_PWM_MAX_DUTY : 0U;
    update_leds_pwm(board_duty, ext_duty);

    /*!< Check if (NTC - POT) > threshold for 5 seconds */
    if (diff > (int32_t)FSM_DIFF_THRESHOLD) {
        if (!threshold_active) {
            threshold_active = 1;
            threshold_start_tick = ctx->tick_ms;
        } else if ((ctx->tick_ms - threshold_start_tick) >= FSM_LISTENING_WARN_TIME_MS) {
            system_fsm_transition_to(STATE_WARNING);
            return;
        }
    } else {
        threshold_active = 0;
    }

    /*!< Transmit sensor data every 100ms */
    if ((ctx->tick_ms - last_tx_tick) >= FSM_LISTENING_TX_PERIOD_MS) {
        last_tx_tick = ctx->tick_ms;

        float pot_v = calc_pot_volts(raw_pot);
        float temp_c = calc_ntc_temp(raw_ntc);

        char msg_stream[100];
        format_sensor_data(raw_pot, pot_v, raw_ntc, temp_c, msg_stream);
        uart_send_checked(ctx, msg_stream);
    }
}

void state_paused_fn(void *data) {
    struct SystemContext *ctx = (struct SystemContext *)data;
    if (ctx == NULL) {
        return;
    }

    /*!< Board LED: 2000ms period, 50% duty cycle (1000ms ON, 1000ms OFF) */
    uint16_t board_duty = ((ctx->tick_ms % 2000U) < 1000U) ? FSM_PWM_MAX_DUTY : 0U;
    /*!< External LED: same breathing effect as in Waiting state */
    uint16_t ext_duty = calc_breathing_duty(ctx->tick_ms);
    update_leds_pwm(board_duty, ext_duty);

    if (ctx->btn_pressed) {
        ctx->btn_pressed = 0;
        system_fsm_transition_to(STATE_LISTENING);
        return;
    }

    process_cli(ctx);
}

void state_warning_fn(void *data) {
    struct SystemContext *ctx = (struct SystemContext *)data;
    if (ctx == NULL || ctx->adc_buffer == NULL) {
        return;
    }

    update_leds_pwm(0, 0);

    /*!< Check for hardware fault */
    uint16_t raw_pot = ctx->adc_buffer[0];
    uint16_t raw_ntc = ctx->adc_buffer[1];
    if (raw_pot == 0 || raw_pot >= 4095 || raw_ntc == 0 || raw_ntc >= 4095) {
        ctx->error_msg = "Sensor fault detected during Warning state";
        system_fsm_transition_to(STATE_FATAL);
        return;
    }

    /*!< Check if Warning state persists for more than 10 seconds (then go to Fatal) */
    if ((ctx->tick_ms - state_entry_tick) >= FSM_WARNING_FATAL_TIME_MS) {
        ctx->error_msg = "Warning state persisted for more than 10 seconds";
        system_fsm_transition_to(STATE_FATAL);
        return;
    }

    /*!< Check if User Button is held for more than 2 seconds */
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) {
        if (!btn_holding) {
            btn_holding = 1;
            btn_hold_start_tick = ctx->tick_ms;
        } else if ((ctx->tick_ms - btn_hold_start_tick) >= FSM_WARNING_BTN_HOLD_MS) {
            ctx->btn_pressed = 0;
            system_fsm_transition_to(STATE_WAITING);
            return;
        }
    } else {
        btn_holding = 0;
    }

    if ((ctx->tick_ms - last_tx_tick) >= FSM_WARNING_TX_PERIOD_MS) {
        last_tx_tick = ctx->tick_ms;
        uart_send_checked(ctx, "WARNING\r\n");
    }
}

void state_fatal_fn(void *data) {
    struct SystemContext *ctx = (struct SystemContext *)data;
    if (ctx == NULL) {
        return;
    }

    /*!< Board LED: 40ms period, 50% duty cycle (20ms ON, 20ms OFF). External LED: OFF */
    uint16_t board_duty = ((ctx->tick_ms % 40U) < 20U) ? FSM_PWM_MAX_DUTY : 0U;
    update_leds_pwm(board_duty, 0U);

    if (ctx->btn_pressed) {
        ctx->btn_pressed = 0;
        NVIC_SystemReset();
    }

    if ((ctx->tick_ms - last_tx_tick) >= FSM_FATAL_TX_PERIOD_MS) {
        last_tx_tick = ctx->tick_ms;
        char err_buf[128];
        snprintf(err_buf, sizeof(err_buf), "ERROR: %s\r\n",
                 (ctx->error_msg != NULL) ? ctx->error_msg : "Unspecified fatal error");
        HAL_UART_Transmit(&huart2, (const uint8_t *)err_buf, strlen(err_buf), 100);
    }
}

enum FSMReturnCode system_fsm_init(void) {
    return fsm_api_init(&fsm_handler, system_states, STATE_COUNT, STATE_POST);
}

enum FSMReturnCode system_fsm_run(void *data) {
    return fsm_api_run_state(&fsm_handler, data);
}

enum FSMReturnCode system_fsm_transition_to(enum SystemStateID next_state) {
    return fsm_api_transition_to(&fsm_handler, (uint8_t)next_state);
}

uint8_t system_fsm_get_state(void) {
    return fsm_api_get_state(&fsm_handler);
}