#ifndef DSP_DASHBOARD_H
#define DSP_DASHBOARD_H

/**
 * @file dsp_dashboard.h
 * @brief Modern 160x128 landscape dashboard for BLUETFT (ST7735).
 *
 * Layout (no overflow):
 * - Header 15px: date left, time right
 * - Left gutter 12px: vertical volume bar + % label
 * - Right gutter 12px: vertical VU meter
 * - Center 136px: station, song title (wrapped), WiFi SSID + signal %
 */

#include "globals/globals.h"
#include "config/features_config.h"

extern int32_t sleeptimer_ms;
extern int bitrate;
extern int mbitrate;

uint8_t getOutputVolume(void);
uint8_t getVuMeterTrebleLevel(void);
uint8_t getVuMeterBassLevel(void);
void utf8ascii(char *s);

#if defined(BLUETFT)

#  define DSP_DASH_W 160U
#  define DSP_DASH_H 128U
#  define DSP_GUTTER_W 12U
#  define DSP_HEADER_H 15U
#  define DSP_CONTENT_X DSP_GUTTER_W
#  define DSP_CONTENT_W (DSP_DASH_W - (2U * DSP_GUTTER_W))
#  define DSP_BAR_TOP 16U
#  define DSP_BAR_HEIGHT 98U
#  define DSP_BAR_WIDTH 7U
#  define DSP_LEFT_BAR_X 2U
#  define DSP_VU_BAR_W 2U
#  define DSP_VU_GAP 2U
#  define DSP_VU_TREBLE_X (DSP_DASH_W - DSP_GUTTER_W + 2U)
#  define DSP_VU_BASS_X (DSP_VU_TREBLE_X + DSP_VU_BAR_W + DSP_VU_GAP)

#  define DSP_COL_HEADER_BG 0x1082U
#  define DSP_COL_LINE 0x39E7U
#  define DSP_COL_VOL 0x04DFU
#  define DSP_COL_VOL_BG 0x2104U
#  define DSP_COL_VU 0xFFE0U
#  define DSP_COL_VU_BASS 0x07FFU
#  define DSP_COL_VU_BG 0x3186U
#  define DSP_COL_WIFI 0x07E0U
#  define DSP_COL_WIFI_WEAK 0xFD20U
#  define DSP_COL_WIFI_OFF 0x7BEFU
#  define DSP_COL_LABEL 0xC618U
#  define DSP_FONT_W 6U
#  define DSP_TEXT_PAD 3U
#  define DSP_TEXT_X (DSP_CONTENT_X + DSP_TEXT_PAD)
#  define DSP_CHARS_PER_LINE ((DSP_CONTENT_W - DSP_TEXT_PAD - 1U) / DSP_FONT_W)
#  define DSP_STATION_Y 17U
#  define DSP_STATION_ROW_H 12U
#  define DSP_TITLE_Y (DSP_STATION_Y + DSP_STATION_ROW_H + 3U)
#  define DSP_TITLE_ROW_H 12U

static bool dspDashboardChromeReady = false;
static char dspDashboardLastDate[14] = "";
static char dspDashboardLastTime[8] = "";
static char dspDashboardLastHeaderState[24] = "";
inline void dspDashboardInvalidateHeader(void) {
  dspDashboardLastDate[0] = '\0';
  dspDashboardLastTime[0] = '\0';
  dspDashboardLastHeaderState[0] = '\0';
}
static uint8_t dspDashboardLastVol = 255;
static uint8_t dspDashboardLastVuTreble = 255;
static uint8_t dspDashboardLastVuBass = 255;
static char dspDashboardLastWifi[48] = "";
static int32_t dspDashboardLastSnoozeSec = -1;
static char dspDashboardLastStationKey[88] = "";
static char dspDashboardLastTitle[150] = "";
static uint8_t dspDashboardLastTitleLines = 0;
static char dspDashboardLastShortStatus[150] = "";

#  define DSP_DASH_COLOR_LEGACY WHITE
#  define DSP_DASH_COLOR_TITLE YELLOW
#  define DSP_DASH_COLOR_STATION CYAN
#  define DSP_DASH_COLOR_STATUS GREEN

static char dspDashStationText[150] = "";
static char dspDashShortStatusText[150] = "";
static char dspDashLegacyText[64] = "";
static bool dspDashStationUpdateReq = false;
static bool dspDashShortStatusUpdateReq = false;
static bool dspDashLegacyUpdateReq = false;

