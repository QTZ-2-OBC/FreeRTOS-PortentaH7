#include "./include/common.h"
#include <string.h>

void QTZ_ByteArray_Init(QTZ_ByteArray *self, uint8_t *data, size_t capacity) {
  self->data = data;
  self->length = 0;
  self->capacity = capacity;
}

void QTZ_ByteArray_InitWithLength(QTZ_ByteArray *self, uint8_t *data,
                                  size_t capacity, size_t length) {
  self->data = data;
  self->length = length;
  self->capacity = capacity;
}

QTZ_BYTEARRAYGET_Result QTZ_ByteArray_Get(QTZ_ByteArray *self, size_t index,
                                          uint8_t *out) {
  if (index >= self->length) {
    return QTZ_BYTEARRAYGET_INDEX_OUT_OF_BOUNDS;
  }

  *out = self->data[index];
  return QTZ_BYTEARRAYGET_OK;
}

QTZ_BYTEARRAYSET_Result QTZ_ByteArray_Set(QTZ_ByteArray *self, size_t index,
                                          uint8_t value) {
  if (index >= self->length) {
    return QTZ_BYTEARRAYSET_INDEX_OUT_OF_BOUNDS;
  }

  self->data[index] = value;
  return QTZ_BYTEARRAYSET_OK;
}

QTZ_BYTEARRAYAPPEND_Result QTZ_ByteArray_Append(QTZ_ByteArray *self,
                                                uint8_t value) {
  if (self->length > self->capacity) {
    return QTZ_BYTEARRAYAPPEND_NOT_ENOUGH_SPACE;
  }

  self->data[self->length] = value;
  self->length++;
  return QTZ_BYTEARRAYAPPEND_OK;
}

QTZ_BYTEARRAYEXTEND_Result QTZ_ByteArray_Extend(QTZ_ByteArray *self,
                                                QTZ_ByteArray *other) {
  if (self->length + other->length > self->capacity) {
    return QTZ_BYTEARRAYEXTEND_NOT_ENOUGH_SPACE;
  }

  uint8_t *dest = self->data + self->length;
  memcpy(dest, other->data, other->length);

  return QTZ_BYTEARRAYEXTEND_OK;
}

void QTZ_ByteArray_Reset(QTZ_ByteArray *self) { self->length = 0; }
