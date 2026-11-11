#include "../../obc.h"

// =======================================
// BEGIN STUB IMPLEMENTATIONS
// =======================================
// Stub implementation of logs. Since we're using the debug interface!
// Comment out the implementation if the logs become to much...
extern uint8_t __DEBUG_INNER_BUFFER[QTZ_DEBUG_CAPACITY];

void QTZ_OBC_ArmInterrupts() {}
void QTZ_OBC_BeginCritical() {}
void QTZ_OBC_EndCritical() {}
void QTZ_Panic() {}
QTZ_OBC_OperationResult QTZ_OBC_SendI2C_IT(QTZ_ByteArray *msg) {
  return QTZ_OBC_RESULT_OK;
};
QTZ_OBC_OperationResult QTZ_OBC_SendRS485_IT(QTZ_ByteArray *msg) {
  return QTZ_OBC_RESULT_OK;
};
// =======================================
// END STUB IMPLEMENTATIONS
// =======================================
