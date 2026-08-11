#include "obc.h"
#include "cmsis_os2.h"
#include "debug.h"
#include "rs485.h"
#include "rs485_framed.h"

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
  QTZ_Debug_Log("%s: Command: '%c' - Pre/Post Delay: %d:%d - Timeouts: %d:%d - "
                "Expects: %d\n",
                QTZ_OBC_ToStr(cmd.module_id), cmd.command_id, cmd.pre_delay,
                cmd.post_delay, cmd.send_timeout, cmd.recv_timeout,
                cmd.response_size);
  if (cmd.pre_delay != 0) {
    osDelay(cmd.pre_delay);
  }

  {
    /* Framed (CRC16 + ACK + retry) send — same wire protocol as the A3200
     * side's rs485_link.c, see rs485_framed.h. cmd.send_timeout is reused
     * as the per-attempt ACK-wait timeout, keeping each command table
     * entry's existing per-command tuning meaningful. */
    QTZ_RS485F_Result result = QTZ_RS485F_SendAcked(
        &cmd.command_id, 1, cmd.send_timeout, QTZ_RS485F_MAX_RETRIES);
    if (QTZ_RS485F_OK != result) {
      QTZ_Debug_Error("%s: Failed to send command! Error: %d\n",
                      QTZ_OBC_ToStr(cmd.module_id), result);
      Error_Handler();
    }
  }

  QTZ_Debug_Log("%s: Waiting for response...\n", QTZ_OBC_ToStr(cmd.module_id));
  {
    uint8_t frame_len = 0;
    uint8_t max_payload = (uint8_t)QTZ_ByteArray_Remaining(response_buffer);
    QTZ_RS485F_Result result = QTZ_RS485F_RecvFrame(
        response_buffer->data, &frame_len, max_payload, cmd.recv_timeout);
    if (QTZ_RS485F_OK != result) {
      QTZ_Debug_Error(
          "%s: Failed to receive response from command! Error: %d\n",
          QTZ_OBC_ToStr(cmd.module_id), result);
      Error_Handler();
    }
    response_buffer->length += frame_len;
  }

  QTZ_OBC_Result response_status = response_buffer->data[0];
  QTZ_Debug_Log("%s: Received response: D:%d C:%c - '%.*s'\n",
                QTZ_OBC_ToStr(cmd.module_id), response_status, response_status,
                response_buffer->length, response_buffer->data);

  if (response_status != QTZ_OBC_OK) {
    QTZ_Debug_Warning("%s: The response status is not 'D:%d'!",
                      QTZ_OBC_ToStr(cmd.module_id), QTZ_OBC_OK);
    // Normally this would be an error we would like to handle in some way
    // a simple print is enough since it's the caller that determines if it's an
    // error or not

    // Error_Handler();
  }

  return response_status;
}
