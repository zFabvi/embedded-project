#ifndef MOCK_USART_H
#define MOCK_USART_H

#include <stdint.h>
#include <stddef.h>

typedef int HAL_StatusTypeDef;
#define HAL_OK      0
#define HAL_ERROR   1
#define HAL_BUSY    2
#define HAL_TIMEOUT 3

#define GPIO_PIN_13    13
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET   1

static int huart2;
static int GPIOC;
static int mock_btn_pin_state = GPIO_PIN_SET;

static inline HAL_StatusTypeDef HAL_UART_Transmit(void *h, const uint8_t *d, size_t l, uint32_t t) {
    (void)h; (void)d; (void)l; (void)t;
    return HAL_OK;
}

static inline HAL_StatusTypeDef HAL_UART_Receive(void *h, uint8_t *d, size_t l, uint32_t t) {
    (void)h; (void)d; (void)l; (void)t;
    return HAL_TIMEOUT;
}

static inline int HAL_GPIO_ReadPin(int port, int pin) {
    (void)port; (void)pin;
    return mock_btn_pin_state;
}

static inline void NVIC_SystemReset(void) {}

#endif