#ifndef QTZ_LIB
#define QTZ_LIB

#include <stddef.h>
#include <stdint.h>


// Holds an array of bytes
typedef struct {
	// The pointer to the first element of the array.
  uint8_t *data;
	// The total length of the array.
  size_t length;
} QTZ_ByteArray;


#endif
