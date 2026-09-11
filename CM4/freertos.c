
// This file is a part of MRNIU/FreeRTOS-PortentaH7
// (https://github.com/MRNIU/FreeRTOS-PortentaH7).
//
// freertos.c for MRNIU/FreeRTOS-PortentaH7.

// NOTE: Even though is marked as unused, don't uncomment this file!
#include "FreeRTOS.h"
#include "adcs.h"
#include "cmsis_os2.h"
#include "debug.h"
#include "i2c.h"
#include "main.h"
#include "milo.h"
#include "obc.h"
#include "portable.h"
#include "stm32h7xx_hal_i2c.h"
#include "stm32h7xx_hal_uart.h"
#include "task.h"
#include <common.h>
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
  while (1) {
    QTZ_OBC_Routine_Tick(&GLOBAL_CTX);
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
