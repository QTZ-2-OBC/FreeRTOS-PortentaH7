#define QTZ_LIB_IMPLEMENTATION
#include "../../obc.h"
#include <stdlib.h>

void QTZ_Panic() {
#ifdef QTZ_COVERAGE
  return;
#else
  abort();
#endif
}

// =======================================
// BEGIN STUB IMPLEMENTATIONS
// =======================================
extern uint8_t __DEBUG_INNER_BUFFER[QTZ_DEBUG_CAPACITY];
void QTZ_Debug_Print() {}
void QTZ_OBC_ArmInterrupts() {}
void QTZ_OBC_BeginCritical() {}
void QTZ_OBC_EndCritical() {}

QTZ_OBC_OperationResult QTZ_OBC_SendI2C_IT(QTZ_ByteArray *msg) {
  return QTZ_OBC_RESULT_OK;
}
QTZ_OBC_OperationResult QTZ_OBC_SendRS485_IT(QTZ_ByteArray *msg) {
  return QTZ_OBC_RESULT_OK;
}
// =======================================
// END STUB IMPLEMENTATIONS
// =======================================
