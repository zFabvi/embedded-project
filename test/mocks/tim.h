#ifndef MOCK_TIM_H
#define MOCK_TIM_H

#include <stdint.h>

#define TIM_CHANNEL_1 0
#define TIM_CHANNEL_2 1

static int htim1;

#define __HAL_TIM_SET_COMPARE(htim, ch, duty) ((void)0)

#endif