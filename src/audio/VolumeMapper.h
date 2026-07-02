#ifndef VOLUME_MAPPER_H
#define VOLUME_MAPPER_H

#include <Arduino.h>

/**
 * @file VolumeMapper.h
 * @brief Maps user-facing volume (0-100) to the VS1053 audible range.
 *
 * On the VS1053, values below ~40 are barely audible. This class linearly maps
 * the external 0-100 scale onto the internal kChipFloor-100 range so that the
 * full encoder travel is perceptible.
 */
class VolumeMapper {
public:
  /** Lowest VS1053 level that is still clearly audible. */
  static constexpr uint8_t kChipFloor = 40;

  /**
   * @brief Maps a user-facing volume onto the audible chip range.
   *
   * @param external User-facing volume 0-100
   * @return uint8_t Chip volume in the range kChipFloor-100
   */
  static uint8_t mapExternalToChip(uint8_t external) {
    if (external > 100) {
      external = 100;
    }
    return kChipFloor + (uint16_t)external * (100 - kChipFloor) / 100;
  }
};

#endif /* VOLUME_MAPPER_H */
