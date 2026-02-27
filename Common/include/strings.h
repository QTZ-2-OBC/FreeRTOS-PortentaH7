#ifndef QTZ_STRINGS_LIB
#define QTZ_STRINGS_LIB

#include "common.h"

// Holds a string data.
typedef struct {
	QTZ_ByteArray *bytes;
	size_t capacity;
}QTZ_String;

// Initializes a QTZ_String with a specified capacity.
void QTZ_String_InitWithCapacity(QTZ_String*self, QTZ_ByteArray *bytes, size_t capacity);

// Initializes a QTZ_String with the same capacity as the array length.
void QTZ_String_Init(QTZ_String*self, QTZ_ByteArray *bytes);

#endif
