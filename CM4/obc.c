#include "obc.h"
#include "cmsis_os2.h"
#include "debug.h"
#include "rs485.h"

char *QTZ_OBC_ToStr(QTZ_OBC_Module module_id) {
#ifdef QTZ_DEBUG
  switch (module_id) {
  case QTZ_OBC_MODULE_MILO:
    return "MILO";
  case QTZ_OBC_MODULE_ADCS:
    return "ADCS";
  default:
    return "UNKNOWN";
  }
#else
  return "";
#endif
}

// FIXME: This is a temporary implementation, it doesn't quite handle all
// errors!
QTZ_OBC_Result QTZ_OBC_SendCommand(QTZ_OBC_Command cmd,
                                   QTZ_ByteArray *response_buffer) {
  char *module_name = QTZ_OBC_ToStr(cmd.module_id);
  QTZ_Debug_Log("%s: Command: '%c' - Pre/Post Delay: %d:%d - Timeouts: %d:%d - "
                "Expects: %d\n",
                module_name, cmd.command_id, cmd.pre_delay, cmd.post_delay,
                cmd.send_timeout, cmd.recv_timeout, cmd.response_size);
  if (cmd.pre_delay != 0) {
    osDelay(cmd.pre_delay);
  }

  QTZ_Debug_Log("%s: Waiting for response...\n", QTZ_OBC_ToStr(cmd.module_id));
  {
    QTZ_RECEIVERS485_Result result =
        QTZ_RS485_Receive(response_buffer, cmd.response_size, cmd.recv_timeout);
    if (QTZ_RECEIVERS485_OK != result) {
      QTZ_Debug_Error(
          "%s: Failed to receive response from command! Error: %d\n",
          module_name, result);
      Error_Handler();
    }
  }
  QTZ_OBC_Result response_status = response_buffer->data[0];
  QTZ_Debug_Log("%s: Received response: D:%d C:%c - '%.*s'\n", module_name,
                response_status, response_status, response_buffer->length,
                response_buffer->data);

  QTZ_Debug_Log("%s: Sending ACK...", module_name);
  {
    QTZ_SENDRS485_Result result =
        QTZ_RS485_SendCStr(QTZ_OBC_ACK, 1, cmd.send_timeout);
    if (QTZ_SENDRS485_OK != result) {
      QTZ_Debug_Error("%s: Failed to send command! Error: %d\n", module_name,
                      result);
      Error_Handler();
    }
  }
  QTZ_Debug_Log("%s: Command done!", module_name);

  if (response_status != QTZ_OBC_OK) {
    QTZ_Debug_Warning("%s: The response status is not 'D:%d'!", module_name,
                      QTZ_OBC_OK);
    // NOTE: Normally this would be an error we would like to handle in some way
    // a simple print is enough since it's the caller that determines if it's an
    // error or not

    // Error_Handler();
  }

  return response_status;
}
