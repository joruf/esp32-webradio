#ifndef DSP_COMMON_H
#define DSP_COMMON_H

#include "globals/types.h"

/**
 * @brief Draw battery charge bar above the first screen segment divider.
 *
 * @param segments Screen segment layout (segment 1 y-position is used)
 * @param screenWidth Display width in pixels
 * @param displayReady True when the TFT driver is initialized
 */
inline void dspCommonBatteryBar(scrseg_struct *segments, uint16_t screenWidth, bool displayReady) {
  if (!displayReady) {
    return;
  }
  if (ini_block.bat0 >= ini_block.bat100) {
    return;
  }

  static uint16_t previousBarWidth = 0;
  uint16_t clampedAdc = constrain(adcval, ini_block.bat0, ini_block.bat100);
  uint16_t barWidth = map(clampedAdc, ini_block.bat0, ini_block.bat100, 0, screenWidth);

  if (barWidth == previousBarWidth) {
    return;
  }

  previousBarWidth = barWidth;
  uint16_t ypos = segments[1].y - 5;
  dsp_fillRect(0, ypos, barWidth, 2, GREEN);
  dsp_fillRect(barWidth, ypos, screenWidth - barWidth, 2, RED);
}

/**
 * @brief Draw volume indicator bar along the bottom edge of the display.
 *
 * @param screenWidth Display width in pixels
 * @param screenHeight Display height in pixels
 * @param volume Current volume level (0–100)
 * @param displayReady True when the TFT driver is initialized
 */
inline void dspCommonVolumeBar(uint16_t screenWidth, uint16_t screenHeight, uint8_t volume, bool displayReady) {
  if (!displayReady) {
    return;
  }

  static uint8_t previousVolume = 0;
  if (volume == previousVolume) {
    return;
  }

  previousVolume = volume;
  uint16_t pos = map(volume, 0, 100, 0, screenWidth);
  dsp_fillRect(0, screenHeight - 2, pos, 2, RED);
  dsp_fillRect(pos, screenHeight - 2, screenWidth - pos, 2, GREEN);
}

/**
 * @brief Draw audio activity bar between the top line and the first segment divider.
 *
 * @param screenWidth Display width in pixels
 * @param level VU level 0-100
 * @param displayReady True when the TFT driver is initialized
 * @param visible When false, clears the bar area
 */
inline void dspCommonVuMeterBar(uint16_t screenWidth, uint8_t level, bool displayReady, bool visible) {
  if (!displayReady) {
    return;
  }

  const uint16_t y = 9;
  const uint16_t h = 2;
  static uint8_t previousLevel = 255;

  if (!visible) {
    if (previousLevel == 0) {
      return;
    }
    previousLevel = 0;
    dsp_fillRect(0, y, screenWidth, h, BLACK);
    return;
  }

  if (level == previousLevel) {
    return;
  }

  previousLevel = level;
  uint16_t pos = map(level, 0, 100, 0, screenWidth);
  if (pos > screenWidth) {
    pos = screenWidth;
  }

  dsp_fillRect(0, y, pos, h, YELLOW);
  if (pos < screenWidth) {
    dsp_fillRect(pos, y, screenWidth - pos, h, BLACK);
  }
}

#endif
