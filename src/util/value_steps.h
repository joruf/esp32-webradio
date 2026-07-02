#ifndef VALUE_STEPS_H
#define VALUE_STEPS_H

#include <cstdint>

/**
 * @file value_steps.h
 * @brief Small, hardware-independent numeric helpers.
 *
 * Kept free of Arduino/hardware dependencies so the logic can be exercised by
 * native unit tests as well as the firmware build.
 */

/**
 * @brief Snap a value to the nearest multiple of @p step and clamp it.
 *
 * Rounds @p value to the closest multiple of @p step (round half up) and then
 * constrains the result to the inclusive range [@p minValue, @p maxValue].
 *
 * @param value    Raw input value
 * @param step     Step size (values <= 0 are treated as 1)
 * @param minValue Lower inclusive bound of the result
 * @param maxValue Upper inclusive bound of the result
 * @return int32_t Snapped and clamped value
 */
inline int32_t snapValueToStep(int32_t value, int32_t step, int32_t minValue, int32_t maxValue) {
  if (step <= 0) {
    step = 1;
  }
  if (value < 0) {
    value = 0;
  }

  int32_t snapped = ((value + step / 2) / step) * step;

  if (snapped < minValue) {
    snapped = minValue;
  }
  if (snapped > maxValue) {
    snapped = maxValue;
  }

  return snapped;
}

#endif /* VALUE_STEPS_H */
