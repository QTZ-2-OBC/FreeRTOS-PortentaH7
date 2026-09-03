#include "include/obc.h"
#include "include/common.h"
#include "include/debug.h"
#include <string.h>

#define QTZ_OBC_ROUTINE_PREFIX "OBC-MR"
#define QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION_LOG_FMT                          \
  "[" QTZ_OBC_ROUTINE_PREFIX                                                   \
  "]: Can't transition from `%s` -> `%s`. State should be: `%s`"

#define QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION2_LOG_FMT                         \
  "[" QTZ_OBC_ROUTINE_PREFIX                                                   \
  "]: Can't transition from `%s` -> `%s`. State should be: `%s` or `%s`"

// -- I2C --
uint8_t i2c_rx_buffer[QTZ_OBC_I2C_RX_LEN];
uint8_t i2c_tx_buffer[QTZ_OBC_I2C_TX_LEN];

// -- UART --
uint8_t uart_rx_buffer[QTZ_OBC_UART_RX_LEN];
uint8_t uart_tx_buffer[QTZ_OBC_UART_TX_LEN];

// =================
// -- Private API --
// =================
char *QTZ_OBC_StateToStr(QTZ_OBC_State st) {
  switch (st) {
  case QTZ_OBC_STATE_IDLE:
    return "IDLE";
  case QTZ_OBC_STATE_ERROR:
    return "ERROR";
  case QTZ_OBC_STATE_HANDOVER_IDLE:
    return "HANDOVER_IDLE";
  }
}

char *QTZ_OBC_MiloTaskToStr(int variant) {
  switch (variant) {
  case QTZ_OBC_MILO_TASK_STATE_UNSTARTED: {
    return "MILO_UNSTARTED";
  } break;
  case QTZ_OBC_MILO_TASK_STATE_BEGIN: {
    return "MILO_BEGIN";
  } break;
  case QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE: {
    return "MILO_TAKE_PICTURE";
  } break;
  case QTZ_OBC_MILO_TASK_STATE_GET_DATA: {
    return "MILO_GET_DATA";
  } break;
  case QTZ_OBC_MILO_TASK_STATE_END: {
    return "MILO_END";
  } break;
  default:
    return "MILO_UNKNOWN";
  }
}

char *QTZ_OBC_CommandToStr(QTZ_OBC_Command cmd) {
  switch (cmd) {
  case QTZ_OBC_COMMAND_GOMSPACE_PING:
    return "PING";
  case QTZ_OBC_COMMAND_GOMSPACE_PING_ACK:
    return "PING_ACK";
  case QTZ_OBC_COMMAND_GOMSPACE_BEGIN_HANDOVER:
    return "BEGIN_HANDOVER";
  case QTZ_OBC_COMMAND_GOMSPACE_HANDOVER_BEGIN_ACK:
    return "HANDOVER_BEGIN_ACK";
  case QTZ_OBC_COMMAND_GOMSPACE_HEARTBEAT:
    return "HEARTBEAT";
  case QTZ_OBC_COMMAND_GOMSPACE_HEARTBEAT_ACK:
    return "HEARTBEAT_ACK";
  case QTZ_OBC_COMMAND_GOMSPACE_STATUS:
    return "STATUS";
  case QTZ_OBC_COMMAND_GOMSPACE_STATUS_ACK:
    return "STATUS_ACK";
  case QTZ_OBC_COMMAND_GOMSPACE_START_TASK:
    return "START_TASK";
  case QTZ_OBC_COMMAND_GOMSPACE_START_TASK_ACK:
    return "START_TASK_ACK";
  case QTZ_OBC_COMMAND_GOMSPACE_GET_IMAGE_CLASI:
    return "GET_IMAGE_CLASI";
  case QTZ_OBC_COMMAND_GOMSPACE_GET_IMAGE_CLASI_ACK:
    return "GET_IMAGE_CLASI_ACK";
  case QTZ_OBC_COMMAND_MILO_PING:
    return "PING";
  case QTZ_OBC_COMMAND_MILO_PING_ACK:
    return "PING_ACK";
  case QTZ_OBC_COMMAND_MILO_TAKE_PICTURE:
    return "TAKE_PICTURE";
  case QTZ_OBC_COMMAND_MILO_TAKE_PICTURE_ACK:
    return "TAKE_PICTURE_ACK";
  case QTZ_OBC_COMMAND_MILO_PICTURE_CLASI:
    return "PICTURE_CLASI";
  case QTZ_OBC_COMMAND_MILO_PICTURE_CLASI_ACK:
    return "PICTURE_CLASI_ACK";
  }
}

