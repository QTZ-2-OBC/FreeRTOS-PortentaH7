#include "../deps/unity/unity.h"
#include "./common.c"
#include "./strings.c"
#include <math.h>
#include <stdio.h>
#include <string.h>

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_QTZ_LengthAsString(void) {
  for (size_t i = 0; i < 10; i++) {
    TEST_ASSERT_EQUAL_size_t(1, QTZ_LengthAsString(i));
  }
  for (size_t i = 10; i < 100; i++) {
    TEST_ASSERT_EQUAL_size_t(2, QTZ_LengthAsString(i));
  }

  for (size_t i = 3; i < 10; i++) {
    size_t start = (size_t)pow(10, i);
    start += 1721;
    TEST_ASSERT_EQUAL_size_t(i + 1, QTZ_LengthAsString(start));
  }
}

void test_QTZ_FmtSizeT(void) {
  // Setup buffer
  uint8_t buffer[8] = {0};
  QTZ_ByteArray byte_array = {.data = buffer, .length = 8};

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

#define TEST_QTZ_FMTSIZET(number)                                              \
  {                                                                            \
    fprintf(stderr, "Testing: %d\n", number);                                  \
    size_t n = number;                                                         \
    char *expected = STR(number);                                              \
    size_t length = QTZ_LengthAsString(n);                                     \
    QTZ_FMTSIZET_Result result = QTZ_FmtSizeT(n, &byte_array);                 \
    TEST_ASSERT_EQUAL_INT(QTZ_FMTSIZET_OK, result);                            \
    TEST_ASSERT_EQUAL_STRING_LEN(expected, byte_array.data, length);           \
  }

  TEST_QTZ_FMTSIZET(5);
  TEST_QTZ_FMTSIZET(7856);
  TEST_QTZ_FMTSIZET(9867);
  TEST_QTZ_FMTSIZET(12345);
  TEST_QTZ_FMTSIZET(76453);
  TEST_QTZ_FMTSIZET(1893);
  TEST_QTZ_FMTSIZET(7777);
  TEST_QTZ_FMTSIZET(34562819);

  fprintf(stderr, "Testing: Buffer not large enough\n");
  uint8_t backup[8] = {0};
  memcpy(backup, buffer, 8);
  QTZ_FMTSIZET_Result result = QTZ_FmtSizeT(123456789, &byte_array);
  TEST_ASSERT_EQUAL_INT(QTZ_FMTSIZET_BUFFER_NOT_LARGE_ENOUGH, result);
  TEST_ASSERT_EQUAL_MEMORY(backup, buffer, 8);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_QTZ_LengthAsString);
  RUN_TEST(test_QTZ_FmtSizeT);
  return UNITY_END();
}
