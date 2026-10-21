#ifndef ST7701_BACKLIGHT_H
#define ST7701_BACKLIGHT_H

/**
 * @file st7701_backlight.h
 * @brief ISR-safe backlight/screensaver requests for ST7701 2.8B (GPIO6 PWM).
 */

#include <Arduino.h>

#if defined(ST7701_28B)

/** 0=idle, 1=full UI brightness, 2=screensaver (dim + clock). */
extern volatile uint8_t st7701BacklightPending;
extern bool st7701ScreensaverActiveFlag;
/** Set when leaving screensaver; spftask does full erase + dashboard redraw. */
extern volatile bool st7701DashboardRestorePending;

inline void IRAM_ATTR st7701RequestBacklight(bool fullUi) {
  st7701BacklightPending = fullUi ? 1 : 2;
}

inline bool st7701IsScreensaverActive(void) {
  return st7701ScreensaverActiveFlag;
}

void st7701ProcessBacklightPending(void);

#endif

#endif
