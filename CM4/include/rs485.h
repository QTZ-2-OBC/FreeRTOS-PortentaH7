#ifndef __rs485_H
#define __rs485_H
#include <strings.h>
#include <gpio.h>
#include <usart.h>


// NOTE: According to MaxCarrier docs, the GPIO_PIN_38 is used for DE when using RS-485.
// This translates to the PI10 on the Portenta H7
#define QTZ_RS485_DE_Pin GPIO_PIN_10
#define QTZ_RS485_GPIO_BASE GPIOI

typedef enum {
  QTZ_SENDRS485_OK,
  QTZ_SENDRS485_Parity_Error,
  QTZ_SENDRS485_Noise_Error,
  QTZ_SENDRS485_Frame_Error,
  QTZ_SENDRS485_Overrun_Error,
  QTZ_SENDRS485_DMA_Transfer_Error,
  QTZ_SENDRS485_ReceiverTimeout,
  QTZ_SENDRS485_Unknown,
} QTZ_SENDRS485_Result;
QTZ_SENDRS485_Result QTZ_SendRS485(UART_HandleTypeDef *handle, QTZ_ByteArray *buffer, uint32_t timeout);
#endif 
