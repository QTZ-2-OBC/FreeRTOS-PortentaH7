
// This file is a part of MRNIU/FreeRTOS-PortentaH7
// (https://github.com/MRNIU/FreeRTOS-PortentaH7).
//
// freertos.c for MRNIU/FreeRTOS-PortentaH7.

// NOTE: Even though is marked as unused, don't uncomment this file!
#include "FreeRTOS.h"
#include "adcs.h"
#include "cmsis_os2.h"
#include "debug.h"
#include "handover_slave.h"
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

osThreadId_t handover_thread;
const osThreadAttr_t handover_thread_attributes = {
    .name = "handover_task",
    // Above adcs_thread's priority: the A3200 enforces a 10 ms round-trip
    // timeout on every handover I2C transaction, so this task must be
    // scheduled promptly. Revisit if bring-up shows ho ping timeouts.
    .priority = (osPriority_t)osPriorityAboveNormal,
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

  int commands_quantity = 1;
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
        // Softened from Error_Handler() (halted the whole task forever on
        // any single RS485 hiccup) to log-and-continue: rs485_framed.c
        // already records this failure for the handover heartbeat's RS485
        // status bit (see QTZ_RS485F_IsHealthy()) — nothing further to do
        // here but skip the rest of this cycle's commands and retry next
        // cycle instead of taking the board down with it.
        QTZ_Debug_Warning("Response is not ok (result=%d) - skipping rest of "
                          "this cycle\n",
                          response_status);
        break;
      }

      if (QTZ_MILO_ImageStatistics == cmd.command_id) {
        if (buffer.length == 0) {
          QTZ_Debug_Warning("No data written to buffer - skipping\n");
          continue;
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
        // Softened from Error_Handler() (halted the whole task forever on
        // any single RS485 hiccup) to log-and-continue: rs485_framed.c
        // already records this failure for the handover heartbeat's RS485
        // status bit (see QTZ_RS485F_IsHealthy()) — nothing further to do
        // here but skip the rest of this cycle's commands and retry next
        // cycle instead of taking the board down with it.
        QTZ_Debug_Warning("Response is not ok (result=%d) - skipping rest of "
                          "this cycle\n",
                          response_status);
        break;
      }

      if (QTZ_MILO_ImageStatistics == cmd.command_id) {
        if (buffer.length == 0) {
          QTZ_Debug_Warning("No data written to buffer - skipping\n");
          continue;
        }
        QTZ_Debug_Log("Statistics: %.*s\n", buffer.length - 1, buffer.data + 1);
      }
    }
    osDelay(5000);
  }
}

void MX_FREERTOS_Init(void) {
  // milo_thread = osThreadNew(MILO_Routine, NULL, &milo_thread_attributes);
  // adcs_thread = osThreadNew(ADCS_Routine, NULL, &adcs_thread_attributes);

  // Quetzal-2 handover prototype: I2C1 must already be initialised (see
  // CM4/main.c's MX_I2C1_Init() call) before this runs.
  QTZ_HandoverSlave_Init();
  handover_thread =
      osThreadNew(HandoverSlave_Routine, NULL, &handover_thread_attributes);
}
