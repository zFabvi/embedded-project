/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdint.h>
#include <string.h>
#include <sys/_intsup.h>

#include "conversions-api.h"
#include "filters-api.h"
#include "eagletrt-fsm-api.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_POST,
    STATE_WAITING,
    STATE_LISTENING,
    STATE_PAUSED,
    STATE_WARNING,
    STATE_FATAL
} SystemState_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

 uint16_t buffer_adc[2]; /*!< DMA buffer for ADC readings: POT at index 0, NTC at index 1 */

 volatile uint8_t cli_mode = 0; /*!< Flag to manage UART interrupt signaling for CLI mode */

 char cli_buffer[10]; /*!< Buffer to store incoming UART characters for command recognition */ 
 uint8_t cli_index = 0; /*!< Current write index for the CLI buffer */
 uint8_t rx_byte; /*!< Temporary storage for the single byte received via UART interrupt */

 volatile uint8_t active_mode = 0; /*!< Flag to control current operational mode and message output */
 volatile uint8_t active_filter = 0; /*!< Flag to identify the currently active data filter */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_TIM14_Init();
  /* USER CODE BEGIN 2 */

  HAL_ADCEx_Calibration_Start(&hadc1); /*!< Calibrate the ADC before starting to ensure accurate analog readings */

  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)buffer_adc, 2); /*!< Start ADC conversions and transfer data to buffer_adc via DMA (2 channels) */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if(cli_mode == 0) { /*!< STREAMING MODE */

      if(active_mode == 0) {
      char start_stream_message[] = "\r\n[---------------------- STREAMING MODE ----------------------]\r\n\n";
      HAL_UART_Transmit(&huart2, (uint8_t*)start_stream_message,
                        strlen(start_stream_message), 1000);
      HAL_Delay(500);
      active_mode = 1;
      }

      uint16_t raw_pot = buffer_adc[0];
      uint16_t raw_ntc = buffer_adc[1];

      if(active_filter == 1) {
        filter_apply_mvavg(&raw_pot, &raw_ntc);
      } else if (active_filter == 2) {
        filter_apply_noise(&raw_pot, &raw_ntc);
      }

      /*!< Conversions using included 'conversions.c' */
      float pot_v = calc_pot_volts(raw_pot);
      float temp_c = calc_ntc_temp(raw_ntc);
      
      char msg_stream[100]; /*!< Formatting sensor data to prevent memory issues caused by float values */
      format_sensor_data(raw_pot, pot_v, raw_ntc, temp_c, msg_stream);

      HAL_UART_Transmit(&huart2, (uint8_t*)msg_stream, strlen(msg_stream), 1000); /*!< Transmitting message with a 100ms delay */
      HAL_Delay(100);
    }
    else{ /*!< CLI MODE */

      if(active_mode == 0) {
      char start_stream_message[] = 
      "\r\n"
      "[----------------------- CLI MODE -----------------------]\r\n\n"
      "COMMANDS: 'noflt' to remove filters,                        \r\n"
      "          'mvavg' to apply moving average (150 elements),   \r\n"
      "          'noise' to apply random spike noise (75% prob)  \r\n\n";

      HAL_UART_Transmit(&huart2, (uint8_t*)start_stream_message,
                        strlen(start_stream_message), 1000);

      HAL_Delay(500);
      active_mode = 1;
      }

      if (HAL_UART_Receive(&huart2, &rx_byte, 1, 10) == HAL_OK) {
        
        HAL_UART_Transmit(&huart2, &rx_byte, 1, 10);

        if (rx_byte != '\r' && rx_byte != '\n') {
          if (cli_index < 10) {
            cli_buffer[cli_index] = rx_byte;
            cli_index++;
          }
        }
        else if (rx_byte == '\r' || rx_byte == '\n') {

          cli_buffer[cli_index] = '\0';

          if(strcmp(cli_buffer, "noflt") == 0){
            active_filter = 0;
            char noflt[] = "\r\nFILTERS REMOVED (RAW MODE)\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)noflt, strlen(noflt), 100);
          }
          else if (strcmp(cli_buffer, "mvavg") == 0) {
            active_filter = 1;
            filter_reset_mvavg();
            char mvavg[] = "\r\nMOVING AVERAGE FILTER ACTIVATED\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)mvavg, strlen(mvavg), 100);
          }
          else if (strcmp(cli_buffer, "noise") == 0) {
            active_filter = 2;
            char noise[] = "\r\nRANDOM SPIKE NOISE ACTIVATED\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)noise, strlen(noise), 100);
          }
          else if (cli_index > 0) {
            char error[] = "\r\nUnknown command";
            HAL_UART_Transmit(&huart2, (uint8_t*)error, strlen(error), 100);
          }

          cli_index = 0;
          memset(cli_buffer, 0, sizeof(cli_buffer));
        }
      }
    }    
  }
}

  /* USER CODE END 3 */

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/*!
 * \brief EXTI line detection callback.
 * 
 * This function is triggered by the hardware whenever a falling edge 
 * interrupt occurs on a configured GPIO pin. It verifies if the interrupt 
 * source is the User Button (PC13) and toggles the operational state 
 * between Streaming Mode and CLI Mode.
 * 
 * \param GPIO_Pin Specifies the EXTI line that triggered the interrupt.
 */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == GPIO_PIN_13)
  {
    cli_mode = !cli_mode;
    active_mode = !active_mode;
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
