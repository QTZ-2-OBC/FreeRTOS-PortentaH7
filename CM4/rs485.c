#include <rs485.h>
#include <stm32h7xx_hal_uart.h>
#include <strings.h>
#include <usart.h>

void QTZ_RS485_InitGPIO() {
  GPIO_InitTypeDef QTZ_RS485_GPIO = {};
  QTZ_RS485_GPIO.Pin = GPIO_PIN_10;
  QTZ_RS485_GPIO.Mode = GPIO_MODE_OUTPUT_PP;
  QTZ_RS485_GPIO.Speed = GPIO_SPEED_LOW;
  // QTZ_RS485_GPIO.Pull = GPIO_NOPULL;
  // QTZ_RS485_GPIO.Alternate = GPIO_AF3_LPUART;
  HAL_GPIO_Init(QTZ_RS485_GPIO_BASE, &QTZ_RS485_GPIO);
}

QTZ_SENDRS485_Result QTZ_SendRS485(UART_HandleTypeDef *handle,
                                   QTZ_ByteArray *buffer, uint32_t timeout) {
  // Switch to transmit mode
  HAL_GPIO_WritePin(QTZ_RS485_GPIO_BASE, QTZ_RS485_DE_Pin, GPIO_PIN_SET);
  if (HAL_OK !=
      HAL_UART_Transmit(handle, buffer->data, buffer->length, timeout)) {
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
  // Switch back to receive mode immediately after
  HAL_GPIO_WritePin(QTZ_RS485_GPIO_BASE, QTZ_RS485_DE_Pin, GPIO_PIN_RESET);
  return QTZ_SENDRS485_OK;
}