inline void dspDashboardInvalidateSnooze(void) { dspDashboardLastSnoozeSec = -1; }

inline void dspDashboardInvalidateStation(void) { dspDashboardLastStationKey[0] = '\0'; }

/**
 * @brief Force the song title row to redraw on the next refresh.
 */
inline void dspDashboardInvalidateTitle(void) { dspDashboardLastTitle[0] = '\0'; }

inline void dspDashboardInvalidateShortStatus(void) { dspDashboardLastShortStatus[0] = '\0'; }

/**
 * @brief Store station name and request a TFT redraw.
 *
 * @param station Station name text
 */
inline void dspDashboardSetStationName(const char *station) {
  strncpy(dspDashStationText, station ? station : "", sizeof(dspDashStationText) - 1);
  dspDashStationText[sizeof(dspDashStationText) - 1] = '\0';
  dspDashboardInvalidateStation();
  dspDashStationUpdateReq = true;
}

/**
 * @brief Store short status text and request a TFT redraw.
 *
 * @param status Short status line (above WiFi row)
 */
inline void dspDashboardSetShortStatus(const char *status) {
  strncpy(dspDashShortStatusText, status ? status : "", sizeof(dspDashShortStatusText) - 1);
  dspDashShortStatusText[sizeof(dspDashShortStatusText) - 1] = '\0';
  dspDashboardInvalidateShortStatus();
  dspDashShortStatusUpdateReq = true;
}

/**
 * @brief Store legacy top-line text (not shown on the BLUETFT dashboard).
 *
 * @param text Legacy segment text
 */
inline void dspDashboardSetLegacy(const char *text) {
  strncpy(dspDashLegacyText, text ? text : "", sizeof(dspDashLegacyText) - 1);
  dspDashLegacyText[sizeof(dspDashLegacyText) - 1] = '\0';
  dspDashLegacyUpdateReq = true;
}

/**
 * @brief Normalize raw icy-br or measured values to display kbps (1-999).
 *
 * @param raw Bitrate from header (kbps or bps) or measured kbps
 * @return int Normalized kbps for display, or 0 if invalid
 */
inline int dspDashboardNormalizeBitrateKbps(int raw) {
  if (raw <= 0) {
    return 0;
  }

  if (raw > 1000) {
    raw = (raw + 500) / 1000;
  }

  if (raw > 999) {
    raw = 999;
  }

  return raw;
}

/**
 * @brief Bitrate label for the station row (0 = hidden).
 *
 * @return int kbps to show as e.g. "128k", or 0 while connecting or selecting
 */
inline int dspDashboardGetDisplayBitrateKbps(void) {
  int raw;

  if (enc_menu_mode == PRESET) {
    return 0;
  }

  if (datamode != DATA && datamode != METADATA) {
    return 0;
  }

  raw = mbitrate > 0 ? mbitrate : bitrate;
  return dspDashboardNormalizeBitrateKbps(raw);
}

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
 * @brief Print one line clipped to the content column (no println – avoids x=0 wrap).
 *
 * @param y Top Y position
 * @param color Text color
 * @param text Line text (truncated to fit content width)
 */
inline void dspDashboardPrintContentLine(uint16_t y, uint16_t color, const char *text) {
  char clipped[DSP_CHARS_PER_LINE + 1];

  if (!tft || !text) {
    return;
  }

  dspDashboardTruncate(clipped, sizeof(clipped), text, DSP_CHARS_PER_LINE);
  dsp_setTextColorFgBg(color, BLACK);
  dsp_setTextSize(1);
  dsp_setCursor(DSP_TEXT_X, y);
  dsp_print(clipped);
}

/**
 * @brief Build WiFi status as "SSID -67dBm (87%)" with SSID truncated to fit one line.
 *
 * @param out Output buffer
 * @param outSize Size of out
 * @param ssid WiFi network name
 * @param rssi Signal strength in dBm
 * @param quality Quality percent 0-100
 */
