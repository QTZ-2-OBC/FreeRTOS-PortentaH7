
// This file is a part of MRNIU/FreeRTOS-PortentaH7
// (https://github.com/MRNIU/FreeRTOS-PortentaH7).
//
// freertos.c for MRNIU/FreeRTOS-PortentaH7.

// NOTE: Even though is marked as unused, don't uncomment this file!
#include "FreeRTOS.h"
#include "../Common/obc.h"
#include "cmsis_os2.h"
#include "portable.h"
#include "stm32h7xx_hal.h"
#include "task.h"
#include <rs485.h>
#include <strings.h>

#define QTZ_FREERTOS_DEBUG_PREFIX "[OBC-RTOS]"

osThreadId_t main_routine_thread;
const osThreadAttr_t main_routine_thread_attributes = {
    .name = "main_routine_task",
    .priority = (osPriority_t)osPriorityNormal,
    .stack_size = 1024 * 2,
};

#define mainHAL_MAX_TIMEOUT 0xFFFFFFFFUL

void PrintAvailableHeap() {
  size_t free_heap = xPortGetFreeHeapSize();
  QTZ_Debug_Log("Available HEAP SIZE: %d\n", free_heap);
}

void MX_MainRoutine(void *argument) {
  (void)argument;

  QTZ_OBC_Packet ping = {
      .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
      .status = QTZ_OBC_RESULT_OK,
      .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
      .cmd_id = QTZ_OBC_COMMAND_MILO_PING,
      .param0 = 0,
      .param1 = 0,
  };
  while (1) {
    QTZ_OBC_WritePacket(&GLOBAL_CTX.i2c.rx,
                        ping); // Send the ping command over and over
    QTZ_Debug_Log("[MAIN]: Pre-tick UART4 State: %u - %u", huart4.gState,
                  HAL_UART_STATE_READY);
    QTZ_OBC_Routine_Tick(&GLOBAL_CTX);
    QTZ_Debug_Log("[MAIN]: Post-tick UART4 State: %u - %u", huart4.gState,
                  HAL_UART_STATE_READY);
    // QTZ_Debug_Log("[MAIN]: .uart_rs485.length: %d - .uart_rs485.capacity:
    // %d",
    //               GLOBAL_CTX.uart_rs485.tx.length,
    //               GLOBAL_CTX.uart_rs485.tx.capacity)

    HAL_Delay(1750);
    // osDelay(1750);
    // TODO: Implement the watchdog logic...
  }
}

void MX_FREERTOS_Init(void) {
  // milo_thread = osThreadNew(MILO_Routine, NULL, &milo_thread_attributes);
  // adcs_thread = osThreadNew(ADCS_Routine, NULL, &adcs_thread_attributes);
  // i2c_thread = osThreadNew(I2C_Routine, NULL, &i2c_thread_attributes);

  main_routine_thread =
      osThreadNew(MX_MainRoutine, NULL, &main_routine_thread_attributes);
}