// ================
// -- Public API --
// ================

void QTZ_OBC_InitWithGlobals(QTZ_OBC_Ctx *ctx) {
  QTZ_ByteArray_CreateOnlyBuffer(rx_buff, i2c_rx_buffer, QTZ_OBC_I2C_RX_LEN);
  QTZ_ByteArray_CreateOnlyBuffer(tx_buff, i2c_tx_buffer, QTZ_OBC_I2C_TX_LEN);

  ctx->i2c.rx = rx_buff;
  ctx->i2c.tx = tx_buff;

  QTZ_ByteArray_CreateOnlyBuffer(rx2_buff, uart_rx_buffer, QTZ_OBC_UART_RX_LEN);
  QTZ_ByteArray_CreateOnlyBuffer(tx2_buff, uart_tx_buffer, QTZ_OBC_UART_TX_LEN);

  ctx->uart_rs485.rx = rx2_buff;
  ctx->uart_rs485.tx = tx2_buff;

  ctx->state = QTZ_OBC_STATE_IDLE;
  ctx->watchdog_ticks = 0;
}

QTZ_OBC_OperationResult QTZ_OBC_ParsePacket(QTZ_ByteArray *buffer,
                                            QTZ_OBC_Packet *p) {
  if (buffer == NULL || p == NULL) {
    return QTZ_OBC_RESULT_ERROR;
  }
  if (buffer->length < QTZ_OBC_PACKET_LEN) {
    return QTZ_OBC_RESULT_ERROR;
  }
  memcpy(p, buffer->data, QTZ_OBC_PACKET_LEN);
  QTZ_ByteArray_Reset(buffer);

  // if (buffer->length == QTZ_OBC_PACKET_LEN) {
  //   QTZ_ByteArray_Reset(buffer);
  // } else {
  //   size_t diff = buffer->length - QTZ_OBC_PACKET_LEN;
  //   memmove(buffer->data, buffer->data + QTZ_OBC_PACKET_LEN, diff);
  //   buffer->length = diff;
  // }

  return QTZ_OBC_RESULT_OK;
}

// Writes the packet to the buffer.
//
// Will overwrite all data available in the buffer with the contents of the
// packet.
QTZ_OBC_OperationResult QTZ_OBC_WritePacket(QTZ_ByteArray *buffer,
                                            QTZ_OBC_Packet p) {
  if (buffer == NULL) {
    return QTZ_OBC_RESULT_ERROR;
  }
  if (buffer->capacity < QTZ_OBC_PACKET_LEN) {
    return QTZ_OBC_RESULT_ERROR;
  }

  memcpy(buffer->data, &p, QTZ_OBC_PACKET_LEN);
  buffer->length = QTZ_OBC_PACKET_LEN;

  return QTZ_OBC_RESULT_OK;
}

#define QTZ_OBC_RespondWithPacket(ctx, buffer, p)                              \
  QTZ_Debug_Log("[" QTZ_OBC_ROUTINE_PREFIX                                     \
                "]: State is `%s`, responding with: [%c][%d][%c][%s][%d][%d]", \
                QTZ_OBC_StateToStr(ctx->state), p.protocol_id, p.status,       \
                p.subsys, QTZ_OBC_CommandToStr(p.cmd_id), p.param0, p.param1); \
  QTZ_OBC_WritePacket(buffer, p);

