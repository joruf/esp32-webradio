/**
 * @file test_volume_mapper.cpp
 * @brief Unit tests for VolumeMapper (external 0-100 to audible chip range).
 */

#include <unity.h>

#include "audio/VolumeMapper.h"

void setUp(void) {}
void tearDown(void) {}

/** Zero maps to the audible floor, not to true silence. */
void test_zero_maps_to_floor(void) {
  TEST_ASSERT_EQUAL_UINT8(VolumeMapper::kChipFloor, VolumeMapper::mapExternalToChip(0));
}

/** Maximum maps to the chip maximum. */
void test_max_maps_to_hundred(void) {
  TEST_ASSERT_EQUAL_UINT8(100, VolumeMapper::mapExternalToChip(100));
}

/** The midpoint maps linearly into [floor, 100]. */
void test_midpoint_is_linear(void) {
  // 40 + 50 * (100 - 40) / 100 = 40 + 30 = 70
  TEST_ASSERT_EQUAL_UINT8(70, VolumeMapper::mapExternalToChip(50));
}

/** The mapping is monotonically non-decreasing across the whole range. */
void test_monotonic(void) {
  uint8_t previous = 0;
  for (int i = 0; i <= 100; i++) {
    uint8_t mapped = VolumeMapper::mapExternalToChip((uint8_t)i);
    TEST_ASSERT_TRUE(mapped >= previous);
    TEST_ASSERT_TRUE(mapped >= VolumeMapper::kChipFloor);
    TEST_ASSERT_TRUE(mapped <= 100);
    previous = mapped;
  }
}

/** Out-of-range input is clamped to 100. */
void test_out_of_range_is_clamped(void) {
  TEST_ASSERT_EQUAL_UINT8(100, VolumeMapper::mapExternalToChip(200));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_zero_maps_to_floor);
  RUN_TEST(test_max_maps_to_hundred);
  RUN_TEST(test_midpoint_is_linear);
  RUN_TEST(test_monotonic);
  RUN_TEST(test_out_of_range_is_clamped);
  return UNITY_END();
}
