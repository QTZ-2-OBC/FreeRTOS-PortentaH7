#include "milo.h"
#include "debug.h"
#include "rs485.h"

// FIXME: This is a temporary implementation, it doesn't quite handle all
// errors!
QTZ_MILO_RESULT QTZ_MILO_SendCommand(QTZ_MILO_COMMAND cmd,
                                     QTZ_ByteArray *response_buffer,
                                     uint32_t timeout) {
  QTZ_Debug_Log("MILO: Command: %c - %d\n", cmd, timeout);
  {
    QTZ_SENDRS485_Result result = QTZ_RS485_SendCStr((char *)&cmd, 1, timeout);
    if (QTZ_SENDRS485_OK != result) {
      QTZ_Debug_Error("MILO: Failed to send command! Error: %d\n", result);
      Error_Handler();
    }
  }

  {
    QTZ_RECEIVERS485_Result result =
        QTZ_RS485_Receive(response_buffer, timeout);
    if (QTZ_RECEIVERS485_OK != result) {
      QTZ_Debug_Error(
          "MILO: Failed to receive response from command! Error: %d\n", result);
      Error_Handler();
    }
  }

  uint8_t response_status = response_buffer->data[0];
  QTZ_Debug_Log("MILO: Received response: %d - %c\n", response_status,
                response_status);
  return response_status;
}