// Handle GOMSPACE OBC Command.
//
// Main OBC state machine for handling communications between the main OBC and
// the secondary OBC.
void QTZ_OBC_HandleHandoverCommand(QTZ_OBC_Ctx *ctx, QTZ_OBC_Packet *p) {
  if (ctx == NULL || p == NULL) {
    return;
  }

  switch (p->cmd_id) {
  case QTZ_OBC_COMMAND_GOMSPACE_PING: {
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE, // Send to gomspace.
        .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_PING_ACK,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->i2c.tx, resp);
  } break;
  case QTZ_OBC_COMMAND_GOMSPACE_BEGIN_HANDOVER: {
    if (ctx->state != QTZ_OBC_STATE_IDLE) {
      QTZ_Debug_Warning(QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION_LOG_FMT,
                        QTZ_OBC_StateToStr(ctx->state),
                        QTZ_OBC_StateToStr(QTZ_OBC_STATE_HANDOVER_IDLE),
                        QTZ_OBC_StateToStr(QTZ_OBC_STATE_IDLE));
      return;
    }
    ctx->state = QTZ_OBC_STATE_HANDOVER_IDLE;
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE, // Send to gomspace.
        .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_HANDOVER_BEGIN_ACK,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->i2c.tx, resp);
  } break;
  case QTZ_OBC_COMMAND_GOMSPACE_HEARTBEAT: {
    if (ctx->state != QTZ_OBC_STATE_HANDOVER_IDLE) {
      QTZ_Debug_Warning(
          "[" QTZ_OBC_ROUTINE_PREFIX
          "]: Can't heartbeat when no handover begin has been called!\n");
      return;
    }
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE, // Send to gomspace.
        .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_HEARTBEAT_ACK,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->i2c.tx, resp);
  } break;
  case QTZ_OBC_COMMAND_GOMSPACE_STATUS: {
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE, // Send to gomspace.
        .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_STATUS_ACK,
        .param0 = ctx->state,
        .param1 = ctx->milo_task.state,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->i2c.tx, resp);
  } break;
  case QTZ_OBC_COMMAND_GOMSPACE_START_TASK: {
    if (ctx->state != QTZ_OBC_STATE_HANDOVER_IDLE) {
      QTZ_Debug_Warning("[" QTZ_OBC_ROUTINE_PREFIX
                        "]: Can't start milo task, OBC is not on handover "
                        "idle mode! (Current: %s)",
                        QTZ_OBC_StateToStr(ctx->state));
      return;
    }
    if (ctx->milo_task.state != QTZ_OBC_MILO_TASK_STATE_UNSTARTED &&
        ctx->milo_task.state != QTZ_OBC_MILO_TASK_STATE_END) {
      QTZ_Debug_Warning(
          QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION2_LOG_FMT,
          QTZ_OBC_StateToStr(ctx->milo_task.state),
          QTZ_OBC_StateToStr(QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE),
          QTZ_OBC_StateToStr(QTZ_OBC_MILO_TASK_STATE_UNSTARTED),
          QTZ_OBC_StateToStr(QTZ_OBC_MILO_TASK_STATE_END));
      return;
    }
    ctx->milo_task.state = QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE;
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE, // Send to gomspace.
        .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_START_TASK_ACK,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->i2c.tx, resp);

    QTZ_OBC_Packet milo_req = {
        .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_MILO, // Send to MILO.
        .cmd_id = QTZ_OBC_COMMAND_MILO_PING,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->uart_rs485.tx, resp);
  } break;
  case QTZ_OBC_COMMAND_GOMSPACE_GET_IMAGE_CLASI: {
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE, // Send to gomspace.
        .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_GET_IMAGE_CLASI_ACK,
        .param0 = ctx->milo_task.image_classification,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->i2c.tx, resp);
  } break;
  }
}

