
// This file is a part of MRNIU/FreeRTOS-PortentaH7
// (https://github.com/MRNIU/FreeRTOS-PortentaH7).
//
// freertos.c for MRNIU/FreeRTOS-PortentaH7.

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "debug.h"
#include "i2c.h"
#include "main.h"
#include "milo.h"
#include "portable.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"
#include "stm32h7xx_hal_i2c.h"
#include "stm32h7xx_hal_uart.h"
#include "stm32h7xx_hal_uart_ex.h"
#include "stm32h7xx_it.h"
#include "task.h"
#include "usart.h"
#include <common.h>
#include <math.h>
#include <rs485.h>
#include <stdint.h>
#include <strings.h>

osThreadId_t samd_thread;
const osThreadAttr_t samd_thread_attributes = {
    .name = "cm4_task",
    .priority = (osPriority_t)osPriorityNormal,
    .stack_size = 128 * 4};
#define mainHAL_MAX_TIMEOUT 0xFFFFFFFFUL
void SAMD_Routine(void *argument);
void MX_FREERTOS_Init(void);

void MX_FREERTOS_Init(void) {
  samd_thread = osThreadNew(SAMD_Routine, NULL, &samd_thread_attributes);
}

void PrintAvailableHeap() {
  size_t free_heap = xPortGetFreeHeapSize();
  QTZ_Debug_Log("Available HEAP SIZE: %d\n", free_heap);
}

void SAMD_Routine(void *argument) {
  // NOTE: Initialize the LED pin on blue.
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.Pin = LED_B_Pin;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(LED_B_GPIO_Port, &GPIO_InitStructure);
  HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);

  QTZ_RS485_InitGPIO();
  PrintAvailableHeap();

  QTZ_ByteArray_Create(buffer, data, 1024);

  uint32_t commands[] = {
      QTZ_MILO_Ping,     1000, QTZ_MILO_Snapshot,        4000,
      QTZ_MILO_ResetCam, 3000, QTZ_MILO_ImageStatistics, 1000,
  };
  int commands_quantity = 3;
  while (1) {
    osDelay(750);
    QTZ_ByteArray_Reset(&buffer);
    HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);

    for (int i = 0; i < commands_quantity * 2; i += 2) {
      QTZ_MILO_COMMAND cmd = commands[i];
      uint32_t timeout = commands[i + 1];

      QTZ_MILO_RESULT result = QTZ_MILO_SendCommand(cmd, &buffer, timeout);
      if (QTZ_MILO_OK != result) {
        QTZ_Debug_Error(
            "Encountered an error when sending command to MILO! Error: %d\n",
            result);
        Error_Handler();
      }

      if (QTZ_MILO_ImageStatistics == cmd) {
        if (buffer.length == 0) {
          QTZ_Debug_Error("No data written to buffer!\n");
          Error_Handler();
        }
        QTZ_Debug_Log("Statistics: %.*s\n", buffer.length - 1, buffer.data + 1);
      }

      osDelay(timeout);
    }
  }
}
