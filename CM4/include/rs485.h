#ifndef __rs485_H
#define __rs485_H
#include <strings.h>
#include <gpio.h>
#include <usart.h>


// NOTE: According to MaxCarrier docs, the GPIO_PIN_38 is used for DE when using RS-485.
// This translates to the PI10 on the Portenta H7
#define QTZ_RS485_DE_Pin GPIO_PIN_10
#define QTZ_RS485_DE_BASE GPIOI

// NOTE: According to MaxCarriers docs: https://docs.arduino.cc/tutorials/portenta-max-carrier/user-manual/#using-arduino-ide-7
// This is also necessary.
#define QTZ_RS485_RE_Pin GPIO_PIN_10
#define QTZ_RS485_RE_BASE GPIOJ

// UART Base handle to use for the UART protocol
#define QTZ_RS485_UART_HANDLE &huart4

// Initialize the GPIO pins for RS485 transmission.
void QTZ_RS485_InitGPIO();

typedef enum {
  QTZ_SENDRS485_OK,
  QTZ_SENDRS485_ParityError,
  QTZ_SENDRS485_NoiseError,
  QTZ_SENDRS485_FrameError,
  QTZ_SENDRS485_OverrunError,
  QTZ_SENDRS485_DMATransferError,
  QTZ_SENDRS485_ReceiverTimeout,
  QTZ_SENDRS485_Unknown,
} QTZ_SENDRS485_Result;
QTZ_SENDRS485_Result QTZ_RS485_Send(QTZ_ByteArray *buffer, uint32_t timeout);

QTZ_SENDRS485_Result QTZ_RS485_SendCStr(char *const data, size_t len, uint32_t timeout);

typedef enum {
	QTZ_RECEIVERS485_OK,
	QTZ_RECEIVERS485_NotEnoughSpace,
	QTZ_RECEIVERS485_ParityError,
	QTZ_RECEIVERS485_NoiseError,
	QTZ_RECEIVERS485_FrameError,
	QTZ_RECEIVERS485_OverrunError,
	QTZ_RECEIVERS485_DMATransferError,
	QTZ_RECEIVERS485_ReceiverTimeout,
	QTZ_RECEIVERS485_Unknown,
}QTZ_RECEIVERS485_Result;
QTZ_RECEIVERS485_Result QTZ_RS485_Receive(QTZ_ByteArray *buffer, uint32_t timeout);
#endif 
