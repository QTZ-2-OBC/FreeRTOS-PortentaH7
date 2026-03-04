#include "../../deps/unity/unity.h"
#include "../common.c"
#include <stdint.h>

void setUp() {}
void tearDown() {}

void test_QTZ_ByteArray_Usage() {
  // First we need to initialize a pointer to some memory.
  uint8_t buffer[20] = {0};

  // We need to initialize the byte array with the memory pointer
  // In this case the buffer will be "empty" and have a capacity to store 2
  // bytes.
  QTZ_ByteArray array = {0};
  // NOTE: The array length doesn't need to be the same as the underlying
  // buffer, just less or equal!
  QTZ_ByteArray_Init(&array, buffer, 2);

  // Since we can store 2 bytes, we can append to the end of the array 2 bytes.
  TEST_ASSERT_EQUAL_INT(QTZ_BYTEARRAYAPPEND_OK,
                        QTZ_ByteArray_Append(&array, 125));
  TEST_ASSERT_EQUAL_INT(QTZ_BYTEARRAYAPPEND_OK,
                        QTZ_ByteArray_Append(&array, 234));

  // The array is already full!
  TEST_ASSERT_EQUAL_INT(array.length, array.capacity);

  // Since the array is already full, if we try to append another byte, it'll
  // fail!
  TEST_ASSERT_EQUAL_INT(QTZ_BYTEARRAYAPPEND_NOT_ENOUGH_SPACE,
                        QTZ_ByteArray_Append(&array, 75));

  // And no modifications are done to the underlying data.
  uint8_t expected_buffer[2] = {125, 234};
  TEST_ASSERT_EQUAL_MEMORY(expected_buffer, buffer, 2);

  // If you need to reuse the array, remember to reset it!
  QTZ_ByteArray_Reset(&array);

  // Now we can append other 2 items as if the array was empty!
  TEST_ASSERT_EQUAL_INT(QTZ_BYTEARRAYAPPEND_OK,
                        QTZ_ByteArray_Append(&array, 10));
  TEST_ASSERT_EQUAL_INT(QTZ_BYTEARRAYAPPEND_OK,
                        QTZ_ByteArray_Append(&array, 20));
  expected_buffer[0] = 10;
  expected_buffer[1] = 20;
  TEST_ASSERT_EQUAL_MEMORY(expected_buffer, buffer, 2);

  // What if we want to append all items from an array into our array?
  // We have to "extend" it!
  QTZ_ByteArray array2 = {0};
  // This array will start right after "array" and have 10 bytes of capacity!
  QTZ_ByteArray_Init(&array2, buffer + 2, 3);

  // Copy all bytes from "array" into "array2" only if "array2" has enough
  // capacity.
  TEST_ASSERT_EQUAL_INT(QTZ_BYTEARRAYEXTEND_OK,
                        QTZ_ByteArray_Extend(&array2, &array));
  TEST_ASSERT_EQUAL_INT(2, array2.length);
  // If we try to extend it again, it'll fail because we don't have enough space
  // to store 4 bytes!
  TEST_ASSERT_EQUAL_INT(QTZ_BYTEARRAYEXTEND_NOT_ENOUGH_SPACE,
                        QTZ_ByteArray_Extend(&array2, &array));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_QTZ_ByteArray_Usage);
  return UNITY_END();
}