inline void dspDashboardFormatWifiLine(char *out, size_t outSize, const char *ssid, int rssi, uint8_t quality) {
  char suffix[20];
  char ssidPart[DSP_CHARS_PER_LINE + 1];
  size_t suffixLen;
  size_t maxSsidChars;

  if (!out || outSize == 0) {
    return;
  }

  snprintf(suffix, sizeof(suffix), " %ddBm (%u%%)", rssi, (unsigned)quality);
  suffixLen = strlen(suffix);

  if (suffixLen + 1 >= DSP_CHARS_PER_LINE) {
    snprintf(out, outSize, "%ddBm (%u%%)", rssi, (unsigned)quality);
    return;
  }

  maxSsidChars = DSP_CHARS_PER_LINE - suffixLen;
  dspDashboardTruncate(ssidPart, sizeof(ssidPart), ssid, (uint8_t)maxSsidChars);
  snprintf(out, outSize, "%s%s", ssidPart, suffix);
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
  dsp_fillRect(DSP_VU_TREBLE_X, DSP_BAR_TOP, DSP_VU_BAR_W, DSP_BAR_HEIGHT, DSP_COL_VU_BG);
  dsp_fillRect(DSP_VU_BASS_X, DSP_BAR_TOP, DSP_VU_BAR_W, DSP_BAR_HEIGHT, DSP_COL_VU_BG);

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
  char headerState[24];
  const char *dateStr;
  const char *timeStr;
  uint16_t timeX;

  if (!tft) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  dspDashboardSplitTimeText(timetxt, dateLine, sizeof(dateLine), timeLine, sizeof(timeLine));
  dateStr = dateLine[0] ? dateLine : "--";
  timeStr = timeLine[0] ? timeLine : "--:--";
  snprintf(headerState, sizeof(headerState), "%s|%s|%c", dateStr, timeStr, (datamode != STOPPED) ? 'P' : 'S');

  if (strcmp(headerState, dspDashboardLastHeaderState) == 0) {
    return;
  }

  strncpy(dspDashboardLastHeaderState, headerState, sizeof(dspDashboardLastHeaderState) - 1);
  dspDashboardLastHeaderState[sizeof(dspDashboardLastHeaderState) - 1] = '\0';
  strncpy(dspDashboardLastDate, dateStr, sizeof(dspDashboardLastDate) - 1);
  dspDashboardLastDate[sizeof(dspDashboardLastDate) - 1] = '\0';
  strncpy(dspDashboardLastTime, timeStr, sizeof(dspDashboardLastTime) - 1);
  dspDashboardLastTime[sizeof(dspDashboardLastTime) - 1] = '\0';

  dsp_fillRect(0, 0, DSP_GUTTER_W, DSP_HEADER_H, DSP_COL_HEADER_BG);
  dsp_fillRect(DSP_CONTENT_X, 0, DSP_CONTENT_W, DSP_HEADER_H, DSP_COL_HEADER_BG);
  dsp_fillRect(DSP_DASH_W - DSP_GUTTER_W, 0, DSP_GUTTER_W, DSP_HEADER_H, DSP_COL_HEADER_BG);
  dsp_fillRect(0, DSP_HEADER_H - 1, DSP_DASH_W, 1, DSP_COL_LINE);

  dsp_fillRect(3, 5, 4, 4, (datamode != STOPPED) ? GREEN : DSP_COL_HEADER_BG);

  dsp_setTextColor(WHITE);
  dsp_setTextSize(1);
  dsp_setCursor(DSP_TEXT_X, 3);
  dsp_print(dateStr);

  timeX = (uint16_t)(DSP_CONTENT_X + DSP_CONTENT_W - (strlen(timeStr) * DSP_FONT_W) - 1U);
  dsp_setCursor(timeX, 3);
  dsp_print(timeStr);
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
  // The gutter is only wide enough for two digits, so the value 100 spills one
  // digit into the content column. Clear the full three-digit width (otherwise a
  // leftover "0" stays behind when going from 100 to a two-digit value) and
  // restore the vertical divider pixel column wiped by the wider clear.
  dsp_fillRect(0, 116, DSP_FONT_W * 3U + 2U, 10, BLACK);
  dsp_fillRect(DSP_GUTTER_W - 1U, 116, 1, 10, DSP_COL_LINE);
  dsp_setTextColor(DSP_COL_VOL);
  dsp_setTextSize(1);
  dsp_setCursor(1, 118);
  dsp_print(label);
}

/**
 * @brief Draw one vertical 2px VU bar in the right gutter.
 *
 * @param x Left X position
 * @param level Activity level 0-100
 * @param barColor Fill color for the active segment
 */
