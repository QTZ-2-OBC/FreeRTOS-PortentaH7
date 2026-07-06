#include "./include/debug.h"
#include "include/debug.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#ifdef QTZ_DEBUG
uint8_t __DEBUG_INNER_BUFFER[QTZ_DEBUG_CAPACITY];

#define QTZ_LOG_WITH_PREFIX(prefix)                                            \
  {                                                                            \
    snprintf((char *)__DEBUG_INNER_BUFFER, QTZ_DEBUG_CAPACITY, prefix);        \
    va_list args;                                                              \
    va_start(args, msg);                                                       \
    int needed_space = vsnprintf((char *)(__DEBUG_INNER_BUFFER + 5),           \
                                 QTZ_DEBUG_CAPACITY - 5, msg, args);           \
    if (needed_space >= QTZ_DEBUG_CAPACITY) {                                  \
      __DEBUG_INNER_BUFFER[QTZ_DEBUG_CAPACITY - 1] = 0;                        \
    }                                                                          \
    va_end(args);                                                              \
    QTZ_Debug_Print();                                                         \
  }
#endif

// NOTE: Make sure every prefix is the same length!
// If you ever change the length of all prefixes then update the
// QTZ_LOG_WITH_PREFIX accordingly!

// Logs a debug message with the default priority
void QTZ_Debug_Log(const char *msg, ...) {
#ifdef QTZ_DEBUG
  QTZ_LOG_WITH_PREFIX("LOG: ");
#endif
}
// Logs a warning message with a warning priority
void QTZ_Debug_Warning(const char *msg, ...) {
#ifdef QTZ_DEBUG
  QTZ_LOG_WITH_PREFIX("WAR: ");
#endif
}
// Logs an error message with the highest priority
void QTZ_Debug_Error(const char *msg, ...) {
#ifdef QTZ_DEBUG
  QTZ_LOG_WITH_PREFIX("ERR: ");
#endif
}
