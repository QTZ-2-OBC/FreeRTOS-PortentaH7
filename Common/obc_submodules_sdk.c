#include "include/obc.h"
#include <string.h>

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
  default:
    return "UNKNOWN";
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
  case QTZ_OBC_COMMAND_GOMSPACE_BEGIN_HANDOVER_ACK:
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
  case QTZ_OBC_COMMAND_GOMSPACE_ABORT_HANDOVER:
    return "ABORT_HANDOVER";
  case QTZ_OBC_COMMAND_GOMSPACE_ABORT_HANDOVER_ACK:
    return "ABORT_HANDOVER_ACK";
  case QTZ_OBC_COMMAND_TIMEOUT:
    return "TIMEOUT";
  default:
    return "UNKNOWN";
  }
}

QTZ_OBC_OperationResult QTZ_OBC_ParsePacket(QTZ_ByteArray *buffer,
                                            QTZ_OBC_Packet *p) {
  if (buffer == NULL || p == NULL) {
    return QTZ_OBC_RESULT_ERROR;
  }

  QTZ_OBC_OperationResult result = QTZ_OBC_RESULT_ERROR;

  QTZ_OBC_BeginCritical();
  if (buffer->length >= QTZ_OBC_PACKET_LEN) {
    memcpy(p, buffer->data, QTZ_OBC_PACKET_LEN);
    QTZ_ByteArray_Reset(buffer);
    result = QTZ_OBC_RESULT_OK;
  }
  QTZ_OBC_EndCritical();

  return result;
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

  QTZ_OBC_OperationResult result = QTZ_OBC_RESULT_ERROR;

  QTZ_OBC_BeginCritical();
  if (buffer->capacity >= QTZ_OBC_PACKET_LEN) {
    memcpy(buffer->data, &p, QTZ_OBC_PACKET_LEN);
    buffer->length = QTZ_OBC_PACKET_LEN;
    result = QTZ_OBC_RESULT_OK;
  }
  QTZ_OBC_EndCritical();

  return result;
}