inline void dspDashboardDrawVuBar(uint16_t x, uint8_t level, uint16_t barColor) {
  uint16_t fillH;
  uint16_t fillY;

  fillH = map(level, 0, 100, 0, DSP_BAR_HEIGHT);
  fillY = DSP_BAR_TOP + DSP_BAR_HEIGHT - fillH;

  dsp_fillRect(x, DSP_BAR_TOP, DSP_VU_BAR_W, DSP_BAR_HEIGHT, DSP_COL_VU_BG);
  if (fillH > 0) {
    dsp_fillRect(x, fillY, DSP_VU_BAR_W, fillH, barColor);
  }
}

/**
 * @brief Draw treble and bass VU bars side by side in the right gutter.
 *
 * @param treble Treble activity level 0-100
 * @param bass Bass activity level 0-100
 * @param visible When false, clear both bars
 */
inline void dspDashboardDrawVu(uint8_t treble, uint8_t bass, bool visible) {
  if (!tft) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  if (!visible) {
    if (dspDashboardLastVuTreble == 0 && dspDashboardLastVuBass == 0) {
      return;
    }
    treble = 0;
    bass = 0;
  }

  if (treble == dspDashboardLastVuTreble && bass == dspDashboardLastVuBass) {
    return;
  }

  dspDashboardLastVuTreble = treble;
  dspDashboardLastVuBass = bass;

  dspDashboardDrawVuBar(DSP_VU_TREBLE_X, treble, DSP_COL_VU);
  dspDashboardDrawVuBar(DSP_VU_BASS_X, bass, DSP_COL_VU_BASS);
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
  char line[DSP_CHARS_PER_LINE + 1];
  uint8_t lineIndex = 0;
  size_t pos = 0;
  size_t len;

  if (!tft || !text) {
    return;
  }

  dsp_fillRect(DSP_CONTENT_X, y, DSP_CONTENT_W, height, BLACK);
  dsp_setTextColorFgBg(color, BLACK);
  dsp_setTextSize(1);

  len = strlen(text);
  while (text[pos] != '\0' && lineIndex < maxLines) {
    size_t chunk = 0;
    size_t scan;

    for (scan = 0; pos + scan < len && scan < DSP_CHARS_PER_LINE; scan++) {
      const char c = text[pos + scan];
      if (c == '\n' || c == '\r') {
        break;
      }
      chunk++;
    }

    if (chunk == 0) {
      if (text[pos] == '\n' || text[pos] == '\r') {
        pos++;
        lineIndex++;
        continue;
      }
      break;
    }

    if (chunk == DSP_CHARS_PER_LINE && pos + chunk < len) {
      size_t breakAt = chunk;
      while (breakAt > 0 && text[pos + breakAt] != ' ') {
        breakAt--;
      }
      if (breakAt > 0) {
        chunk = breakAt;
      }
    }

    memset(line, 0, sizeof(line));
    memcpy(line, text + pos, chunk);
    while (line[0] == ' ') {
      memmove(line, line + 1, strlen(line));
    }

    dspDashboardTruncate(line, sizeof(line), line, DSP_CHARS_PER_LINE);
    dsp_setCursor(DSP_TEXT_X, y + (lineIndex * 8));
    dsp_print(line);

    pos += chunk;
    if (text[pos] == '\n' || text[pos] == '\r') {
      pos++;
    }
    while (text[pos] == ' ') {
      pos++;
    }
    lineIndex++;
  }
}

/**
 * @brief Draw WiFi SSID, RSSI and quality percent on one line.
 */
inline void dspDashboardDrawWifiStatus(void) {
  char line[48];
  char ssidRaw[33];
  int rssi = 0;
  uint8_t quality = 0;
  uint16_t textColor = DSP_COL_WIFI_OFF;
  const uint16_t y = 106;

  if (!tft) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  if (WiFi.status() == WL_CONNECTED) {
    String connected = WiFi.SSID();
    strncpy(ssidRaw, connected.c_str(), sizeof(ssidRaw) - 1);
    ssidRaw[sizeof(ssidRaw) - 1] = '\0';
    rssi = WiFi.RSSI();
    quality = dspDashboardRssiToPercent(rssi);
    textColor = (quality >= 50) ? DSP_COL_WIFI : DSP_COL_WIFI_WEAK;
    dspDashboardFormatWifiLine(line, sizeof(line), ssidRaw, rssi, quality);
  } else {
    snprintf(line, sizeof(line), "No WiFi");
  }

  if (strcmp(line, dspDashboardLastWifi) == 0) {
    return;
  }

  strncpy(dspDashboardLastWifi, line, sizeof(dspDashboardLastWifi) - 1);
  dspDashboardLastWifi[sizeof(dspDashboardLastWifi) - 1] = '\0';

  dsp_fillRect(DSP_CONTENT_X, y, DSP_CONTENT_W, 12, BLACK);
  dspDashboardPrintContentLine(y + 2, textColor, line);
}

