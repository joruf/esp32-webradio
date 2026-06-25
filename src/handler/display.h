#if defined(BLUETFT)
#include "display/dsp_dashboard.h"
#endif

#if defined(BLUETFT)

/**
 * @brief Requests display of the song title on the TFT dashboard.
 *
 * @param str Title text
 * @note The actual display update happens in dspDashboardRefreshLiveStatus()
 */
void showTitleOnTFT(const char *str) {
  if (str) {
    icystreamtitle = String(str);
  }
  dspDashboardInvalidateTitle();
}

/**
 * @brief Requests display of the song title on the TFT dashboard.
 *
 * @param str Title text
 */
void showTitleOnTFT(String &str) {
  icystreamtitle = str;
  dspDashboardInvalidateTitle();
}

/**
 * @brief Requests display of the station name on the TFT dashboard.
 *
 * @param str Station name text
 */
void showStationNameOnTFT(const char *str) {
  dspDashboardSetStationName(str);
}

/**
 * @brief Requests display of the station name on the TFT dashboard.
 *
 * @param str Station name text
 */
void showStationNameOnTFT(String &str) {
  dspDashboardSetStationName(str.c_str());
}

/**
 * @brief Requests display of short status text above the WiFi row.
 *
 * @param str Status text
 */
void showShortStatusOnTFT(const char *str) {
  dspDashboardSetShortStatus(str);
}

/**
 * @brief Requests display of short status text above the WiFi row.
 *
 * @param str Status text
 */
void showShortStatusOnTFT(String &str) {
  dspDashboardSetShortStatus(str.c_str());
}

/**
 * @brief Stores legacy top-line text (not shown on the BLUETFT dashboard).
 *
 * @param str Legacy text
 */
void showLegacyOnTFT(const char *str) {
  dspDashboardSetLegacy(str);
}

/**
 * @brief Stores legacy top-line text (not shown on the BLUETFT dashboard).
 *
 * @param str Legacy text
 */
void showLegacyOnTFT(String &str) {
  dspDashboardSetLegacy(str.c_str());
}

#endif /* BLUETFT */

/**
 * @brief Requests display of a text on the TFT display (char* version).
 *
 * @param inx Index of the TFT segment (0=legacy, 1=title, 2=station, 3=short status)
 * @param str String to display (as char*)
 * @note The actual display update happens asynchronously in handle_tft_txt()
 */
void showTextOnTFT(uint16_t inx, const char *str) {
#if defined(BLUETFT)
  switch (inx) {
  case 0:
    showLegacyOnTFT(str);
    return;
  case 1:
    showTitleOnTFT(str);
    return;
  case 2:
    showStationNameOnTFT(str);
    return;
  case 3:
    showShortStatusOnTFT(str);
    return;
  default:
    return;
  }
#else
  if (inx < TFTSECS) // Segment available on display
  {
    if (str) // String specified?
    {
      tftdata[inx].str = String(str); // set string
    }
    tftdata[inx].update_req = true; // and request flag
  }
#endif
}

/**
 * @brief Requests display of a text on the TFT display (String reference version).
 *
 * @param inx Index of the TFT segment (must be < TFTSECS)
 * @param str String to display (as String reference)
 * @note The actual display update happens asynchronously in handle_tft_txt()
 */
void showTextOnTFT(uint16_t inx, String &str) {
#if defined(BLUETFT)
  switch (inx) {
  case 0:
    showLegacyOnTFT(str);
    return;
  case 1:
    showTitleOnTFT(str);
    return;
  case 2:
    showStationNameOnTFT(str);
    return;
  case 3:
    showShortStatusOnTFT(str);
    return;
  default:
    return;
  }
#else
  if (inx < TFTSECS) // Segment available on display
  {
    tftdata[inx].str = str;         // Set string
    tftdata[inx].update_req = true; // and request flag
  }
#endif
}

/**
 * @brief Turns the TFT backlight on or off.
 *
 * Supports both normal (tft_bl_pin) and inverted (tft_blx_pin) logic.
 * Can be called from interrupt context (IRAM_ATTR).
 *
 * @param enable true = backlight ON, false = OFF
 * @note When enabling, the bltimer (backlight timeout counter) is reset.
 */
