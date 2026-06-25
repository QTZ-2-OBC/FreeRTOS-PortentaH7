#include "usart.h"
#include <debug.h>
#include <string.h>

// NOTE: Same buffer defined on debug.c
// This is an implementation detail, this buffer should no be modified directly
// under normal circumstances But we need it here since QTZ_Debug_Print needs
// to know what to print!
extern uint8_t *__DEBUG_INNER_BUFFER[QTZ_DEBUG_CAPACITY];
const uint32_t QTZ_DEBUG_MAX_TIMEOUT = 0xFFFFFFFFUL;

void QTZ_Debug_Init() { MX_USART6_UART_Init(); }
void QTZ_Debug_Print() {
  uint16_t size = strlen((char *)__DEBUG_INNER_BUFFER);
  HAL_UART_Transmit(&huart6, (uint8_t *)__DEBUG_INNER_BUFFER, size,
                    QTZ_DEBUG_MAX_TIMEOUT);
}
