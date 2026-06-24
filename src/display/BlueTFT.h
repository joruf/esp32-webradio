// bluetft.h
//
// Separated from the main sketch to allow several display types.
// Includes for various ST7735 displays.  Size is 160 x 128.  Select
// INITR_BLACKTAB for this and set dsp_getwidth() to 160. Works also for the 128
// x 128 version.  Select INITR_144GREENTAB for this and set dsp_getwidth() to
// 128.

#include "St7735Display.h"
#include <Adafruit_ST7735.h>

// Color definitions for the TFT screen (if used)
#define BLACK ST7735_BLACK
#define BLUE ST7735_BLUE
#define RED ST7735_RED
#define GREEN ST7735_GREEN
#define CYAN ST7735_GREEN | ST7735_BLUE
#define MAGENTA ST7735_RED | ST7735_BLUE
#define YELLOW ST7735_RED | ST7735_GREEN
#define WHITE ST7735_WHITE

// Dashboard text segments (content column; chrome drawn by dsp_dashboard.h)
#define TFTSECS 4
scrseg_struct tftdata[TFTSECS] = {
    {false, WHITE, 0, 8, ""},     // 0: legacy / unused on dashboard
    {false, YELLOW, 32, 62, ""},  // 1: song title (wrapped)
    {false, CYAN, 17, 12, ""},    // 2: station name
    {false, GREEN, 97, 8, ""},    // 3: short status (above WiFi row)
};

St7735Display st7735Display;
Adafruit_ST7735 *tft = NULL;

#define dsp_setRotation() tft->setRotation(1)
#define dsp_print(a) tft->print(a)
#define dsp_println(b) tft->println(b)
#define dsp_fillRect(a, b, c, d, e) tft->fillRect(a, b, c, d, e)
#define dsp_setTextSize(a) tft->setTextSize(a)
#define dsp_setTextColor(a) tft->setTextColor(a)
#define dsp_setCursor(a, b) tft->setCursor(a, b)
#define dsp_erase() tft->fillScreen(BLACK)
#define dsp_getwidth() 160
#define dsp_getheight() 128
#define dsp_update()
#define dsp_isUseSPI() true

#include "dsp_common.h"
#include "dsp_dashboard.h"

bool dsp_begin() {
  if (!st7735Display.begin()) {
    return false;
  }
  tft = st7735Display.panel();
  if (tft != NULL) {
    displaytype = T_BLUETFT;
    dsp_erase();
    dspDashboardDrawChrome();
  }
  return (tft != NULL);
}

void displayBatteryChargeOnTFT() {
  if (!tft || ini_block.bat0 >= ini_block.bat100) {
    return;
  }

  static uint16_t previousBarWidth = 0;
  uint16_t clampedAdc = constrain(adcval, ini_block.bat0, ini_block.bat100);
  uint16_t barWidth = map(clampedAdc, ini_block.bat0, ini_block.bat100, 0, DSP_CONTENT_W);

  if (barWidth == previousBarWidth) {
    return;
  }

  previousBarWidth = barWidth;
  dsp_fillRect(DSP_CONTENT_X, 13, barWidth, 1, GREEN);
  dsp_fillRect(DSP_CONTENT_X + barWidth, 13, DSP_CONTENT_W - barWidth, 1, DSP_COL_HEADER_BG);
}

void displayVolumeOnTFT() {
  if (tft) {
    dspDashboardDrawVolume(getOutputVolume());
  }
}

void displayVuMeterOnTFT(uint8_t level, bool visible) {
  if (tft) {
    dspDashboardDrawVu(level, visible);
  }
}

void showTimeOnTFT(const char *str, uint16_t color) {
  (void)color;
  if (!tft) {
    return;
  }
  if (str && str[0] == '\0') {
    dspDashboardInvalidateHeader();
    return;
  }
  dspDashboardDrawHeader(timetxt);
}
