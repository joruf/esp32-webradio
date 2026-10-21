/**
 * @file St7701_28B.h
 * @brief Display glue for Waveshare ESP32-S3-Touch-LCD-2.8B (ST7701 RGB, landscape 640x480).
 */

#ifndef ST7701_28B_H
#define ST7701_28B_H

#include "St7701Display.h"
#include "st7701_backlight.h"
#include "st7701_rgb_flush.h"
#include <esp_task_wdt.h>

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

#define TFTSECS 4

St7701Display st7701Display;
Arduino_GFX *tft = NULL;

volatile uint8_t st7701BacklightPending = 0;
/** Set from loop() on touch volume swipe; drawn and flushed only from spftask. */
volatile bool st7701UiVolumeDirty = false;
bool st7701ScreensaverActiveFlag = false;
volatile bool st7701DashboardRestorePending = false;

/** Leave physical x=0 and x=639 black; UI draws in logical 0..637 at panel x=1..638. */
#ifndef ST7701_DRAW_X_INSET
#define ST7701_DRAW_X_INSET 1
#endif

/* Landscape: 1 normal (panel ctor), 3 = 180° flip. */
#define dsp_setRotation()                                          \
  do {                                                             \
    if (tft) {                                                     \
      tft->setRotation(radioFeatures.display_rotate_180 ? 3 : 1); \
    }                                                              \
  } while (0)
#define dsp_fillRect(a, b, c, d, e) \
  tft->fillRect((int16_t)((a) + ST7701_DRAW_X_INSET), (b), (c), (d), (e))
#define dsp_setTextSize(a) tft->setTextSize(a)
#define dsp_setTextColor(a) tft->setTextColor(a)
#define dsp_setTextColorFgBg(fg, bg) tft->setTextColor(fg, bg)
#define dsp_setCursor(a, b) tft->setCursor((int16_t)((a) + ST7701_DRAW_X_INSET), (b))
#define dsp_setTextWrap(w) tft->setTextWrap(w)
#define dsp_erase() tft->fillScreen(BLACK)
#define dsp_getwidth() ((uint16_t)DSP_DASH_W)
#define dsp_getheight() ((uint16_t)(tft ? tft->height() : St7701Display::kLogicalH))
#define dsp_update()                                                                       \
  do {                                                                                     \
    if (tft) {                                                                             \
      esp_task_wdt_reset();                                                                \
      tft->setTextWrap(false);                                                             \
      /* Keep 1 px side gutters black so edge pixels are never UI-painted. */             \
      tft->fillRect(0, 0, ST7701_DRAW_X_INSET, tft->height(), BLACK);                      \
      tft->fillRect((int16_t)(tft->width() - ST7701_DRAW_X_INSET), 0, ST7701_DRAW_X_INSET, \
                    tft->height(), BLACK);                                                 \
      st7701FlushRgbFramebuffer(tft);                                                      \
      esp_task_wdt_reset();                                                                \
    }                                                                                      \
  } while (0)
#define dsp_print(a)            \
  do {                          \
    if (tft) {                  \
      tft->setTextWrap(false);  \
      tft->print(a);            \
    }                           \
  } while (0)
#define dsp_println(b)          \
  do {                          \
    if (tft) {                  \
      tft->setTextWrap(false);  \
      tft->println(b);          \
    }                           \
  } while (0)
#define dsp_isUseSPI() false

#include "dsp_common.h"
#include "dsp_dashboard.h"

inline void st7701PresentDashboardFrame(void);

inline void st7701ProcessBacklightPending(void) {
  const uint8_t req = st7701BacklightPending;
  if (req == 0) {
    return;
  }
  st7701BacklightPending = 0;

  if (req == 1) {
    const bool leavingScreensaver = st7701ScreensaverActiveFlag || dspDashboardScreensaver;
    st7701ScreensaverActiveFlag = false;
    st7701Display.setBacklightLevel(100);
    if (leavingScreensaver) {
      dspDashboardSt7701MarkRestoreFromScreensaver();
      st7701DashboardRestorePending = true;
    }
    st7701UiVolumeDirty = true;
    return;
  }

  st7701ScreensaverActiveFlag = true;
  st7701Display.setBacklightLevel(25);
  dspDashboardScreensaver = true;
  dspDashboardLastSaverTime[0] = '\0';
  st7701UiVolumeDirty = true;
}

