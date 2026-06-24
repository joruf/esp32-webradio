#ifndef DSP_DASHBOARD_H
#define DSP_DASHBOARD_H

/**
 * @file dsp_dashboard.h
 * @brief Modern 160x128 landscape dashboard for BLUETFT (ST7735).
 *
 * Layout (no overflow):
 *  - Header 15px: date left, time right
 *  - Left gutter 12px: vertical volume bar + % label
 *  - Right gutter 12px: vertical VU meter
 *  - Center 136px: station, song title (wrapped), WiFi SSID + signal %
 */

#include "globals/globals.h"
#include "handler/features_config.h"

uint8_t getOutputVolume(void);
uint8_t getVuMeterLevel(void);
void utf8ascii(char *s);

#if defined(BLUETFT)

#define DSP_DASH_W 160U
#define DSP_DASH_H 128U
#define DSP_GUTTER_W 12U
#define DSP_HEADER_H 15U
#define DSP_CONTENT_X DSP_GUTTER_W
#define DSP_CONTENT_W (DSP_DASH_W - (2U * DSP_GUTTER_W))
#define DSP_BAR_TOP 16U
#define DSP_BAR_HEIGHT 98U
#define DSP_BAR_WIDTH 7U
#define DSP_LEFT_BAR_X 2U
#define DSP_RIGHT_BAR_X (DSP_DASH_W - DSP_GUTTER_W + 3U)

#define DSP_COL_HEADER_BG 0x1082U
#define DSP_COL_LINE 0x39E7U
#define DSP_COL_VOL 0x04DFU
#define DSP_COL_VOL_BG 0x2104U
#define DSP_COL_VU 0xFFE0U
#define DSP_COL_VU_BG 0x3186U
#define DSP_COL_WIFI 0x07E0U
#define DSP_COL_WIFI_WEAK 0xFD20U
#define DSP_COL_WIFI_OFF 0x7BEFU
#define DSP_COL_LABEL 0xC618U

static bool dspDashboardChromeReady = false;
static char dspDashboardLastDate[14] = "";
static char dspDashboardLastTime[8] = "";
inline void dspDashboardInvalidateHeader(void) {
  dspDashboardLastDate[0] = '\0';
  dspDashboardLastTime[0] = '\0';
}
static uint8_t dspDashboardLastVol = 255;
static uint8_t dspDashboardLastVu = 255;
static char dspDashboardLastWifi[40] = "";

/**
 * @brief Convert RSSI (dBm) to quality percent (same curve as Web UI).
 *
 * @param rssi WiFi RSSI in dBm
 * @return uint8_t Quality 0-100
 */
inline uint8_t dspDashboardRssiToPercent(int rssi) {
  if (rssi >= -50) {
    return 100;
  }
  if (rssi <= -100) {
    return 0;
  }
  return (uint8_t)(2 * (rssi + 100));
}

/**
 * @brief Truncate UTF-8 ASCII string to max character count for display width.
 *
 * @param dest Output buffer
 * @param destSize Size of dest
 * @param src Source string
 * @param maxChars Maximum characters to copy
 */
