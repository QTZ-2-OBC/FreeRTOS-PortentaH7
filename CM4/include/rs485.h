#include <strings.h>
#include <usart.h>

// FIXME: Define the DE/RE Pin for RS485 transmission!
#define QTZ_RS485_DE_Pin 000
#define QTZ_RS485_RE_Pin 000

typedef enum {
  QTZ_SendRS485_OK,
  QTZ_SendRS485_Parity_Error,
  QTZ_SendRS485_Noise_Error,
  QTZ_SendRS485_Frame_Error,
  QTZ_SendRS485_Overrun_Error,
  QTZ_SendRS485_DMA_Transfer_Error,
  QTZ_SendRS485_ReceiverTimeout,
  QTZ_SendRS485_Unknown,
} QTZ_SendRS485_Result;
QTZ_SendRS485_Result QTZ_SendRS485(UART_HandleTypeDef *handle,
                                   GPIO_TypeDef *port, QTZ_ByteArray *buffer,
                                   uint32_t timeout);
