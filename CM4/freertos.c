
// This file is a part of MRNIU/FreeRTOS-PortentaH7
// (https://github.com/MRNIU/FreeRTOS-PortentaH7).
//
// freertos.c for MRNIU/FreeRTOS-PortentaH7.

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "i2c.h"
#include "main.h"
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
#include <stdint.h>
#include <strings.h>

osThreadId_t cm4_task_handle;
const osThreadAttr_t cm4_task_attributes = {.name = "cm4_task",
                                            .priority =
                                                (osPriority_t)osPriorityNormal,
                                            .stack_size = 128 * 4};
#define mainHAL_MAX_TIMEOUT 0xFFFFFFFFUL
void StartM4DefaultTask(void *argument);
void MX_FREERTOS_Init(void);

void MX_FREERTOS_Init(void) {
  cm4_task_handle = osThreadNew(StartM4DefaultTask, NULL, &cm4_task_attributes);
}

uint16_t strlength(char *msg) {
  uint16_t length = 0;
  while (1) {
    if (msg[length] == 0) {
      break;
    }
    length++;
  }
  return length;
}

void UART_Transmit(const char *msg, size_t len) {
  HAL_StatusTypeDef result =
      HAL_UART_Transmit(&huart4, (uint8_t *)msg, len, mainHAL_MAX_TIMEOUT);
  if (result != HAL_OK) {
    Error_Handler();
  }
}

void UART_TransmitCStr(char *msg) { UART_Transmit(msg, strlength(msg)); }

void PrintAvailableHeap(QTZ_ByteArray *buffer) {
  char *msg = "Available HEAP SIZE: ";
  const int msg_length = strlength(msg);
  UART_Transmit(msg, msg_length);

  size_t free_heap = xPortGetFreeHeapSize();
  if (QTZ_FMTSIZET_OK != QTZ_FmtSizeT(free_heap, buffer)) {
    UART_TransmitCStr("\nERROR: Failed to format free_heap as number!\n");
    return;
  }
  if (QTZ_BYTEARRAYAPPEND_OK != QTZ_ByteArray_Append(buffer, '\n')) {
    UART_TransmitCStr("\nERROR: Failed to append '\\n'! Ending execution...\n");
    return;
  }
  UART_Transmit((char *)buffer->data, buffer->length);
  QTZ_ByteArray_Reset(buffer);
}

void StartM4DefaultTask(void *argument) {
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.Pin = LED_B_Pin;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(LED_B_GPIO_Port, &GPIO_InitStructure);
  HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, 1);

  uint8_t byteBuffer[30] = {0};
  QTZ_ByteArray buffer = {0};
  QTZ_ByteArray_Init(&buffer, byteBuffer, 30);
  PrintAvailableHeap(&buffer);
  QTZ_ByteArray_Reset(&buffer);

  const uint16_t SAMD_21_ADDR = 0x44;
  HAL_StatusTypeDef result;
  int commands[] = {
      'p', 500, 'S', 5000, 'r', 0,
  };
  int commands_quantity = 3;
  while (1) {
    osDelay(750);
    HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);

    for (int i = 0; i < commands_quantity * 2; i++) {
      uint8_t cmd = commands[i];
      int timeout = commands[i + 1];

      UART_TransmitCStr("Trying command: ");
      UART_Transmit((char *)&cmd, 1);
      UART_TransmitCStr(" with timeout: ");
      if (QTZ_FMTSIZET_OK != QTZ_FmtSizeT(timeout, &buffer)) {
        UART_TransmitCStr("\nERROR: Failed to format delay as an integer!\n");
        QTZ_ByteArray_Reset(&buffer);
        continue;
      }
      if (QTZ_BYTEARRAYAPPEND_OK != QTZ_ByteArray_Append(&buffer, '\n')) {
        UART_TransmitCStr("\nERROR: Failed to append '\\n' at end buffer!\n");
        QTZ_ByteArray_Reset(&buffer);
        continue;
      }
      UART_Transmit((char *)buffer.data, buffer.length);
      QTZ_ByteArray_Reset(&buffer);

      result = HAL_I2C_Master_Transmit(&hi2c1, SAMD_21_ADDR, &cmd, 1, timeout);
      if (HAL_OK != result) {
        UART_TransmitCStr("\nERROR: Failed to transmit I2C!\n");
        return;
      }
    }
  }
}