inline void dspDashboardTruncate(char *dest, size_t destSize, const char *src, uint8_t maxChars) {
  size_t i = 0;

  if (!dest || destSize == 0) {
    return;
  }

  dest[0] = '\0';
  if (!src) {
    return;
  }

  while (src[i] != '\0' && i < maxChars && i + 1 < destSize) {
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';
}

/**
 * @brief Split timetxt ("Mo 15.06.25 20:30") into date and time parts.
 *
 * @param timetxt Full time string from NTP
 * @param dateOut Date portion output buffer
 * @param dateSize Size of dateOut
 * @param timeOut Time portion output buffer
 * @param timeSize Size of timeOut
 */
inline void dspDashboardSplitTimeText(const char *timetxt, char *dateOut, size_t dateSize, char *timeOut,
                                      size_t timeSize) {
  const char *space = nullptr;
  size_t i = 0;

  if (dateOut && dateSize > 0) {
    dateOut[0] = '\0';
  }
  if (timeOut && timeSize > 0) {
    timeOut[0] = '\0';
  }
  if (!timetxt || timetxt[0] == '\0') {
    return;
  }

  for (i = 0; timetxt[i] != '\0'; i++) {
    if (timetxt[i] == ' ' && i > 3) {
      space = timetxt + i;
      break;
    }
  }

  if (!space) {
    dspDashboardTruncate(dateOut, dateSize, timetxt, 12);
    return;
  }

  dspDashboardTruncate(dateOut, dateSize, timetxt, (uint8_t)(space - timetxt));
  while (space[0] == ' ') {
    space++;
  }
  dspDashboardTruncate(timeOut, timeSize, space, 8);
}

/**
 * @brief Draw static dashboard chrome (header, gutters, separators).
 */
inline void dspDashboardDrawChrome(void) {
  if (!tft) {
    return;
  }

  dsp_fillRect(0, 0, DSP_DASH_W, DSP_HEADER_H, DSP_COL_HEADER_BG);
  dsp_fillRect(0, DSP_HEADER_H - 1, DSP_DASH_W, 1, DSP_COL_LINE);
  dsp_fillRect(DSP_GUTTER_W - 1, DSP_HEADER_H, 1, DSP_DASH_H - DSP_HEADER_H, DSP_COL_LINE);
  dsp_fillRect(DSP_DASH_W - DSP_GUTTER_W, DSP_HEADER_H, 1, DSP_DASH_H - DSP_HEADER_H, DSP_COL_LINE);
  dsp_fillRect(DSP_CONTENT_X, 30, DSP_CONTENT_W, 1, DSP_COL_LINE);
  dsp_fillRect(DSP_CONTENT_X, 96, DSP_CONTENT_W, 1, DSP_COL_LINE);

  dsp_fillRect(DSP_LEFT_BAR_X, DSP_BAR_TOP, DSP_BAR_WIDTH, DSP_BAR_HEIGHT, DSP_COL_VOL_BG);
  dsp_fillRect(DSP_RIGHT_BAR_X, DSP_BAR_TOP, DSP_BAR_WIDTH, DSP_BAR_HEIGHT, DSP_COL_VU_BG);

  dspDashboardChromeReady = true;
}

/**
 * @brief Update header date and time labels.
 *
 * @param timetxt Full formatted clock string
 */
inline void dspDashboardDrawHeader(const char *timetxt) {
  char dateLine[14];
  char timeLine[8];

  if (!tft) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  dspDashboardSplitTimeText(timetxt, dateLine, sizeof(dateLine), timeLine, sizeof(timeLine));

  if (strcmp(dateLine, dspDashboardLastDate) != 0) {
    dsp_fillRect(DSP_CONTENT_X, 0, 90, 14, DSP_COL_HEADER_BG);
    dsp_setTextColor(WHITE);
    dsp_setTextSize(1);
    dsp_setCursor(DSP_CONTENT_X, 3);
    dsp_print(dateLine[0] ? dateLine : "--");
    strncpy(dspDashboardLastDate, dateLine, sizeof(dspDashboardLastDate) - 1);
    dspDashboardLastDate[sizeof(dspDashboardLastDate) - 1] = '\0';
  }

  if (strcmp(timeLine, dspDashboardLastTime) != 0) {
    uint16_t tw = (uint16_t)strlen(timeLine) * 6;
    uint16_t tx = (tw < DSP_DASH_W) ? (DSP_DASH_W - DSP_GUTTER_W - tw - 2) : DSP_CONTENT_X;

    dsp_fillRect(tx, 0, DSP_DASH_W - tx, 14, DSP_COL_HEADER_BG);
    dsp_setTextColor(WHITE);
    dsp_setTextSize(1);
    dsp_setCursor(tx, 3);
    dsp_print(timeLine[0] ? timeLine : "--:--");
    strncpy(dspDashboardLastTime, timeLine, sizeof(dspDashboardLastTime) - 1);
    dspDashboardLastTime[sizeof(dspDashboardLastTime) - 1] = '\0';
  }

  dsp_fillRect(1, 4, 4, 4, (datamode != STOPPED) ? GREEN : DSP_COL_VOL_BG);
}

/**
 * @brief Draw vertical volume bar and percentage in the left gutter.
 *
 * @param volume Volume level 0-100
 */
inline void dspDashboardDrawVolume(uint8_t volume) {
  uint16_t fillH;
  uint16_t fillY;
  char label[6];

  if (!tft) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  if (volume == dspDashboardLastVol) {
    return;
  }

  dspDashboardLastVol = volume;
  fillH = map(volume, 0, 100, 0, DSP_BAR_HEIGHT);
  fillY = DSP_BAR_TOP + DSP_BAR_HEIGHT - fillH;

  dsp_fillRect(DSP_LEFT_BAR_X, DSP_BAR_TOP, DSP_BAR_WIDTH, DSP_BAR_HEIGHT, DSP_COL_VOL_BG);
  if (fillH > 0) {
    dsp_fillRect(DSP_LEFT_BAR_X, fillY, DSP_BAR_WIDTH, fillH, DSP_COL_VOL);
  }

  snprintf(label, sizeof(label), "%u", (unsigned)volume);
  dsp_fillRect(0, 116, DSP_GUTTER_W, 10, BLACK);
  dsp_setTextColor(DSP_COL_VOL);
  dsp_setTextSize(1);
  dsp_setCursor(1, 118);
  dsp_print(label);
}

/**
 * @brief Draw vertical VU activity bar in the right gutter.
 *
 * @param level Activity level 0-100
 * @param visible When false, clear the bar
 */
inline void dspDashboardDrawVu(uint8_t level, bool visible) {
  uint16_t fillH;
  uint16_t fillY;

  if (!tft) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  if (!visible) {
    if (dspDashboardLastVu == 0) {
      return;
    }
    level = 0;
  }

  if (level == dspDashboardLastVu) {
    return;
  }

  dspDashboardLastVu = level;
  fillH = map(level, 0, 100, 0, DSP_BAR_HEIGHT);
  fillY = DSP_BAR_TOP + DSP_BAR_HEIGHT - fillH;

  dsp_fillRect(DSP_RIGHT_BAR_X, DSP_BAR_TOP, DSP_BAR_WIDTH, DSP_BAR_HEIGHT, DSP_COL_VU_BG);
  if (fillH > 0) {
    dsp_fillRect(DSP_RIGHT_BAR_X, fillY, DSP_BAR_WIDTH, fillH, DSP_COL_VU);
  }
}

/**
 * @brief Draw wrapped text inside the content column.
 *
 * @param y Top Y position
 * @param height Maximum height in pixels
 * @param color Text color
 * @param text Text to render
 * @param maxLines Maximum number of 8px lines
 */
inline void dspDashboardDrawWrappedText(uint16_t y, uint16_t height, uint16_t color, const char *text,
                                        uint8_t maxLines) {
  char line[24];
  const uint8_t charsPerLine = (uint8_t)(DSP_CONTENT_W / 6);
  uint8_t lineIndex = 0;
  size_t pos = 0;
  size_t len;

  if (!tft || !text) {
    return;
  }

  dsp_fillRect(DSP_CONTENT_X, y, DSP_CONTENT_W, height, BLACK);
  dsp_setTextColor(color);
  dsp_setTextSize(1);

  len = strlen(text);
  while (text[pos] != '\0' && lineIndex < maxLines) {
    size_t chunk = charsPerLine;

    if (pos + chunk < len) {
      while (chunk > 0 && text[pos + chunk] != ' ') {
        chunk--;
      }
      if (chunk == 0) {
        chunk = charsPerLine;
      }
    } else {
      chunk = len - pos;
    }

    memset(line, 0, sizeof(line));
    memcpy(line, text + pos, chunk);
    while (line[0] == ' ') {
      memmove(line, line + 1, strlen(line));
    }

    dsp_setCursor(DSP_CONTENT_X + 1, y + (lineIndex * 8));
    dsp_println(line);

    pos += chunk;
    while (text[pos] == ' ') {
      pos++;
    }
    lineIndex++;
  }
}

/**
 * @brief Draw WiFi SSID, mini signal bar and quality percent.
 */
inline void dspDashboardDrawWifiStatus(void) {
  char line[40];
  char ssid[20];
  int rssi = 0;
  uint8_t quality = 0;
  uint16_t barColor = DSP_COL_WIFI_OFF;
  uint16_t barW;
  const uint16_t y = 106;

  if (!tft) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  if (WiFi.status() == WL_CONNECTED) {
    String connected = WiFi.SSID();
    dspDashboardTruncate(ssid, sizeof(ssid), connected.c_str(), 14);
    rssi = WiFi.RSSI();
    quality = dspDashboardRssiToPercent(rssi);
    barColor = (quality >= 50) ? DSP_COL_WIFI : DSP_COL_WIFI_WEAK;
    snprintf(line, sizeof(line), "%s %u%%", ssid, (unsigned)quality);
  } else {
    strcpy(ssid, "No WiFi");
    snprintf(line, sizeof(line), "Offline");
  }

  if (strcmp(line, dspDashboardLastWifi) == 0) {
    return;
  }

  strncpy(dspDashboardLastWifi, line, sizeof(dspDashboardLastWifi) - 1);
  dspDashboardLastWifi[sizeof(dspDashboardLastWifi) - 1] = '\0';

  dsp_fillRect(DSP_CONTENT_X, y, DSP_CONTENT_W, 14, BLACK);
  dsp_setTextColor(barColor);
  dsp_setTextSize(1);
  dsp_setCursor(DSP_CONTENT_X + 1, y + 2);
  dsp_print(ssid);

  if (WiFi.status() == WL_CONNECTED) {
    char pct[6];
    barW = map(quality, 0, 100, 0, 28);
    snprintf(pct, sizeof(pct), "%u%%", (unsigned)quality);
    dsp_fillRect(DSP_CONTENT_X + DSP_CONTENT_W - 50, y + 4, 28, 6, DSP_COL_VOL_BG);
    if (barW > 0) {
      dsp_fillRect(DSP_CONTENT_X + DSP_CONTENT_W - 50, y + 4, barW, 6, barColor);
    }
    dsp_setTextColor(barColor);
    dsp_setCursor(DSP_CONTENT_X + DSP_CONTENT_W - 22, y + 2);
    dsp_print(pct);
  }
}

/**
 * @brief Render a text segment into the dashboard content zones.
 *
 * @param inx Segment index (1=title, 2=station, 3=status)
 * @param segment Segment data
 */
inline void dspDashboardDrawSegment(uint16_t inx, scrseg_struct *segment) {
  char buf[150];
  uint16_t len;

  if (!tft || !segment) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  len = segment->str.length();
  if (len == 0) {
    if (inx == 1) {
      dsp_fillRect(DSP_CONTENT_X, 32, DSP_CONTENT_W, 63, BLACK);
    } else if (inx == 2) {
      dsp_fillRect(DSP_CONTENT_X, 17, DSP_CONTENT_W, 12, BLACK);
    } else if (inx == 3) {
      dsp_fillRect(DSP_CONTENT_X, 97, DSP_CONTENT_W, 8, BLACK);
    }
    return;
  }

  if (len >= sizeof(buf)) {
    len = sizeof(buf) - 1;
  }
  segment->str.toCharArray(buf, len + 1);
  utf8ascii(buf);

  if (inx == 1) {
    dspDashboardDrawWrappedText(32, 62, segment->color, buf, 7);
  } else if (inx == 2) {
    dspDashboardDrawWrappedText(17, 12, segment->color, buf, 1);
  } else if (inx == 3) {
    dspDashboardDrawWrappedText(97, 8, segment->color, buf, 1);
  }
}

/**
 * @brief Periodic dashboard refresh (header, WiFi, volume).
 */
inline void dspDashboardTick(void) {
  if (!tft) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  dspDashboardDrawHeader(timetxt);
  dspDashboardDrawVolume(getOutputVolume());
  dspDashboardDrawWifiStatus();

  if (features.vu_meter && datamode != STOPPED) {
    dspDashboardDrawVu(getVuMeterLevel(), true);
  } else {
    dspDashboardDrawVu(0, false);
  }
}

#endif /* BLUETFT */

#endif /* DSP_DASHBOARD_H */