/**
 * @brief Draw snooze countdown on the status line (updates every second).
 */
inline void dspDashboardUpdateSnooze(void) {
  char line[24];
  int32_t sec;
  const uint16_t y = 97;
  const uint16_t snoozeColor = 0xF81FU;

  if (!tft) {
    return;
  }

  if (enc_menu_mode == PRESET) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  if (sleeptimer_ms <= 0) {
    if (dspDashboardLastSnoozeSec >= 0) {
      dsp_fillRect(DSP_CONTENT_X, y, DSP_CONTENT_W, 8, BLACK);
      dspDashboardLastSnoozeSec = -1;
    }
    return;
  }

  sec = sleeptimer_ms / 1000;
  if (sec < 0) {
    sec = 0;
  }

  if (sec == dspDashboardLastSnoozeSec) {
    return;
  }

  dspDashboardLastSnoozeSec = sec;

  if (sec >= 3600) {
    snprintf(line, sizeof(line), "Snooze %luh%02lum", (unsigned long)(sec / 3600), (unsigned long)((sec % 3600) / 60));
  } else if (sec >= 60) {
    snprintf(line, sizeof(line), "Snooze %lum%02lus", (unsigned long)(sec / 60), (unsigned long)(sec % 60));
  } else {
    snprintf(line, sizeof(line), "Snooze %lus", (unsigned long)sec);
  }

  dsp_fillRect(DSP_CONTENT_X, y, DSP_CONTENT_W, 8, BLACK);
  dspDashboardPrintContentLine(y, snoozeColor, line);
}

/**
 * @brief Draw station name (left) and bitrate label (right) on one row.
 *
 * @param station Station name text
 * @param color Station name color
 * @param brKbps Bitrate in kbps, or 0 to hide
 */
inline void dspDashboardDrawStationName(const char *station, uint16_t color, int brKbps) {
  char brLabel[6];
  char stationClip[DSP_CHARS_PER_LINE + 1];
  char cacheKey[88];
  uint8_t stationMaxChars;
  const uint16_t y = 17;

  if (!tft) {
    return;
  }

  brLabel[0] = '\0';
  if (brKbps > 0) {
    snprintf(brLabel, sizeof(brLabel), "%dk", brKbps);
  }

  snprintf(cacheKey, sizeof(cacheKey), "%s|%d", station ? station : "", brKbps);
  if (strcmp(cacheKey, dspDashboardLastStationKey) == 0) {
    return;
  }

  strncpy(dspDashboardLastStationKey, cacheKey, sizeof(dspDashboardLastStationKey) - 1);
  dspDashboardLastStationKey[sizeof(dspDashboardLastStationKey) - 1] = '\0';

  stationMaxChars = DSP_CHARS_PER_LINE;
  if (brLabel[0] != '\0') {
    const uint8_t brChars = (uint8_t)strlen(brLabel);
    if (stationMaxChars > brChars + 1U) {
      stationMaxChars = (uint8_t)(DSP_CHARS_PER_LINE - brChars - 1U);
    }
    if (stationMaxChars < 6U) {
      stationMaxChars = 6U;
    }
  }

  dsp_fillRect(DSP_CONTENT_X, y, DSP_CONTENT_W, 12, BLACK);
  dspDashboardTruncate(stationClip, sizeof(stationClip), station ? station : "", stationMaxChars);

  dsp_setTextColor(color);
  dsp_setTextSize(1);
  dsp_setCursor(DSP_TEXT_X, y + 2);
  dsp_print(stationClip);

  if (brLabel[0] != '\0') {
    const uint16_t bx = (uint16_t)(DSP_CONTENT_X + DSP_CONTENT_W - (strlen(brLabel) * DSP_FONT_W) - 1U);
    dsp_setTextColor(DSP_COL_LABEL);
    dsp_setCursor(bx, y + 2);
    dsp_print(brLabel);
  }
}

