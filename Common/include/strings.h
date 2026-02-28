#ifndef QTZ_LIB_STRINGS
#define QTZ_LIB_STRINGS

#include "common.h"

// Get the number of digits of n.
size_t QTZ_DigitQuantity(size_t n);

typedef enum {
  QTZ_FMTSIZET_OK,
  QTZ_FMTSIZET_BUFFER_NOT_LARGE_ENOUGH,
} QTZ_FMTSIZET_Result;

// Convert n into a string containing n.
QTZ_FMTSIZET_Result QTZ_FmtSizeT(size_t n, QTZ_ByteArray *buffer);

#endif
