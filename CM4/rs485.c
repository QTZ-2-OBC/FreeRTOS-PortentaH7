#include "cmsis_os2.h"
#include "common.h"
#include "debug.h"
#include <rs485.h>
#include <strings.h>
#include <usart.h>

// Enabled transmission but also disables reception
void QTZ_RS485_BeginTransmission() {
  HAL_GPIO_WritePin(QTZ_RS485_DE_BASE, QTZ_RS485_DE_Pin, GPIO_PIN_SET);
  osDelay(QTZ_RS485_PRE_DELAY);
}
void QTZ_RS485_EndTransmission() {
  HAL_GPIO_WritePin(QTZ_RS485_DE_BASE, QTZ_RS485_DE_Pin, GPIO_PIN_RESET);
  osDelay(QTZ_RS485_POST_DELAY);
}

void QTZ_RS485_BeginReception() {
  HAL_GPIO_WritePin(QTZ_RS485_RE_BASE, QTZ_RS485_RE_Pin, GPIO_PIN_RESET);
}
void QTZ_RS485_EndReception() {
  HAL_GPIO_WritePin(QTZ_RS485_RE_BASE, QTZ_RS485_RE_Pin, GPIO_PIN_SET);
}

void QTZ_RS485_EnableRS485(QTZ_Bool enable) {
  int state = GPIO_PIN_RESET;
  if (enable) {
    state = GPIO_PIN_SET;
  }
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, state);
}

void QTZ_RS485_ModeRS232(QTZ_Bool enable) {
  int state = GPIO_PIN_SET;
  if (enable) {
    state = GPIO_PIN_RESET;
  }
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, state);
}

void QTZ_RS485_YZTerm(QTZ_Bool enable) {
  int state = GPIO_PIN_RESET;
  if (enable) {
    state = GPIO_PIN_SET;
  }
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, state);
}

void QTZ_RS485_ABTerm(QTZ_Bool enable) {
  int state = GPIO_PIN_RESET;
  if (enable) {
    state = GPIO_PIN_SET;
  }
  HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_7, state);
}

void QTZ_RS485_FullDuplex(QTZ_Bool enable) {
  int state = GPIO_PIN_SET;
  if (enable) {
    state = GPIO_PIN_RESET;
  }
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, state);

  if (enable) {
    QTZ_RS485_ABTerm(QTZ_BOOL_TRUE);
    QTZ_RS485_YZTerm(QTZ_BOOL_TRUE);
  }
}

void QTZ_RS485_InitGPIO() {
  // Initialize RS485 mode
  GPIO_InitTypeDef QTZ_RS485_GPIO = {};
  QTZ_RS485_GPIO.Mode = GPIO_MODE_OUTPUT_PP;
  QTZ_RS485_GPIO.Speed = GPIO_SPEED_FREQ_HIGH;

  // Pin to enable/disable RS485
  QTZ_RS485_GPIO.Pin = GPIO_PIN_7;
  HAL_GPIO_Init(GPIOC, &QTZ_RS485_GPIO);

  // Pin to enable/disable RS232 mode
  QTZ_RS485_GPIO.Pin = GPIO_PIN_6;
  HAL_GPIO_Init(GPIOC, &QTZ_RS485_GPIO);

  // Pin to enable/disable XZ Term
  QTZ_RS485_GPIO.Pin = GPIO_PIN_3;
  HAL_GPIO_Init(GPIOG, &QTZ_RS485_GPIO);

  // Pin to enable/disable AB Term
  QTZ_RS485_GPIO.Pin = GPIO_PIN_7;
  HAL_GPIO_Init(GPIOJ, &QTZ_RS485_GPIO);

  // Pin to enable FULL DUPLEX
  QTZ_RS485_GPIO.Pin = GPIO_PIN_8;
  HAL_GPIO_Init(GPIOA, &QTZ_RS485_GPIO);

  // Pins for transmition
  QTZ_RS485_GPIO.Pin = QTZ_RS485_DE_Pin;
  HAL_GPIO_Init(QTZ_RS485_DE_BASE, &QTZ_RS485_GPIO);
  QTZ_RS485_GPIO.Pin = QTZ_RS485_RE_Pin;
  HAL_GPIO_Init(QTZ_RS485_RE_BASE, &QTZ_RS485_GPIO);

  // Disable everything...
  QTZ_RS485_EnableRS485(QTZ_BOOL_FALSE);
  QTZ_RS485_ModeRS232(QTZ_BOOL_FALSE);
  QTZ_RS485_FullDuplex(QTZ_BOOL_FALSE);
  QTZ_RS485_ABTerm(QTZ_BOOL_FALSE);
  QTZ_RS485_YZTerm(QTZ_BOOL_FALSE);

  // Enable only stuff we care about...
  QTZ_RS485_EnableRS485(QTZ_BOOL_TRUE);
  QTZ_RS485_ABTerm(QTZ_BOOL_TRUE);
  QTZ_RS485_BeginReception();
}

