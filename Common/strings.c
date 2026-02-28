#include "include/strings.h"
#include "include/common.h"
#include <math.h>

size_t QTZ_DigitQuantity(size_t n) {
  if (n < 2) { // Handle edge cases... AKA (0, 1)
    return 1;
  } else {
    // This formula only works if n is 2 or more!
    return ((size_t)log10(n)) + 1;
  }
}

QTZ_FMTSIZET_Result QTZ_FmtSizeT(size_t n, QTZ_ByteArray *buffer) {

  if (n == 0) {
    if (buffer->length <= 0) {
      return QTZ_FMTSIZET_BUFFER_NOT_LARGE_ENOUGH;
    }

    if (QTZ_BYTEARRAYAPPEND_OK != QTZ_ByteArray_Append(buffer, '0')) {
      return QTZ_FMTSIZET_BUFFER_NOT_LARGE_ENOUGH;
    }

    return QTZ_FMTSIZET_OK;
  } else {
    size_t buffer_length = QTZ_DigitQuantity(n);
    size_t remaining_space = buffer->capacity - buffer->length;
    if (remaining_space < buffer_length) {
      return QTZ_FMTSIZET_BUFFER_NOT_LARGE_ENOUGH;
    }

    for (size_t i = 0; i < buffer_length; i++) {
      size_t zeros = buffer_length - i - 1;
      uint8_t character = '?';
      if (zeros > 0) {
        size_t divider = pow(10, zeros);
        size_t digit = n / divider;
        character = digit + '0';

        n -= digit * divider;
      } else {
        character = n + '0';
      }

      if (character < '0' ||
          character > '9') { // Something went wrong with string conversion!
        character = '?';
      }

      if (QTZ_BYTEARRAYAPPEND_OK != QTZ_ByteArray_Append(buffer, character)) {
        return QTZ_FMTSIZET_BUFFER_NOT_LARGE_ENOUGH;
      }
    }

    return QTZ_FMTSIZET_OK;
  }
}
