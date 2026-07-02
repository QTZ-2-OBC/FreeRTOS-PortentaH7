#include "milo.h"
#include "debug.h"
#include "rs485.h"

// FIXME: This is a temporary implementation, it doesn't quite handle all
// errors!
QTZ_MILO_Result QTZ_MILO_SendCommand(QTZ_Command cmd,
                                     QTZ_ByteArray *response_buffer) {
  QTZ_Debug_Log("MILO: Command: %c - %d:%d - Expects: %d\n", cmd.command_id,
                cmd.send_timeout, cmd.recv_timeout, cmd.response_size);
  {
    QTZ_SENDRS485_Result result =
        QTZ_RS485_SendCStr((char *)&cmd.command_id, 1, cmd.send_timeout);
    if (QTZ_SENDRS485_OK != result) {
      QTZ_Debug_Error("MILO: Failed to send command! Error: %d\n", result);
      Error_Handler();
    }
  }

  QTZ_Debug_Log("MILO: Waiting for response...\n");
  {
    QTZ_RECEIVERS485_Result result =
        QTZ_RS485_Receive(response_buffer, cmd.response_size, cmd.recv_timeout);
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
