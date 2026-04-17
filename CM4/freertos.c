
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

void StartM4DefaultTask(void *argument) {
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.Pin = LED_B_Pin;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(LED_B_GPIO_Port, &GPIO_InitStructure);
  HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, 1);

  char *msg = "Available HEAP SIZE: ";
  const int msg_length = strlength(msg);
  UART_Transmit(msg, msg_length);

  size_t free_heap = xPortGetFreeHeapSize();
  uint8_t buffer[30] = {0};
  QTZ_ByteArray array = {0};
  QTZ_ByteArray_Init(&array, buffer, 30);
  if (QTZ_FMTSIZET_OK != QTZ_FmtSizeT(free_heap, &array)) {
    Error_Handler();
  }
  size_t digits = QTZ_DigitQuantity(free_heap);
  if (digits < 30) {
    buffer[digits] = '\n';
  }
  UART_Transmit((char *)array.data, array.length);
  QTZ_ByteArray_Reset(&array);

  const uint16_t SAMD_21_ADDR = 0x44;
  HAL_StatusTypeDef result;
  while (1) {
    osDelay(750);
    HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);

    result = HAL_UART_Receive(&huart4, array.data, 1, 100);
    if (result == HAL_TIMEOUT) {
      continue;
    }

    if (result != HAL_OK) {
      char *msg = "Failed to receive from UART!\n";
      UART_Transmit(msg, strlength(msg));
    }

    char received_byte = array.data[0];
    switch (received_byte) {
    case 'r': {
      uint8_t samd_msg = '9';
      result = HAL_I2C_Master_Transmit(&hi2c1, SAMD_21_ADDR, &samd_msg, 1, 500);
      if (result != HAL_OK) {
        char *msg = "Failed to send I2C message to SAMD21\n";
        UART_Transmit(msg, strlength(msg));
      }

      break;
    }
    }
  }
}
