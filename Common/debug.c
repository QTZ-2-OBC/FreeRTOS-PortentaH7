#include "./include/debug.h"
#include "include/debug.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#ifdef QTZ_DEBUG
uint8_t __DEBUG_INNER_BUFFER[QTZ_DEBUG_CAPACITY];
void QTZ_Debug_InnerLog(const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  int needed_space =
      vsnprintf((char *)(__DEBUG_INNER_BUFFER), QTZ_DEBUG_CAPACITY, msg, args);
  if (needed_space >= QTZ_DEBUG_CAPACITY) {
    __DEBUG_INNER_BUFFER[QTZ_DEBUG_CAPACITY - 1] = 0;
  }
  va_end(args);
  QTZ_Debug_Print();
}
#endif
