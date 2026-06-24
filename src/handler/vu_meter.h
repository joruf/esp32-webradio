/**
 * @file vu_meter.h
 * @brief Simple audio activity meter derived from stream throughput.
 */

#ifndef VU_METER_H
#define VU_METER_H

#include "features_config.h"

/**
 * @brief Audio activity meter derived from stream byte throughput.
 */
class VuMeter {
public:
  /**
   * @brief Feed byte count from the MP3/network loop into the VU calculation.
   *
   * @param bytes Bytes read in the current iteration
   */
  inline void feed(uint16_t bytes) {
    if (!features.vu_meter || bytes == 0) {
      return;
    }
    byteAccum_ += bytes;
  }

  /**
   * @brief Recalculate and optionally render the VU meter on the TFT.
   */
  void update(void) {
    if (!features.vu_meter) {
      level_ = 0;
      return;
    }

    if (millis() - lastCalcMs_ < 200) {
      return;
    }

    lastCalcMs_ = millis();
    uint32_t rate = byteAccum_ * 5;
    byteAccum_ = 0;

    uint8_t target = 0;
    if (datamode != STOPPED && rate > 0) {
      if (rate > 50000) {
        target = 100;
      } else {
        target = (uint8_t)constrain((int32_t)(rate / 500), 4, 100);
      }
    }

    if (target > level_) {
      level_ = target;
    } else if (level_ > 5) {
      level_ -= 4;
    } else {
      level_ = target;
    }

    if (displaytype == T_BLUETFT || displaytype == T_ILI9341) {
      char bar[22];
      uint8_t blocks = level_ / 10;
      if (blocks > 10) {
        blocks = 10;
      }
      memset(bar, '.', 10);
      bar[10] = '\0';
      for (uint8_t i = 0; i < blocks; i++) {
        bar[i] = '|';
      }
      showTextOnTFT(0, bar);
    }
  }

  /**
   * @brief Current VU level for JSON output.
   *
   * @return uint8_t Level 0-100
   */
  uint8_t level(void) const {
    return level_;
  }

private:
  uint8_t level_ = 0;
  uint32_t byteAccum_ = 0;
  uint32_t lastCalcMs_ = 0;
};

VuMeter vuMeter;

inline void vuMeterFeed(uint16_t bytes) {
  vuMeter.feed(bytes);
}

inline void vuMeterUpdate(void) {
  vuMeter.update();
}

inline uint8_t getVuMeterLevel(void) {
  return vuMeter.level();
}

#endif
