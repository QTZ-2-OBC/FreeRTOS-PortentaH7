#include "../../deps/unity/unity.h"
#include "../debug.c"
#include <string.h>

void setUp() {}
void tearDown() {}

// Stub implementation
void QTZ_Debug_Print() {}

void test_QTZ_Format() {
  QTZ_Debug_Log("VALUE IS: %d", 25);
  char *expected = "debug_test.c:12 [LOG] VALUE IS: 25";
  TEST_ASSERT_EQUAL_STRING_LEN(expected, __DEBUG_INNER_BUFFER,
                               strlen(expected));

  QTZ_Debug_Warning("THE ARR IS: %s", "hola!");
  expected = "debug_test.c:17 [WAR] THE ARR IS: hola!";
  TEST_ASSERT_EQUAL_STRING_LEN(expected, __DEBUG_INNER_BUFFER,
                               strlen(expected));

  QTZ_Debug_Error("The length is: %lu!", 512);
  expected = "debug_test.c:22 [ERR] The length is: 512!";
  TEST_ASSERT_EQUAL_STRING_LEN(expected, __DEBUG_INNER_BUFFER,
                               strlen(expected));
}

#define QTZ_LONG_STRING                                                        \
  "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus quis "   \
  "odio et dolor ornare convallis in hendrerit leo. Phasellus nisi sem, "      \
  "porttitor nec mauris eget, commodo laoreet felis. Nunc nec dapibus risus, " \
  "vel convallis nisl. Pellentesque dignissim accumsan libero at hendrerit. "  \
  "Aenean congue non sem dictum elementum. Cras nec enim dignissim risus "     \
  "sodales hendrerit sed et purus. Class aptent taciti sociosqu ad litora "    \
  "torquent per conubia nostra, per inceptos himenaeos. Nam eleifend sapien "  \
  "sed augue bibendum faucibus. Suspendisse dapibus accumsan massa id "        \
  "finibus. Nunc posuere ipsum feugiat dolor molestie, ac sollicitudin enim "  \
  "aliquam. Nulla imperdiet odio magna, non tempus neque luctus at. Maecenas " \
  "interdum viverra massa, sed dignissim nibh aliquet elementum. Aenean "      \
  "cursus neque vitae metus interdum, ac consectetur lacus ullamcorper. "      \
  "Proin auctor sem nec tincidunt consequat. Mauris eleifend, turpis sed "     \
  "tempus ullamcorper, nibh diam lacinia arcu, aliquam accumsan lacus purus "  \
  "id metus. Suspendisse vitae gravida ex, vel pretium nibh. Sed sed porta "   \
  "elit, sed pretium diam. Nam lacinia, nisl accumsan rutrum imperdiet, "      \
  "risus augue vulputate odio, eget maximus velit magna vel turpis. Duis "     \
  "tristique, eros quis ultricies fringilla, purus neque maximus mauris, ut "  \
  "consectetur ex nunc ac risus. Proin sapien tortor, iaculis ut nunc at, "    \
  "vestibulum molestie est. Mauris."

void test_QTZ_Format_Length() {
  QTZ_Debug_Log("The file contents are: %s", QTZ_LONG_STRING);
  char *expected =
      "debug_test.c:51 [LOG] The file contents are: " QTZ_LONG_STRING;
  TEST_ASSERT_EQUAL_STRING_LEN(expected, __DEBUG_INNER_BUFFER,
                               QTZ_DEBUG_CAPACITY -
                                   1); // -1 because of the null terminator
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_QTZ_Format);
  RUN_TEST(test_QTZ_Format_Length);
  return UNITY_END();
}