QTZ_OBC_TaskCommandHandling QTZ_OBC_MILO_TaskTick(QTZ_OBC_Ctx *ctx,
                                                  QTZ_OBC_Packet *p) {
  switch (p->cmd_id) {
  case QTZ_OBC_COMMAND_MILO_PING_ACK: {
    if (ctx->milo_task.state != QTZ_OBC_MILO_TASK_STATE_BEGIN &&
        ctx->milo_task.state != QTZ_OBC_MILO_TASK_STATE_END) {
      QTZ_Debug_Warning(
          QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION2_LOG_FMT,
          QTZ_OBC_MiloTaskToStr(ctx->milo_task.state),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_BEGIN),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_END));
      return QTZ_OBC_TASK_COMMAND_UNHANDLED;
    }
    ctx->milo_task.state = QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE;
    QTZ_OBC_Packet req = {
        .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_MILO, // Send to MILO.
        .cmd_id = QTZ_OBC_COMMAND_MILO_TAKE_PICTURE,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->uart_rs485.tx, req);
  } break;
  case QTZ_OBC_COMMAND_MILO_TAKE_PICTURE_ACK: {
    if (ctx->milo_task.state != QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE) {
      QTZ_Debug_Warning(
          QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION_LOG_FMT,
          QTZ_OBC_MiloTaskToStr(ctx->milo_task.state),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_GET_DATA),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE));
      return QTZ_OBC_TASK_COMMAND_UNHANDLED;
    }
    ctx->milo_task.state = QTZ_OBC_MILO_TASK_STATE_GET_DATA;
    QTZ_OBC_Packet req = {
        .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_MILO, // Send to MILO.
        .cmd_id = QTZ_OBC_COMMAND_MILO_PICTURE_CLASI,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->uart_rs485.tx, req);
  } break;
  case QTZ_OBC_COMMAND_MILO_PICTURE_CLASI_ACK: {
    if (ctx->milo_task.state != QTZ_OBC_MILO_TASK_STATE_GET_DATA) {
      QTZ_Debug_Warning(
          QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION_LOG_FMT,
          QTZ_OBC_MiloTaskToStr(ctx->milo_task.state),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_END),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_GET_DATA));
      return QTZ_OBC_TASK_COMMAND_UNHANDLED;
    }
    ctx->milo_task.state = QTZ_OBC_MILO_TASK_STATE_END;
    // Task has ended, so now we wait for the main OBC to want to retrieve
    // the result of the operation. We just need to save the operation result.
    ctx->milo_task.image_classification = p->param0;
  } break;
  default:
    return QTZ_OBC_TASK_COMMAND_UNHANDLED;
  }

  // If none of the switch case statements above early returned, then the input
  // has been handled!
  return QTZ_OBC_TASK_COMMAND_HANDLED;
}

void QTZ_OBC_HandleSubsystemCommand(QTZ_OBC_Ctx *ctx, QTZ_OBC_Packet *p) {
  if (QTZ_OBC_TASK_COMMAND_HANDLED == QTZ_OBC_MILO_TaskTick(ctx, p)) {
    return; // MILO handled the command, so we don't need to check if the other
            // subsystems should handle it!
  }
  // NOTE: Add other tasks for other subsystems...
}

void QTZ_OBC_Routine_Tick(QTZ_OBC_Ctx *ctx) {
  ctx->watchdog_ticks += 1;

  QTZ_OBC_Packet p;
  if (QTZ_OBC_RESULT_OK != QTZ_OBC_ParsePacket(&ctx->i2c.rx, &p)) {
    QTZ_Debug_Warning("[" QTZ_OBC_ROUTINE_PREFIX
                      "]: No packet received from gomspace! Checking other "
                      "submodules...");
    if (QTZ_OBC_RESULT_OK != QTZ_OBC_ParsePacket(&ctx->uart_rs485.rx, &p)) {
      QTZ_Debug_Warning("[" QTZ_OBC_ROUTINE_PREFIX
                        "]: No packet received from any submodule either! "
                        "Doing nothing...");
      return;
    }
  }

  QTZ_Debug_Log("[" QTZ_OBC_ROUTINE_PREFIX
                "]: State is `%s`, received command: [%c][%d][%c][%s][%d][%d]",
                QTZ_OBC_StateToStr(ctx->state), p.protocol_id, p.status,
                p.subsys, QTZ_OBC_CommandToStr(p.cmd_id), p.param0, p.param1);

  if (p.subsys != QTZ_OBC_SUBSYSTEM_PORTENTA) {
    QTZ_Debug_Log("[" QTZ_OBC_ROUTINE_PREFIX
                  "]: Ignoring command since it doesn't belong to PortentaH7!");
    return;
  }

  switch (p.protocol_id) {
  case QTZ_OBC_PROTOCOL_HANDOVER: {
    QTZ_OBC_HandleHandoverCommand(ctx, &p);
  } break;
  case QTZ_OBC_PROTOCOL_SUBSYSTEMS: {
    QTZ_OBC_HandleSubsystemCommand(ctx, &p);
  } break;
  }
}