/**
 * @brief Refresh station row when measured bitrate changes during playback.
 */
inline void dspDashboardRefreshStationBitrate(void) {
  char buf[150];

  if (!tft) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  if (dspDashStationText[0] == '\0') {
    if (dspDashboardLastStationKey[0] != '\0') {
      dsp_fillRect(DSP_CONTENT_X, 17, DSP_CONTENT_W, 12, BLACK);
      dspDashboardLastStationKey[0] = '\0';
    }
    return;
  }

  strncpy(buf, dspDashStationText, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  utf8ascii(buf);

  dspDashboardDrawStationName(buf, DSP_DASH_COLOR_STATION, dspDashboardGetDisplayBitrateKbps());
}

/**
 * @brief Draw song title inside the content column with word-wrapping.
 *
 * @param title Title text
 * @param color Text color
 */
inline void dspDashboardDrawSongTitle(const char *title, uint16_t color) {
  const uint16_t start_y = DSP_TITLE_Y;
  const size_t max_chars_per_line = 22; // Maximum characters allowed per line
  const uint16_t y_offset = 12;         // Adjustable pixel offset for subsequent lines

  if (!tft) {
    return;
  }

  const char *safe_title = title ? title : "";

  if (strcmp(safe_title, dspDashboardLastTitle) == 0) {
    return;
  }

  // Clear the precise bounding box of the PREVIOUS text before modifying states
  if (dspDashboardLastTitleLines > 0) {
    uint16_t clear_h = dspDashboardLastTitleLines * y_offset;
    dsp_fillRect(DSP_CONTENT_X, start_y, DSP_CONTENT_W, clear_h, BLACK);
  } else {
    dsp_fillRect(DSP_CONTENT_X, start_y, DSP_CONTENT_W, DSP_TITLE_ROW_H, BLACK);
  }

  strncpy(dspDashboardLastTitle, safe_title, sizeof(dspDashboardLastTitle) - 1);
  dspDashboardLastTitle[sizeof(dspDashboardLastTitle) - 1] = '\0';

  // If the new title is empty, we are completely done after clearing the screen area
  if (safe_title[0] == '\0') {
    dspDashboardLastTitleLines = 0;
    return;
  }

  dsp_setTextColor(color);
  dsp_setTextSize(1);

  size_t title_len = strlen(safe_title);
  size_t src_index = 0;
  uint16_t current_y = start_y + 2;
  uint8_t current_line_count = 0;

  // Loop until the entire string is processed
  while (src_index < title_len) {
    // Skip any leading spaces at the start of a new line
    while (src_index < title_len && safe_title[src_index] == ' ') {
      src_index++;
    }

    if (src_index >= title_len) {
      break;
    }

    size_t remaining_len = title_len - src_index;
    size_t chunk_len = (remaining_len > max_chars_per_line) ? max_chars_per_line : remaining_len;

    // If the remaining text is longer than the line limit, look for a word boundary
    if (remaining_len > max_chars_per_line) {
      size_t last_space_index = 0;
      bool space_found = false;

      // Search backwards within the current chunk for a space
      for (size_t i = chunk_len; i > 0; i--) {
        if (safe_title[src_index + i - 1] == ' ') {
          last_space_index = i - 1;
          space_found = true;
          break;
        }
      }

      // If a space was found, adjust the chunk length to break at the word boundary
      if (space_found && last_space_index > 0) {
        chunk_len = last_space_index;
      }
    }

    // Safely copy and terminate the line chunk
    char titleClip[max_chars_per_line + 1];
    strncpy(titleClip, safe_title + src_index, chunk_len);
    titleClip[chunk_len] = '\0';

    // Set cursor position and print the current line
    dsp_setCursor(DSP_TEXT_X, current_y);
    dsp_print(titleClip);

    current_line_count++;

    // Prepare variables for the next line
    src_index += chunk_len;
    current_y += y_offset; // Move down for the next iteration
  }

  // Save the amount of rendered lines for the next update cycle
  dspDashboardLastTitleLines = current_line_count;
}

/**
 * @brief Refresh song title row from icystreamtitle.
 */
inline void dspDashboardRefreshTitle(void) {
  char buf[150];
  uint16_t len;

  if (!tft) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  String songTitle = metaGetTitle();
  len = songTitle.length();
  if (len == 0) {
    // Always call draw to process the background clearing if a title was previously on screen
    dspDashboardDrawSongTitle("", DSP_DASH_COLOR_TITLE);
    return;
  }

  if (len >= sizeof(buf)) {
    len = sizeof(buf) - 1;
  }
  songTitle.toCharArray(buf, len + 1);
  utf8ascii(buf);

  dspDashboardDrawSongTitle(buf, DSP_DASH_COLOR_TITLE);
}

/**
 * @brief Draw short status text on the row above the WiFi line.
 *
 * Snooze countdown uses the same row and takes priority when active.
 *
 * @param status Status text
 * @param color Text color
 */
inline void dspDashboardDrawShortStatus(const char *status, uint16_t color) {
  char buf[150];
  const uint16_t y = 97;

  if (!tft) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  if (sleeptimer_ms > 0 && enc_menu_mode != PRESET) {
    return;
  }

  if (!status || status[0] == '\0') {
    if (dspDashboardLastShortStatus[0] != '\0') {
      dsp_fillRect(DSP_CONTENT_X, y, DSP_CONTENT_W, 8, BLACK);
      dspDashboardLastShortStatus[0] = '\0';
    }
    return;
  }

  if (strcmp(status, dspDashboardLastShortStatus) == 0) {
    return;
  }

  strncpy(dspDashboardLastShortStatus, status, sizeof(dspDashboardLastShortStatus) - 1);
  dspDashboardLastShortStatus[sizeof(dspDashboardLastShortStatus) - 1] = '\0';

  strncpy(buf, status, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  utf8ascii(buf);

  dspDashboardDrawWrappedText(y, 8, color, buf, 1);
}

/**
 * @brief Legacy top-line text (unused on the BLUETFT dashboard layout).
 *
 * @param text Legacy text
 * @param color Text color
 */
inline void dspDashboardDrawLegacy(const char *text, uint16_t color) {
  (void)text;
  (void)color;
}

/**
 * @brief Process pending dashboard text update requests (station, short status).
 *
 * @return true if at least one field was redrawn
 */
inline bool dspDashboardProcessTextUpdates(void) {
  bool updated = false;
  char buf[150];

  if (dspDashLegacyUpdateReq) {
    dspDashLegacyUpdateReq = false;
  }

  if (dspDashStationUpdateReq) {
    dspDashStationUpdateReq = false;
    strncpy(buf, dspDashStationText, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    utf8ascii(buf);
    dspDashboardDrawStationName(buf, DSP_DASH_COLOR_STATION, dspDashboardGetDisplayBitrateKbps());
    updated = true;
  }

  if (dspDashShortStatusUpdateReq) {
    dspDashShortStatusUpdateReq = false;
    strncpy(buf, dspDashShortStatusText, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    utf8ascii(buf);
    dspDashboardDrawShortStatus(buf, DSP_DASH_COLOR_STATUS);
    updated = true;
  }

  return updated;
}

/**
 * @brief Redraw short status from the stored dashboard text.
 */
inline void dspDashboardRefreshShortStatus(void) {
  char buf[150];

  if (!tft) {
    return;
  }

  if (!dspDashboardChromeReady) {
    dspDashboardDrawChrome();
  }

  strncpy(buf, dspDashShortStatusText, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  utf8ascii(buf);
  dspDashboardDrawShortStatus(buf, DSP_DASH_COLOR_STATUS);
}

/**
 * @brief Refresh dashboard fields from live globals (same source as buildLiveStatusJson).
 *
 * Song title: icystreamtitle. Station row: dspDashStationText.
 * Call only while the SPI bus is claimed when dsp_isUseSPI().
 */
inline void dspDashboardRefreshLiveStatus(void) {
  if (!tft) {
    return;
  }

  dspDashboardRefreshTitle();
  dspDashboardUpdateSnooze();
  dspDashboardRefreshStationBitrate();
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
  dspDashboardRefreshStationBitrate();
  dspDashboardDrawVolume(getOutputVolume());
  dspDashboardDrawWifiStatus();

  dspDashboardUpdateSnooze();

  if (radioFeatures.vu_meter && datamode != STOPPED) {
    dspDashboardDrawVu(getVuMeterTrebleLevel(), getVuMeterBassLevel(), true);
  } else {
    dspDashboardDrawVu(0, 0, false);
  }
}

#endif /* BLUETFT */

#endif /* DSP_DASHBOARD_H */