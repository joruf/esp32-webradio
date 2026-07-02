/**
 * @file test_value_steps.cpp
 * @brief Unit tests for snapValueToStep() (volume and alarm-fade quantization).
 */

#include <unity.h>

#include "util/value_steps.h"

void setUp(void) {}
void tearDown(void) {}

/** Values round to the nearest multiple of the step (round half up). */
void test_rounds_to_nearest_multiple(void) {
  TEST_ASSERT_EQUAL_INT32(50, snapValueToStep(50, 5, 0, 100));
  TEST_ASSERT_EQUAL_INT32(50, snapValueToStep(52, 5, 0, 100));
  TEST_ASSERT_EQUAL_INT32(55, snapValueToStep(53, 5, 0, 100));
  TEST_ASSERT_EQUAL_INT32(55, snapValueToStep(57, 5, 0, 100));
  TEST_ASSERT_EQUAL_INT32(60, snapValueToStep(58, 5, 0, 100));
}

/** Results are clamped to the inclusive [min, max] range. */
void test_clamps_to_bounds(void) {
  TEST_ASSERT_EQUAL_INT32(100, snapValueToStep(200, 5, 0, 100));
  TEST_ASSERT_EQUAL_INT32(0, snapValueToStep(-10, 5, 0, 100));
  TEST_ASSERT_EQUAL_INT32(5, snapValueToStep(1, 5, 5, 600));
  TEST_ASSERT_EQUAL_INT32(600, snapValueToStep(1000, 5, 5, 600));
}

/** The alarm-fade use case: min 5, max 600, step 5. */
void test_alarm_fade_quantization(void) {
  TEST_ASSERT_EQUAL_INT32(15, snapValueToStep(15, 5, 5, 600));
  TEST_ASSERT_EQUAL_INT32(30, snapValueToStep(29, 5, 5, 600));
  TEST_ASSERT_EQUAL_INT32(5, snapValueToStep(3, 5, 5, 600));
  TEST_ASSERT_EQUAL_INT32(60, snapValueToStep(60, 5, 5, 600));
}

/** The volume use case: min 0, max 100, step 5. */
void test_volume_quantization(void) {
  TEST_ASSERT_EQUAL_INT32(0, snapValueToStep(0, 5, 0, 100));
  TEST_ASSERT_EQUAL_INT32(45, snapValueToStep(43, 5, 0, 100));
  TEST_ASSERT_EQUAL_INT32(100, snapValueToStep(98, 5, 0, 100));
}

/** A step of zero is treated as one (no division by zero, no rounding). */
void test_zero_step_is_safe(void) {
  TEST_ASSERT_EQUAL_INT32(42, snapValueToStep(42, 0, 0, 100));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_rounds_to_nearest_multiple);
  RUN_TEST(test_clamps_to_bounds);
  RUN_TEST(test_alarm_fade_quantization);
  RUN_TEST(test_volume_quantization);
  RUN_TEST(test_zero_step_is_safe);
  return UNITY_END();
}
