#include "cmsis_os2.h"
#include "common.h"
#include "stm32h7xx_hal_gpio.h"
#include <rs485.h>
#include <stm32h7xx_hal_uart.h>
#include <strings.h>
#include <usart.h>

// Enabled transmission but also disables reception
void QTZ_RS485_EnableTransmission() {
  HAL_GPIO_WritePin(QTZ_RS485_RE_BASE, QTZ_RS485_RE_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(QTZ_RS485_DE_BASE, QTZ_RS485_DE_Pin, GPIO_PIN_SET);
  osDelay(5);
}

// Enabled reception but also disables transmission
void QTZ_RS485_EnableReception() {
  HAL_GPIO_WritePin(QTZ_RS485_DE_BASE, QTZ_RS485_DE_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(QTZ_RS485_RE_BASE, QTZ_RS485_RE_Pin, GPIO_PIN_RESET);
  osDelay(5);
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
}

QTZ_SENDRS485_Result QTZ_RS485_SendCStr(UART_HandleTypeDef *handle,
                                        char *const data, size_t len,
                                        uint32_t timeout) {
  QTZ_ByteArray arr;
  QTZ_ByteArray_InitWithLength(&arr, (uint8_t *)data, len, len);
  return QTZ_RS485_Send(handle, &arr, timeout);
}

QTZ_SENDRS485_Result QTZ_RS485_Send(UART_HandleTypeDef *handle,
                                    QTZ_ByteArray *buffer, uint32_t timeout) {
  // Switch to transmit mode
  QTZ_RS485_EnableTransmission();
  HAL_StatusTypeDef result =
      HAL_UART_Transmit(handle, buffer->data, buffer->length, timeout);

  // Wait for the shift register to fully clock out the last byte
  while (__HAL_UART_GET_FLAG(handle, UART_FLAG_TC) == RESET) {
  }

  // Switch back to receive mode after
  QTZ_RS485_EnableReception();

  if (HAL_OK != result) {
    switch (HAL_UART_GetError(handle)) {
    case HAL_UART_ERROR_PE:
      return QTZ_SENDRS485_Parity_Error;
    case HAL_UART_ERROR_NE:
      return QTZ_SENDRS485_Noise_Error;
    case HAL_UART_ERROR_FE:
      return QTZ_SENDRS485_Frame_Error;
    case HAL_UART_ERROR_ORE:
      return QTZ_SENDRS485_Overrun_Error;
    case HAL_UART_ERROR_DMA:
      return QTZ_SENDRS485_DMA_Transfer_Error;
    case HAL_UART_ERROR_RTO:
      return QTZ_SENDRS485_ReceiverTimeout;
    default:
      return QTZ_SENDRS485_Unknown;
    }
  }
  return QTZ_SENDRS485_OK;
}