bool dsp_begin() {
  if (!st7701Display.begin()) {
    return false;
  }
  tft = st7701Display.gfx();
  if (tft != NULL) {
    displaytype = T_BLUETFT;
    dsp_setTextWrap(false);
    dsp_erase();
    dspDashboardDrawChrome();
    dspDashboardDrawFooterIp();
    st7701UiVolumeDirty = true; // First visible frame from spftask (no setup-task flush)
  }
  return (tft != NULL);
}

void renderBatteryChargeOnTFT() {
  (void)adcval;
  if (!tft || ini_block.bat0 >= ini_block.bat100) {
    return;
  }
  st7701UiVolumeDirty = true;
}

void renderVolumeOnTFT() {
  if (tft && !st7701ScreensaverActiveFlag) {
    st7701UiVolumeDirty = true;
  }
}

void renderVuMeterOnTFT(uint8_t treble, uint8_t bass, bool visible) {
  if (tft && !st7701ScreensaverActiveFlag) {
    dspDashboardDrawVu(treble, bass, visible);
  }
}

void renderTimeOnTFT(const char *str, uint16_t color) {
  (void)color;
  if (!tft) {
    return;
  }
  if (str && str[0] == '\0') {
    dspDashboardInvalidateHeader();
    return;
  }
  if (st7701ScreensaverActiveFlag) {
    st7701UiVolumeDirty = true;
    return;
  }
  // Header/time is redrawn on the next spftask full-frame present.
}

/**
 * @brief ST7701 RGB dashboard: one full-frame compose, then a single flush (spftask only).
 */
inline void st7701PresentDashboardFrame(void) {
  if (!tft) {
    return;
  }

  static uint32_t lastLiveDashboardMs = 0;
  static uint32_t lastVuDrawMs = 0;
  static uint32_t lastFlushMs = 0;
  const uint32_t now = millis();
  static constexpr uint32_t kMinFlushIntervalMs = 300UL;
  static constexpr uint32_t kLiveRefreshMs = 3000UL;
  static constexpr uint32_t kVuRefreshMs = 1200UL;

  if (st7701DashboardRestorePending) {
    st7701DashboardRestorePending = false;
    st7701UiVolumeDirty = false;
    dspDashboardSt7701RestoreFromScreensaver();
    if (time_req) {
      time_req = false;
    }
    dsp_update();
    lastFlushMs = now;
    return;
  }

  const bool volDirty = st7701UiVolumeDirty;
  if (volDirty) {
    st7701UiVolumeDirty = false;
  }

  const bool timeDirty = time_req;
  const bool textPending = dspDashboardSt7701HasPendingText();
  const bool refreshLiveFields =
      (datamode == STOPPED) || ((now - lastLiveDashboardMs) >= kLiveRefreshMs) || timeDirty || volDirty;
  if (refreshLiveFields) {
    lastLiveDashboardMs = now;
  }

  const bool drawVuMeters =
      (datamode == STOPPED) ||
      (radioFeatures.vu_meter && datamode != STOPPED &&
       (((now - lastVuDrawMs) >= kVuRefreshMs) || volDirty || timeDirty));
  if (drawVuMeters && datamode != STOPPED) {
    lastVuDrawMs = now;
  }

  const bool needPresent = volDirty || timeDirty || refreshLiveFields || dspDashboardScreensaver ||
                           !dspDashboardChromeReady || textPending;
  if (!needPresent) {
    return;
  }

  const bool urgentFlush =
      volDirty || timeDirty || textPending || !dspDashboardChromeReady || dspDashboardScreensaver;
  if (!urgentFlush && (now - lastFlushMs) < kMinFlushIntervalMs) {
    if (volDirty) {
      st7701UiVolumeDirty = true;
    }
    return;
  }

  if (st7701ScreensaverActiveFlag) {
    dspDashboardDrawScreensaverClock(timetxt);
    dsp_update();
    lastFlushMs = now;
    if (timeDirty) {
      time_req = false;
    }
    return;
  }

  dspDashboardSt7701PresentFrame(refreshLiveFields, drawVuMeters);
  if (timeDirty) {
    time_req = false;
  }
  dsp_update();
  lastFlushMs = now;
}

#endif // ST7701_28B_H
