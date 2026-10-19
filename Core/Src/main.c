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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <string.h>
#include <sys/_intsup.h>
#include "../Inc/recruiting/conversions.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

 uint16_t buffer_adc[2]; // buffer_adc is used to r/w POT (in 0) & NTC (in 1)

 volatile uint8_t cli_mode = 0; // volatile variable to manage the interrupt signal

 char cli_buffer[10]; // characters sequence for command recognition 
 uint8_t cli_index = 0; // index used to scroll positions while reading command
 uint8_t rx_byte; // used to move a value in the buffer 

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

  /* USER CODE BEGIN 2 */


  HAL_ADCEx_Calibration_Start(&hadc1); // ADC calibration should give better results

  // Enabling ADC and storing data in buffer_adc (2 conversions)
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)buffer_adc, 2);



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if(cli_mode == 0) { // STREAMING

      // Every cycle the buffer is gonna extract the POT & NTC values 
      uint16_t raw_pot = buffer_adc[0];
      uint16_t raw_ntc = buffer_adc[1];

      // Conversions using included 'conversions.c'
      float pot_v = calc_pot_volts(raw_pot);
      float temp_c = calc_ntc_temp(raw_ntc);

      // Formatting sensor data to prevent memory issues caused by float values
      char msg_stream[100];
      format_sensor_data(raw_pot, pot_v, raw_ntc, temp_c, msg_stream);

      // Transmitting message with a 100ms delay
      HAL_UART_Transmit(&huart2, (uint8_t*)msg_stream, strlen(msg_stream), 1000);
      HAL_Delay(100);

    }
    else{ // CLI

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

          if( strcmp(cli_buffer, "noflt") == 0){
            char command_noflt[] = "\r\nnoflt exec\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)command_noflt, strlen(command_noflt), 100);
          }
          else if (strcmp(cli_buffer, "mvavg") == 0) {
            char command_mvavg[] = "\r\nmvavg exec\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)command_mvavg, strlen(command_mvavg), 100);
          }
          else if (strcmp(cli_buffer, "noise") == 0) {
            char command_rndns[] = "\r\nrndns exec\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)command_rndns, strlen(command_rndns), 100);
          }
          else if (cli_index > 0) {
            char command_error[] = "\r\nUnknown command";
            HAL_UART_Transmit(&huart2, (uint8_t*)command_error, strlen(command_error), 100);
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

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
  // currently the User Button pin is PC13
  if(GPIO_Pin == GPIO_PIN_13)
  {
    cli_mode = !cli_mode;
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
