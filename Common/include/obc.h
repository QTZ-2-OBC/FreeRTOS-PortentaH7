#ifndef __obc_H
#define __obc_H
#include "common.h"
#include <stdint.h>

typedef enum {
  QTZ_OBC_MODULE_MILO,
  QTZ_OBC_MODULE_ADCS,
} QTZ_OBC_Module;

typedef struct {
  // Send operation timeout.
  uint32_t send_timeout;
  // Receive operation timeout.
  uint32_t recv_timeout;
  // Delay before doing any operation.
  uint32_t pre_delay;
  // Delay after doing all operations.
  uint32_t post_delay;
  // The size of the response in bytes.
  //
  // This amount will be written to the provided buffer.
  uint16_t response_size;
  // The module ID responsible for managing the command.
  QTZ_OBC_Module module_id;
  // The command ID that will be sent.
  uint8_t command_id;
} QTZ_OBC_Command;

typedef enum {
  QTZ_OBC_OK = '0',
  QTZ_OBC_Timeout,
} QTZ_OBC_Result;

QTZ_OBC_Result QTZ_OBC_SendCommand(QTZ_OBC_Command cmd,
                                   QTZ_ByteArray *response_buffer);
#endif
