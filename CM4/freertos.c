
// This file is a part of MRNIU/FreeRTOS-PortentaH7
// (https://github.com/MRNIU/FreeRTOS-PortentaH7).
//
// freertos.c for MRNIU/FreeRTOS-PortentaH7.

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"
#include "stm32h7xx_hal_uart.h"
#include "stm32h7xx_hal_uart_ex.h"
#include "stm32h7xx_it.h"
#include "task.h"
#include "usart.h"
#include <stdint.h>

osThreadId_t cm4_task_handle;
const osThreadAttr_t cm4_task_attributes = {.name = "cm4_task",
                                            .priority =
                                                (osPriority_t)osPriorityNormal,
                                            .stack_size = 128 * 4};
#define mainHAL_MAX_TIMEOUT 0xFFFFFFFFUL
void StartM4DefaultTask(void *argument);
void MX_FREERTOS_Init(void);

/* Handle to the UART used to output strings. */
static UART_HandleTypeDef xUARTHandle = {0};

void MX_FREERTOS_Init(void) {
  // setup(); //<-this line
  cm4_task_handle = osThreadNew(StartM4DefaultTask, NULL, &cm4_task_attributes);
}

void StartM4DefaultTask(void *argument) {
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.Pin = LED_B_Pin;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(LED_B_GPIO_Port, &GPIO_InitStructure);
  HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, 1);
  char *msg = "Hola Mundo!";
  /* Infinite loop */
  while (1) {
    osDelay(1000);
    HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
    HAL_StatusTypeDef result = HAL_UART_Transmit(
        &huart4, (uint8_t *)msg, sizeof(msg), mainHAL_MAX_TIMEOUT);
    if (result != HAL_OK) {
      Error_Handler();
    }
  }
  // loop(); //<-this line
}
