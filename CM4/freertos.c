
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
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_B_GPIO_Port, &GPIO_InitStructure);
  HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);

  PrintAvailableHeap();

  QTZ_ByteArray_Create(buffer, data, 64);

  // clang-format off
  uint32_t commands[] = {
      QTZ_MILO_Ping, 1, 7000, // Ping the module, find out if it's ok!
      // QTZ_MILO_Snapshot, 1, 4000, // Take a picture
      // QTZ_MILO_ImageStatistics, 7, 1000, // Retrieve image statistics!
      // QTZ_MILO_ResetCam, 1, 3000, // Reset the camera to save the picture!
  };
  // clang-format on
  int commands_quantity = 1;

  // while (1) {
  //   osDelay(750);
  //   HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
  //
  // {
  // 	QTZ_Debug_Log("Sending msg...\n");
  // 	QTZ_SENDRS485_Result res = QTZ_RS485_SendCStr("ACT", 3, 3000);
  // 	if (res != QTZ_SENDRS485_OK) {
  // 		QTZ_Debug_Log("Result is: %d\n", res);
  // 		continue;
  // 	}
  // }
  //
  // QTZ_Debug_Log("Waiting for msg...\n");
  //   QTZ_RECEIVERS485_Result result = QTZ_RS485_Receive(&buffer, 1, 3000);
  // // uint8_t * new_data = buffer.data << 8;
  //   QTZ_Debug_Log("Received result: %d - %.*s\n", result, buffer.length,
  //                 buffer.data);
  // QTZ_ByteArray_Reset(&buffer);
  // }

  while (1) {
    osDelay(750);
    QTZ_ByteArray_Reset(&buffer);
    HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);

    // for (int i = 0; i < commands_quantity * 3; i += 3) {
    //   QTZ_MILO_COMMAND cmd = commands[i];
    //   uint16_t response_size = commands[i + 1];
    //   uint32_t timeout = commands[i + 2];

    uint32_t timeout = 7000;
    uint16_t response_size = 2;

    {
      QTZ_Debug_Log("Sending msg...\n");
      QTZ_SENDRS485_Result res = QTZ_RS485_SendCStr("p", 1, timeout);
      if (res != QTZ_SENDRS485_OK) {
        QTZ_Debug_Log("Result is: %d\n", res);
        continue;
      }
    }

    QTZ_Debug_Log("Waiting for msg...\n");
    QTZ_RECEIVERS485_Result result =
        QTZ_RS485_Receive(&buffer, response_size, timeout);
    QTZ_Debug_Log("Received result: %d - '%.*s'\n", result, buffer.length,
                  buffer.data);
    QTZ_ByteArray_Reset(&buffer);

    // if (QTZ_MILO_ImageStatistics == cmd) {
    //   if (buffer.length == 0) {
    //     QTZ_Debug_Error("No data written to buffer!\n");
    //     Error_Handler();
    //   }
    //   QTZ_Debug_Log("Statistics: %.*s\n", buffer.length - 1, buffer.data +
    //   1);
    // }

    osDelay(timeout);
    // }
  }
}
