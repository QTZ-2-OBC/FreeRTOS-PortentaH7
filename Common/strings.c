#include "include/strings.h"
#include "include/common.h"
#include <math.h>

void QTZ_String_InitWithCapacity(QTZ_String *self, QTZ_ByteArray *bytes,
                                 size_t capacity) {
  self->bytes = bytes;
  self->capacity = capacity;
}

void QTZ_String_Init(QTZ_String *self, QTZ_ByteArray *bytes) {
  self->bytes = bytes;
  self->capacity = bytes->length;
}

size_t QTZ_LengthAsString(size_t n) {
  if (n < 2) { // Handle edge cases... AKA (0, 1)
    return 1;
  } else {
    // This formula only works if n is 2 or more!
    return ((size_t)log10(n)) + 1;
  }
}

typedef enum {
  QTZ_FMTSIZET_OK,
  QTZ_FMTSIZET_BUFFER_NOT_LARGE_ENOUGH,
} QTZ_FMTSIZET_Result;

QTZ_FMTSIZET_Result QTZ_FmtSizeT(size_t n, QTZ_ByteArray *buffer) {
  if (n == 0) {
    if (buffer->length <= 0) {
      return QTZ_FMTSIZET_BUFFER_NOT_LARGE_ENOUGH;
    }

    buffer->data[0] = '0';
    return QTZ_FMTSIZET_OK;
  } else {
    size_t buffer_length = QTZ_LengthAsString(n);
    if (buffer->length < buffer_length) {
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

      buffer->data[i] = character;
    }

    return QTZ_FMTSIZET_OK;
  }
}
