#ifndef QTZ_LIB
#define QTZ_LIB

#include <stddef.h>
#include <stdint.h>


// Holds an array of bytes
typedef struct {
	// The pointer to the first element of the array.
  uint8_t *data;
	// The number of bytes this array contains with information.
	size_t length;
	// The number of bytes the data can hold.
  size_t capacity;
} QTZ_ByteArray;

// Initializes an array with the specified length, the capacity will equal the length.
void QTZ_ByteArray_Init(QTZ_ByteArray *self, uint8_t *data, size_t capacity);
// Initializes an array with the specified length and capacity.
void QTZ_ByteArray_InitWithLength(QTZ_ByteArray *self, uint8_t *data,  size_t capacity, size_t length);

// Resets the length of of the array back to 0.
//
// This doesn't free any memory!
void QTZ_ByteArray_Reset(QTZ_ByteArray *self);

typedef enum {
	QTZ_BYTEARRAYGET_OK,
	QTZ_BYTEARRAYGET_INDEX_OUT_OF_BOUNDS,
}QTZ_BYTEARRAYGET_Result ;

// Attempt to get a byte from the array.
QTZ_BYTEARRAYGET_Result QTZ_ByteArray_Get(QTZ_ByteArray*self, size_t index, uint8_t*out);

typedef enum {
	QTZ_BYTEARRAYSET_OK,
	QTZ_BYTEARRAYSET_INDEX_OUT_OF_BOUNDS,
}QTZ_BYTEARRAYSET_Result ;
// Attempt to get a byte from the array.
QTZ_BYTEARRAYSET_Result QTZ_ByteArray_Set(QTZ_ByteArray*self, size_t index, uint8_t value);

typedef enum {
	QTZ_BYTEARRAYAPPEND_OK,
	QTZ_BYTEARRAYAPPEND_NOT_ENOUGH_SPACE,
} QTZ_BYTEARRAYAPPEND_Result;
// Attempts to append the provided value to the byte array.
QTZ_BYTEARRAYAPPEND_Result QTZ_ByteArray_Append(QTZ_ByteArray*self, uint8_t value);

typedef enum {
	QTZ_BYTEARRAYEXTEND_OK,
	QTZ_BYTEARRAYEXTEND_NOT_ENOUGH_SPACE,
} QTZ_BYTEARRAYEXTEND_Result;
// Attempts to append the provided value to the byte array.
QTZ_BYTEARRAYEXTEND_Result QTZ_ByteArray_Extend(QTZ_ByteArray*self, QTZ_ByteArray*other);


#endif
