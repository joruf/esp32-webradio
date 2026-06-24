/**
 * @file vu_meter.h
 * @brief Simple audio activity meter derived from stream throughput.
 */

#ifndef VU_METER_H
#define VU_METER_H

#include "features_config.h"

static uint8_t vuLevel = 0;
static uint32_t vuByteAccum = 0;
static uint32_t vuLastCalcMs = 0;

/**
 * @brief Feed byte count from the MP3/network loop into the VU calculation.
 *
 * @param bytes Bytes read in the current iteration
 */
void vuMeterFeed(uint16_t bytes) {
  if (!features.vu_meter || bytes == 0) {
    return;
  }
  vuByteAccum += bytes;
}

/**
 * @brief Recalculate and optionally render the VU meter on the TFT.
 */
void vuMeterUpdate(void) {
  if (!features.vu_meter) {
    vuLevel = 0;
    return;
  }

  if (millis() - vuLastCalcMs < 200) {
    return;
  }

  vuLastCalcMs = millis();
  uint32_t rate = vuByteAccum * 5;
  vuByteAccum = 0;

  uint8_t target = 0;
  if (datamode != STOPPED && rate > 0) {
    if (rate > 50000) {
      target = 100;
    } else {
      target = (uint8_t)constrain((int32_t)(rate / 500), 4, 100);
    }
  }

  if (target > vuLevel) {
    vuLevel = target;
  } else if (vuLevel > 5) {
    vuLevel -= 4;
  } else {
    vuLevel = target;
  }

  if (displaytype == T_BLUETFT || displaytype == T_ILI9341) {
    char bar[22];
    uint8_t blocks = vuLevel / 10;
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
uint8_t getVuMeterLevel(void) {
  return vuLevel;
}

#endif
