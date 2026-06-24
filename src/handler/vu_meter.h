/**
 * @file vu_meter.h
 * @brief Audio activity meter derived from stream chunk dynamics.
 */

#ifndef VU_METER_H
#define VU_METER_H

#include "features_config.h"

/**
 * @brief Activity meter using per-read chunk size, burst rate, and peak decay.
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

    const uint32_t now = millis();
    if (lastFeedMs_ > 0) {
      const uint32_t delta = now - lastFeedMs_;
      const uint32_t burst = (uint32_t)bytes * 50U / (delta < 5U ? 5U : delta);
      const uint8_t instant = mapBurstToLevel(burst);
      if (instant > peak_) {
        peak_ = instant;
      }
    }
    lastFeedMs_ = now;

    if (bytes < minChunk_) {
      minChunk_ = bytes;
    }
    if (bytes > maxChunk_) {
      maxChunk_ = bytes;
    }

    byteAccum_ += bytes;
    feedCount_++;
  }

  /**
   * @brief Recalculate and optionally render the VU meter on the TFT.
   */
  void update(void) {
    if (!features.vu_meter) {
      if (level_ > 0) {
        level_ = 0;
        if (displaytype == T_BLUETFT || displaytype == T_ILI9341) {
          displayVuMeterOnTFT(0, false);
        }
      }
      return;
    }

    if (millis() - lastCalcMs_ < 80) {
      return;
    }

    lastCalcMs_ = millis();

    uint8_t target = 0;
    if (datamode != STOPPED && feedCount_ > 0) {
      const uint32_t avgChunk = byteAccum_ / feedCount_;
      const uint16_t span = (maxChunk_ > minChunk_) ? (maxChunk_ - minChunk_) : 0;

      uint8_t base = mapChunkToLevel((uint16_t)avgChunk);
      uint8_t swing = (uint8_t)constrain((int32_t)map((int32_t)span, 0, 1400, 0, 40), 0, 40);
      uint8_t peakLevel = mapChunkToLevel(maxChunk_);

      target = base;
      if (swing > target) {
        target = swing;
      }
      if (peakLevel > target) {
        target = peakLevel;
      }
      if (peak_ > target) {
        target = peak_;
      }

      target = (uint8_t)constrain((int32_t)target, 0, 100);
    }

    feedCount_ = 0;
    byteAccum_ = 0;
    minChunk_ = 65535;
    maxChunk_ = 0;

    if (target > level_) {
      level_ = target;
    } else if (level_ > target + 2) {
      level_ -= 3;
    } else if (level_ > 0) {
      level_--;
    } else {
      level_ = target;
    }

    if (peak_ > 0) {
      if (peak_ > 2) {
        peak_ -= 2;
      } else {
        peak_ = 0;
      }
    }

    if (displaytype == T_BLUETFT || displaytype == T_ILI9341) {
      const bool visible = (datamode != STOPPED && level_ > 0);
      displayVuMeterOnTFT(level_, visible);
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
  static uint8_t mapChunkToLevel(uint16_t chunkBytes) {
    return (uint8_t)constrain((int32_t)map((int32_t)chunkBytes, 64, 1800, 8, 98), 0, 100);
  }

  static uint8_t mapBurstToLevel(uint32_t burst) {
    return (uint8_t)constrain((int32_t)map((int32_t)burst, 400, 28000, 12, 100), 0, 100);
  }

  uint8_t level_ = 0;
  uint8_t peak_ = 0;
  uint32_t byteAccum_ = 0;
  uint16_t feedCount_ = 0;
  uint16_t minChunk_ = 65535;
  uint16_t maxChunk_ = 0;
  uint32_t lastCalcMs_ = 0;
  uint32_t lastFeedMs_ = 0;
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
