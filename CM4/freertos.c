
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
    .stack_size = 256 * 8,
};
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

  int commands_quantity = 4;
  // QTZ_Command commands[] = {};
  QTZ_Command commands[] = {
      // Ping the module, find out if it's ok!
      {
          .command_id = QTZ_MILO_Ping,
          .response_size = 1,
          .send_timeout = 1000,
          .recv_timeout = 7000,
      },
      // Take a picture
      {
          .command_id = QTZ_MILO_Snapshot,
          .response_size = 5,
          .send_timeout = 1000,
          .recv_timeout = 7000,
      },
      // Retrieve image statistics!
      {
          .command_id = QTZ_MILO_ImageStatistics,
          .response_size = 5,
          .send_timeout = 1000,
          .recv_timeout = 7000,
      },
      // Reset the camera to save the picture!
      {
          .command_id = QTZ_MILO_ResetCam,
          .response_size = 5,
          .send_timeout = 1000,
          .recv_timeout = 7000,
      },
  };

  while (1) {
    osDelay(750);
    QTZ_ByteArray_Reset(&buffer);
    HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);

    for (int i = 0; i < commands_quantity; i++) {
      QTZ_Command cmd = commands[i];
      QTZ_Debug_Log("MILO: Command: %c - %d:%d - Expects: %d\n", cmd.command_id,
                    cmd.send_timeout, cmd.recv_timeout, cmd.response_size);
      {
        QTZ_SENDRS485_Result result =
            QTZ_RS485_SendCStr((char *)&cmd.command_id, 1, cmd.send_timeout);
        if (QTZ_SENDRS485_OK != result) {
          QTZ_Debug_Error("MILO: Failed to send command! Error: %d\n", result);
          Error_Handler();
        }
      }

      QTZ_Debug_Log("MILO: Waiting for response...\n");
      {
        QTZ_RECEIVERS485_Result result =
            QTZ_RS485_Receive(&buffer, cmd.response_size, cmd.recv_timeout);
        if (QTZ_RECEIVERS485_OK != result) {
          QTZ_Debug_Error(
              "MILO: Failed to receive response from command! Error: %d\n",
              result);
          Error_Handler();
        }
      }

      uint8_t response_status = buffer.data[0];
      QTZ_Debug_Log("MILO: Received response: D:%d C:%c - '%.*s'\n",
                    response_status, response_status, buffer.length,
                    buffer.data);

      if (QTZ_MILO_ImageStatistics == cmd.command_id) {
        if (buffer.length == 0) {
          QTZ_Debug_Error("No data written to buffer!\n");
          Error_Handler();
        }
        QTZ_Debug_Log("Statistics: %.*s\n", buffer.length - 1, buffer.data + 1);
      }
    }
    osDelay(5000);
  }
}
