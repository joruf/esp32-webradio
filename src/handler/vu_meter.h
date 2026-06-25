/**
 * @file vu_meter.h
 * @brief Audio activity meter derived from stream chunk dynamics.
 *
 * Treble reacts to bursts and peaks (fast transients); bass follows sustained
 * average chunk energy (slower). Both levels are shown as separate VU bars.
 */

#ifndef VU_METER_H
#define VU_METER_H

#include "features_config.h"

uint8_t getOutputVolume(void);

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
      if (burst > burstPeak_) {
        burstPeak_ = burst;
      }
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
      if (trebleLevel_ > 0 || bassLevel_ > 0) {
        trebleLevel_ = 0;
        bassLevel_ = 0;
        if (displaytype == T_BLUETFT || displaytype == T_ILI9341) {
          renderVuMeterOnTFT(0, 0, false);
        }
      }
      return;
    }

    if (millis() - lastCalcMs_ < 80) {
      return;
    }

    lastCalcMs_ = millis();

    uint8_t trebleTarget = 0;
    uint8_t bassTarget = 0;

    if (datamode != STOPPED && feedCount_ > 0) {
      const uint32_t avgChunk = byteAccum_ / feedCount_;
      const uint16_t span = (maxChunk_ > minChunk_) ? (maxChunk_ - minChunk_) : 0;

      bassTarget = mapChunkToLevel((uint16_t)avgChunk);
      const uint8_t bassSwing = (uint8_t)constrain((int32_t)map((int32_t)span, 0, 1200, 0, 36), 0, 36);
      if (bassSwing > bassTarget) {
        bassTarget = bassSwing;
      }

      trebleTarget = mapChunkToLevel(maxChunk_);
      if (burstPeak_ > 0) {
        const uint8_t burstLevel = mapBurstToLevel(burstPeak_);
        if (burstLevel > trebleTarget) {
          trebleTarget = burstLevel;
        }
      }
      const uint8_t trebleSwing = (uint8_t)constrain((int32_t)map((int32_t)span, 0, 1200, 0, 65), 0, 65);
      if (trebleSwing > trebleTarget) {
        trebleTarget = trebleSwing;
      }
      if (peak_ > trebleTarget) {
        trebleTarget = peak_;
      }

      trebleTarget = boostVuLevel(trebleTarget);
      bassTarget = boostVuLevel(bassTarget);

      const uint8_t vol = getOutputVolume();
      trebleTarget = scaleVuByVolume(trebleTarget, vol);
      bassTarget = scaleVuByVolume(bassTarget, vol);

      trebleTarget = (uint8_t)constrain((int32_t)trebleTarget, 0, 100);
      bassTarget = (uint8_t)constrain((int32_t)bassTarget, 0, 100);
    }

    feedCount_ = 0;
    byteAccum_ = 0;
    minChunk_ = 65535;
    maxChunk_ = 0;
    burstPeak_ = 0;

    trebleLevel_ = smoothToward(trebleLevel_, trebleTarget, 16, 4);
    bassLevel_ = smoothToward(bassLevel_, bassTarget, 7, 2);

    if (peak_ > 0) {
      if (peak_ > 1) {
        peak_ -= 1;
      } else {
        peak_ = 0;
      }
    }

    if (displaytype == T_BLUETFT || displaytype == T_ILI9341) {
      const bool visible = (datamode != STOPPED && (trebleLevel_ > 0 || bassLevel_ > 0));
      renderVuMeterOnTFT(trebleLevel_, bassLevel_, visible);
    }
  }

  /**
   * @brief Combined VU level (max of treble and bass) for JSON output.
   *
   * @return uint8_t Level 0-100
   */
  uint8_t level(void) const {
    return trebleLevel_ > bassLevel_ ? trebleLevel_ : bassLevel_;
  }

  /**
   * @brief Treble VU level (fast transients).
   *
   * @return uint8_t Level 0-100
   */
  uint8_t trebleLevel(void) const {
    return trebleLevel_;
  }

  /**
   * @brief Bass VU level (sustained energy).
   *
   * @return uint8_t Level 0-100
   */
  uint8_t bassLevel(void) const {
    return bassLevel_;
  }

private:
  static uint8_t smoothToward(uint8_t current, uint8_t target, uint8_t riseStep, uint8_t fallStep) {
    if (target > current) {
      const uint8_t rise = target - current;
      return current + (rise > riseStep ? riseStep : rise);
    }
    if (current > target + fallStep) {
      return current - fallStep;
    }
    if (current > target) {
      return current - 1;
    }
    return target;
  }

  static uint8_t boostVuLevel(uint8_t level) {
    if (level <= 10) {
      return level;
    }

    const uint16_t boosted = 10U + (((uint16_t)level - 10U) * 135U) / 100U;
    return (uint8_t)(boosted > 100U ? 100U : boosted);
  }

  static uint8_t scaleVuByVolume(uint8_t level, uint8_t volume) {
    uint16_t volWeight;
    uint16_t scaled;

    if (level == 0 || volume == 0) {
      return 0;
    }

    volWeight = 10U + ((uint16_t)volume * 90U) / 100U;
    scaled = ((uint16_t)level * volWeight) / 100U;

    if (volume >= 75U && scaled < 100U) {
      scaled += ((uint16_t)volume - 75U) / 4U;
    }

    return (uint8_t)(scaled > 100U ? 100U : scaled);
  }

  static uint8_t mapChunkToLevel(uint16_t chunkBytes) {
    return (uint8_t)constrain((int32_t)map((int32_t)chunkBytes, 40, 1200, 14, 100), 0, 100);
  }

  static uint8_t mapBurstToLevel(uint32_t burst) {
    return (uint8_t)constrain((int32_t)map((int32_t)burst, 200, 18000, 18, 100), 0, 100);
  }

  uint8_t trebleLevel_ = 0;
  uint8_t bassLevel_ = 0;
  uint8_t peak_ = 0;
  uint32_t burstPeak_ = 0;
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

inline uint8_t getVuMeterTrebleLevel(void) {
  return vuMeter.trebleLevel();
}

inline uint8_t getVuMeterBassLevel(void) {
  return vuMeter.bassLevel();
}

#endif