QTZ_SENDRS485_Result QTZ_RS485_SendCStr(char *const data, size_t len,
                                        uint32_t timeout) {
  QTZ_ByteArray arr;
  QTZ_ByteArray_InitWithLength(&arr, (uint8_t *)data, len, len);
  return QTZ_RS485_Send(&arr, timeout);
}

QTZ_SENDRS485_Result QTZ_RS485_Send(QTZ_ByteArray *buffer, uint32_t timeout) {
  // Switch to transmit mode
  QTZ_RS485_EndReception();
  QTZ_RS485_BeginTransmission();
  HAL_StatusTypeDef result = HAL_UART_Transmit(
      QTZ_RS485_UART_HANDLE, buffer->data, buffer->length, timeout);

  // Wait for the shift register to fully clock out the last byte
  while (__HAL_UART_GET_FLAG(QTZ_RS485_UART_HANDLE, UART_FLAG_TC) == RESET) {
  }

  // Switch back to receive mode after
  QTZ_RS485_EndTransmission();
  QTZ_RS485_BeginReception();

  if (HAL_OK != result) {
    switch (HAL_UART_GetError(QTZ_RS485_UART_HANDLE)) {
    case HAL_UART_ERROR_PE:
      return QTZ_SENDRS485_ParityError;
    case HAL_UART_ERROR_NE:
      return QTZ_SENDRS485_NoiseError;
    case HAL_UART_ERROR_FE:
      return QTZ_SENDRS485_FrameError;
    case HAL_UART_ERROR_ORE:
      return QTZ_SENDRS485_OverrunError;
    case HAL_UART_ERROR_DMA:
      return QTZ_SENDRS485_DMATransferError;
    case HAL_UART_ERROR_RTO:
      return QTZ_SENDRS485_ReceiverTimeout;
    default:
      return QTZ_SENDRS485_Unknown;
    }
  }
  return QTZ_SENDRS485_OK;
}

QTZ_RECEIVERS485_Result QTZ_RS485_Receive(QTZ_ByteArray *buffer, uint16_t size,
                                          uint32_t timeout) {
  if (size > QTZ_ByteArray_Remaining(buffer)) {
    return QTZ_RECEIVERS485_NotEnoughSpace;
  }

  // NOTE: Clear NEF and ORE to receive the whole message of the wire.
  __HAL_UART_CLEAR_NEFLAG(QTZ_RS485_UART_HANDLE);
  __HAL_UART_CLEAR_OREFLAG(QTZ_RS485_UART_HANDLE);
  QTZ_RS485_BeginReception();
  HAL_StatusTypeDef result = HAL_UART_Receive(
      QTZ_RS485_UART_HANDLE, QTZ_ByteArray_Current(buffer), size, timeout);
  if (HAL_OK != result) {
    QTZ_Debug_Log("HAL_UART result: %d\n", result);
    if (HAL_TIMEOUT == result) {
      return QTZ_RECEIVERS485_Timeout;
    }

    if (HAL_BUSY == result) {
      return QTZ_RECEIVERS485_Busy;
    }

    uint32_t inner_error = HAL_UART_GetError(QTZ_RS485_UART_HANDLE);
    QTZ_Debug_Log("HAL_UART inner error: %d\n", inner_error);
    switch (inner_error) {
    case HAL_UART_ERROR_PE:
      return QTZ_RECEIVERS485_ParityError;
    case HAL_UART_ERROR_NE:
      return QTZ_RECEIVERS485_NoiseError;
    case HAL_UART_ERROR_FE:
      return QTZ_RECEIVERS485_FrameError;
    case HAL_UART_ERROR_ORE:
      return QTZ_RECEIVERS485_OverrunError;
    case HAL_UART_ERROR_DMA:
      return QTZ_RECEIVERS485_DMATransferError;
    case HAL_UART_ERROR_RTO:
      return QTZ_RECEIVERS485_ReceiverTimeout;
    default:
      return QTZ_RECEIVERS485_Unknown;
    }
  }

  buffer->length += size;

  return QTZ_RECEIVERS485_OK;
}
