#include <rs485.h>
#include <stm32h7xx_hal_uart.h>
#include <strings.h>
#include <usart.h>

void QTZ_RS485_SetTransmitMode(GPIO_TypeDef *port, uint16_t REPin,
                               uint16_t DEPin) {
  HAL_GPIO_WritePin(port, REPin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(port, DEPin, GPIO_PIN_SET);
}

void QTZ_RS485_SetReceiveMode(GPIO_TypeDef *port, uint16_t REPin,
                              uint16_t DEPin) {
  HAL_GPIO_WritePin(port, REPin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(port, DEPin, GPIO_PIN_RESET);
}

QTZ_SendRS485_Result QTZ_SendRS485(UART_HandleTypeDef *handle,
                                   GPIO_TypeDef *port, QTZ_ByteArray *buffer,
                                   uint32_t timeout) {
  // Switch to transmit mode
  QTZ_RS485_SetTransmitMode(port, QTZ_RS485_RE_Pin, QTZ_RS485_DE_Pin);
  if (HAL_OK !=
      HAL_UART_Transmit(handle, buffer->data, buffer->length, timeout)) {
    switch (HAL_UART_GetError(handle)) {
    case HAL_UART_ERROR_PE:
      return QTZ_SendRS485_Parity_Error;
    case HAL_UART_ERROR_NE:
      return QTZ_SendRS485_Noise_Error;
    case HAL_UART_ERROR_FE:
      return QTZ_SendRS485_Frame_Error;
    case HAL_UART_ERROR_ORE:
      return QTZ_SendRS485_Overrun_Error;
    case HAL_UART_ERROR_DMA:
      return QTZ_SendRS485_DMA_Transfer_Error;
    case HAL_UART_ERROR_RTO:
      return QTZ_SendRS485_ReceiverTimeout;
    default:
      return QTZ_SendRS485_Unknown;
    }
  }
  // Switch back to receive mode immediately after
  QTZ_RS485_SetReceiveMode(port, QTZ_RS485_RE_Pin, QTZ_RS485_DE_Pin);
  return QTZ_SendRS485_OK;
}