void IRAM_ATTR enableBacklight(bool enable) {
  if (ini_block.tft_bl_pin >= 0) {              // Backlight for TFT control?
    digitalWrite(ini_block.tft_bl_pin, enable); // Enable/disable backlight
  }

  if (ini_block.tft_blx_pin >= 0) {               // Backlight for TFT (inversed logic) control?
    digitalWrite(ini_block.tft_blx_pin, !enable); // Enable/disable backlight
  }

  if (enable) {
    bltimer = 0; // Reset counter backlight time-out
  }
}

/**
 * @brief Writes a log message to the TFT display if configured.
 *
 * @param str String to log
 */
void tftlog(const char *str) {
  if (tft) // TFT configured?
  {
    dsp_println(str); // show error on TFT
    dsp_update();     // To physical screen
  }
}

/**
 * @brief Clears the background area for a TFT segment.
 *
 * @param p Pointer to the segment to display
 * @param width Width of the area to clear
 */
static void clearSegmentBackground(scrseg_struct *p, uint16_t width) { dsp_fillRect(0, p->y, width, p->height, BLACK); }

/**
 * @brief Draws a divider line above a segment (if space permits and desired).
 *
 * @param p Pointer to the segment to display
 * @param width Width of the divider line
 */
static void drawDividerAboveSegment(scrseg_struct *p, uint16_t width) {
  if ((dsp_getheight() > 64) && (p->y > 1)) {
    dsp_fillRect(0, p->y - 4, width, 1, GREEN); // show divider above text
  }
}

/**
 * @brief Converts and draws the text of a segment on the display.
 *
 * @param p Pointer to the segment to display
 */
static void drawSegmentText(scrseg_struct *p) {
  uint16_t len = p->str.length(); // Required length of buffer
  if (len++) {
    char buf[len];                // Need some buffer space
    p->str.toCharArray(buf, len); // Make a local copy of the string
    utf8ascii(buf);               // Convert possible UTF8
    dsp_setTextColor(p->color);   // Set the requested color
    dsp_setCursor(0, p->y);       // Prepare to show the info
    dsp_println(buf);             // Show the string
  }
}

/**
 * @brief Displays a string on the LCD at a specified y-position.
 *
 * The position is determined by the index in tftdata[] (0 = top line,
 * 1..TFTSECS-1 = bottom lines). The top line is automatically shortened
 * by TIMEPOS characters to make room for the clock display.
 *
 * @param inx Index in tftdata[] (must be < TFTSECS)
 * @note Assumes that 'tft' (display handle) is valid.
 */
void renderInfo(uint16_t inx) {
#if defined(BLUETFT)
  if (!tft || inx >= TFTSECS || inx == 0) {
    return;
  }

  switch (inx) {
  case 1:
    dspDashboardRefreshTitle();
    break;
  case 2:
    dspDashboardRefreshStationBitrate();
    break;
  case 3:
    dspDashboardRefreshShortStatus();
    break;
  default:
    break;
  }
#else
  uint16_t width = dsp_getwidth();
  scrseg_struct *p = &tftdata[inx];

  if (inx == 0) {
    width += TIMEPOS;
  }

  if (tft) {
    clearSegmentBackground(p, width);
    drawDividerAboveSegment(p, width);
    drawSegmentText(p);
  }
#endif
}

/**
 * @brief Processes pending TFT text update requests.
 *
 * Checks all segments for a set update_req flag. When a request is found,
 * the corresponding segment is updated. Only one request is processed per call.
 *
 * @return true  if an update was performed
 * @return false if no pending request was found
 * @note This function should be called regularly in the main loop.
 */
bool handle_tft_txt() {
#if defined(BLUETFT)
  return dspDashboardProcessTextUpdates();
#else
  bool updated = false;

  for (uint16_t i = 0; i < TFTSECS; i++) {
    if (tftdata[i].update_req) {
      renderInfo(i);
      tftdata[i].update_req = false;
      updated = true;
    }
  }

  return updated;
#endif
}
