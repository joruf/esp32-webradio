#ifndef ST7701_RGB_FLUSH_H
#define ST7701_RGB_FLUSH_H

/**
 * @file st7701_rgb_flush.h
 * @brief PSRAM cache writeback for ST7701 RGB scanout.
 *
 * Do not touch VSYNC/HSYNC/PCLK GPIO here: those pins stay owned by the RGB
 * peripheral after gfx->begin(). Reconfiguring them as GPIO blanks the panel.
 */

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#if defined(ST7701_28B)

/** Full-frame cache writeback (caller composes in PSRAM first). */
inline void st7701FlushRgbFramebuffer(Arduino_GFX *gfx) {
  if (!gfx) {
    return;
  }
  gfx->flush(true);
}

#endif

#endif
