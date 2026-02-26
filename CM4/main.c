
// This file is a part of MRNIU/FreeRTOS-PortentaH7
// (https://github.com/MRNIU/FreeRTOS-PortentaH7).
//
// main.c for MRNIU/FreeRTOS-PortentaH7.

#include "main.h"
#include "Legacy/stm32_hal_legacy.h"
#include "cmsis_os.h"
#include "fmc.h"
#include "gpio.h"
#include "usart.h"

void MX_FREERTOS_Init(void);

int main(void)
{
  __HAL_RCC_HSEM_CLK_ENABLE();

  // Update the SystemCoreClock variable.
  SystemCoreClockUpdate();
  HAL_Init();

  osKernelInitialize(); /* Call init function for freertos objects (in
                           freertos.c) */
  MX_UART4_Init();
  MX_FREERTOS_Init();
  /* Start scheduler */
  osKernelStart();
}

void Error_Handler(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.Pin = LED_R_Pin;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(LED_R_GPIO_Port, &GPIO_InitStructure);
  HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, 1);
  /* Infinite loop */
  while (1)
  {
    osDelay(500);
    HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
  }
  return;
}
