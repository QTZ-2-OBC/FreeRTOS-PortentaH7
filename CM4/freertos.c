
// This file is a part of MRNIU/FreeRTOS-PortentaH7
// (https://github.com/MRNIU/FreeRTOS-PortentaH7).
//
// freertos.c for MRNIU/FreeRTOS-PortentaH7.

// NOTE: Even though is marked as unused, don't uncomment this file!
#include "FreeRTOS.h"
#include "adcs.h"
#include "cmsis_os2.h"
#include "debug.h"
#include "main.h"
#include "milo.h"
#include "obc.h"
#include "portable.h"
#include "task.h"
#include <common.h>
#include <rs485.h>
#include <strings.h>

osThreadId_t milo_thread;
const osThreadAttr_t milo_thread_attributes = {
    .name = "milo_task",
    .priority = (osPriority_t)osPriorityNormal,
    .stack_size = 256 * 8,
};

osThreadId_t adcs_thread;
const osThreadAttr_t adcs_thread_attributes = {
    .name = "adcs_task",
    .priority = (osPriority_t)osPriorityNormal,
    .stack_size = 256 * 8,
};

#define mainHAL_MAX_TIMEOUT 0xFFFFFFFFUL

void PrintAvailableHeap() {
  size_t free_heap = xPortGetFreeHeapSize();
  QTZ_Debug_Log("Available HEAP SIZE: %d\n", free_heap);
}

void ADCS_Routine(void *argument) {
  (void)(argument);
  // NOTE: Initialize the LED pin on blue.
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.Pin = LED_B_Pin;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_B_GPIO_Port, &GPIO_InitStructure);
  HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);

  PrintAvailableHeap();

  QTZ_ByteArray_Create(buffer, data, 64);

  int commands_quantity = 5;
  // QTZ_Command commands[] = {};
  QTZ_OBC_Command commands[] = {
      // Ping the module, find out if it's ok!
      {
          .module_id = QTZ_OBC_MODULE_ADCS,
          .command_id = QTZ_ADCS_Ping,
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
      QTZ_ByteArray_Reset(&buffer);
      QTZ_OBC_Command cmd = commands[i];
      QTZ_OBC_Result response_status = QTZ_OBC_SendCommand(cmd, &buffer);
      if (response_status != QTZ_OBC_OK) {
        QTZ_Debug_Error("Response is not ok! HALTING...");
        Error_Handler();
      }

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

void MILO_Routine(void *argument) {
  (void)(argument);
  // NOTE: Initialize the LED pin on blue.
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.Pin = LED_B_Pin;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_B_GPIO_Port, &GPIO_InitStructure);
  HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);

  PrintAvailableHeap();

  QTZ_ByteArray_Create(buffer, data, 64);

  int commands_quantity = 5;
  // QTZ_Command commands[] = {};
  QTZ_OBC_Command commands[] = {
      // Ping the module, find out if it's ok!
      {
          .module_id = QTZ_OBC_MODULE_MILO,
          .command_id = QTZ_MILO_Ping,
          .response_size = 5,
          .send_timeout = 1000,
          .recv_timeout = 7000,
      },
      // Activate earthlimb model
      {
          .module_id = QTZ_OBC_MODULE_MILO,
          .command_id = QTZ_MILO_EnableEarthlimbModel,
          .response_size = 19,
          .send_timeout = 1000,
          .recv_timeout = 7000,
          .post_delay = 3000, // Wait for an image to arrive...
      },
      // Retrieve image statistics!
      {
          .module_id = QTZ_OBC_MODULE_MILO,
          .command_id = QTZ_MILO_ImageStatistics,
          .response_size = 8,
          .send_timeout = 1000,
          .recv_timeout = 7000,
      },
      // Take a picture
      {
          .module_id = QTZ_OBC_MODULE_MILO,
          .command_id = QTZ_MILO_Snapshot,
          .response_size = 14,
          .send_timeout = 1000,
          .recv_timeout = 7000,
      },
      // Reset the camera to save the picture!
      {
          .module_id = QTZ_OBC_MODULE_MILO,
          .command_id = QTZ_MILO_ResetCam,
          .response_size = 5,
          .send_timeout = 1000,
          .recv_timeout = 7000,
          .post_delay = 5000, // Wait for reset to take effect...
      },
  };

  while (1) {
    osDelay(750);
    QTZ_ByteArray_Reset(&buffer);
    HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);

    for (int i = 0; i < commands_quantity; i++) {
      QTZ_ByteArray_Reset(&buffer);
      QTZ_OBC_Command cmd = commands[i];
      QTZ_OBC_Result response_status = QTZ_OBC_SendCommand(cmd, &buffer);
      if (response_status != QTZ_OBC_OK) {
        QTZ_Debug_Error("Response is not ok! HALTING...");
        Error_Handler();
      }

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

void MX_FREERTOS_Init(void) {
  // milo_thread = osThreadNew(MILO_Routine, NULL, &milo_thread_attributes);
  adcs_thread = osThreadNew(ADCS_Routine, NULL, &adcs_thread_attributes);
}
