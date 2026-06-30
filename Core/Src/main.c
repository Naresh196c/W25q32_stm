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
#include "w25q32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
uint32_t flashID;
/**
  * @brief  The application entry point.
  * @retval int
  */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

typedef struct
{
    uint32_t Magic;
    uint32_t BatteryID[8];
} FlashBattery_t;

#define BATTERY_DATA_MAGIC  0x42415454UL

FlashBattery_t flashData;

static void BatteryData_SetEmpty(FlashBattery_t *data)
{
    data->Magic = BATTERY_DATA_MAGIC;

    for (uint8_t i = 0; i < 8U; i++)
    {
        data->BatteryID[i] = 0xFFFFFFFFUL;
    }
}

static uint16_t UART_ReadLine(char *buffer, uint16_t bufferSize)
{
    uint16_t index = 0;
    char ch;

    if (bufferSize == 0U)
    {
        return 0U;
    }

    while (1)
    {
        HAL_UART_Receive(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);

        /* Ignore the LF that follows CR in a PuTTY CR+LF line ending. */
        if ((index == 0U) && ((ch == '\r') || (ch == '\n')))
        {
            continue;
        }

        if ((ch == '\r') || (ch == '\n'))
        {
            buffer[index] = '\0';
            printf("\r\n");
            return index;
        }

        if ((ch == '\b') || (ch == 0x7F))
        {
            if (index > 0U)
            {
                index--;
                printf("\b \b");
            }
            continue;
        }

        if (index < (bufferSize - 1U))
        {
            buffer[index++] = ch;
            HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
        }
    }
}

int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  char input[32];
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
  MX_SPI2_Init();
  MX_USART1_UART_Init();

  /* Send prompts immediately instead of waiting for a newline. */
  setvbuf(stdout, NULL, _IONBF, 0);

  W25_Init();

  uint8_t status;


  printf("\r\n");
  printf("=================================\r\n");
  printf(" W25Q32 TEST\r\n");
  printf("=================================\r\n");

  /* Step 1 : Read JEDEC ID */
  flashID = W25_ReadID();
  printf("JEDEC ID = 0x%06lX\r\n", flashID);

  /* Step 2 : Read Status Register */
  status = W25_ReadStatus();
  printf("Status Before = 0x%02X\r\n", status);




  printf("\r\nReading Previous Data...\r\n");

  W25_ReadData(0,
               (uint8_t *)&flashData,
               sizeof(flashData));

  if (flashData.Magic != BATTERY_DATA_MAGIC)
  {
      printf("No valid battery database found; using empty slots.\r\n");
      BatteryData_SetEmpty(&flashData);
  }

  printf("\r\nStored Battery IDs\r\n");

  for(int i = 0; i < 8; i++)
  {
      printf("Battery %d : 0x%08lX\r\n",
             i + 1,
             flashData.BatteryID[i]);
  }

  while (1)
  {
      char choice;

      printf("\r\n");
      printf("========== MENU ==========\r\n");
      printf("1. Read Battery IDs\r\n");
      printf("2. Write Battery ID\r\n");
      printf("Enter Choice : ");

      UART_ReadLine(input, sizeof(input));
      choice = input[0];

      switch(choice)
      {
          case '1':
          {
        	  W25_ReadData(0,
        	               (uint8_t *)&flashData,
        	               sizeof(flashData));

              if (flashData.Magic != BATTERY_DATA_MAGIC)
              {
                  BatteryData_SetEmpty(&flashData);
              }

        	  printf("\r\nStored Battery IDs\r\n");

        	  for(int i = 0; i < 8; i++)
        	  {
        	      printf("Battery %d : 0x%08lX\r\n",
        	             i + 1,
        	             flashData.BatteryID[i]);
        	  }
        	  break;
          }

          case '2':
          {
              char *end;
              unsigned long slot;
              unsigned long batteryId;
              FlashBattery_t verifyData;

              printf("\r\nSelect Battery Slot (1-8): ");
              UART_ReadLine(input, sizeof(input));
              slot = strtoul(input, &end, 10);

              if ((*input == '\0') || (*end != '\0') ||
                  (slot < 1UL) || (slot > 8UL))
              {
                  printf("Invalid slot. Enter a number from 1 to 8.\r\n");
                  break;
              }

              printf("Enter Battery ID (decimal or 0x hexadecimal): ");
              UART_ReadLine(input, sizeof(input));
              batteryId = strtoul(input, &end, 0);

              if ((*input == '\0') || (*end != '\0') ||
                  (batteryId > 0xFFFFFFFFUL))
              {
                  printf("Invalid Battery ID.\r\n");
                  break;
              }

              flashData.BatteryID[slot - 1UL] = (uint32_t)batteryId;
              flashData.Magic = BATTERY_DATA_MAGIC;

              printf("Erasing Flash...\r\n");
              W25_SectorErase(0);

              printf("Writing...\r\n");
              W25_Write(0,
                        (uint8_t *)&flashData,
                        sizeof(flashData));

              W25_ReadData(0,
                           (uint8_t *)&verifyData,
                           sizeof(verifyData));

              if (memcmp(&flashData, &verifyData, sizeof(flashData)) == 0)
              {
                  printf("Saved Battery %lu : 0x%08lX\r\n",
                         slot, batteryId);
              }
              else
              {
                  printf("ERROR: Flash verification failed.\r\n");
              }
              break;
          }
      }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
