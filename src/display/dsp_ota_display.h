#ifndef DSP_OTA_DISPLAY_H
#define DSP_OTA_DISPLAY_H

/**
 * @file dsp_ota_display.h
 * @brief Full-screen OTA progress UI for TFT displays (optimized for BLUETFT 160x128).
 */

#include "config/firmware_config.h"
#include "globals/globals.h"

#if defined(BLUETFT)
#include "display/dsp_dashboard.h"
#endif

static char dspOtaTargetVersion[16] = "";
static size_t dspOtaTotalBytes = 0;
static uint8_t dspOtaLastPercent = 255;
static uint32_t dspOtaLastDrawMs = 0;

/**
 * @brief Extract the version label from a FIRMWARE string (e.g. "v2.22 2026-06-15").
 *
 * @param firmware Full firmware string
 * @param out Output buffer
 * @param outSize Size of out
 */
inline void dspOtaExtractVersionLabel(const char *firmware, char *out, size_t outSize) {
  size_t i = 0;
  size_t j = 0;

  if (!out || outSize == 0) {
    return;
  }

  out[0] = '\0';
  if (!firmware) {
    return;
  }

  while (firmware[i] != '\0' && firmware[i] != ' ' && j + 1 < outSize) {
    out[j++] = firmware[i++];
  }
  out[j] = '\0';
}

/**
 * @brief Format a byte count for display (B, KB, or MB).
 *
 * @param bytes Byte count
 * @param out Output buffer
 * @param outSize Size of out
 */
inline void dspOtaFormatBytes(size_t bytes, char *out, size_t outSize) {
  if (!out || outSize == 0) {
    return;
  }

  if (bytes >= 1048576UL) {
    snprintf(out, outSize, "%.1f MB", (double)bytes / 1048576.0);
  } else if (bytes >= 1024UL) {
    snprintf(out, outSize, "%u KB", (unsigned)(bytes / 1024UL));
  } else {
    snprintf(out, outSize, "%u B", (unsigned)bytes);
  }
}

/**
 * @brief Estimate OTA duration from firmware size (download + flash write).
 *
 * @param bytes Firmware size in bytes
 * @return uint32_t Estimated seconds
 */
inline uint32_t dspOtaEstimateSeconds(size_t bytes) {
  uint32_t sec;

  if (bytes == 0) {
    return 90;
  }

  sec = (uint32_t)((bytes / 1024UL) / 50UL);
  if (sec < 20U) {
    sec = 20U;
  }
  sec += 25U;
  return sec;
}

/**
 * @brief Format ETA seconds for display.
 *
 * @param seconds Estimated duration
 * @param out Output buffer
 * @param outSize Size of out
 */
inline void dspOtaFormatEta(uint32_t seconds, char *out, size_t outSize) {
  if (!out || outSize == 0) {
    return;
  }

  if (seconds < 60U) {
    snprintf(out, outSize, "~%u sec", (unsigned)seconds);
  } else {
    snprintf(out, outSize, "~%u min", (unsigned)((seconds + 59U) / 60U));
  }
}

#if defined(BLUETFT)

#define OTA_UI_W 160U
#define OTA_UI_H 128U
#define OTA_UI_HEADER_H 18U
#define OTA_UI_BAR_X 8U
#define OTA_UI_BAR_Y 88U
#define OTA_UI_BAR_W 144U
#define OTA_UI_BAR_H 10U
#define OTA_UI_HDR_BG 0x1082U
#define OTA_UI_LINE 0x39E7U
#define OTA_UI_BAR_BG 0x2104U
#define OTA_UI_BAR_FG 0x07E0U
#define OTA_UI_WARN 0xFD20U

/**
 * @brief Draw static OTA screen chrome (header and warning strip).
 */
inline void dspOtaDrawChrome(void) {
  if (!tft) {
    return;
  }

  dsp_erase();
  dsp_fillRect(0, 0, OTA_UI_W, OTA_UI_HEADER_H, OTA_UI_HDR_BG);
  dsp_fillRect(0, OTA_UI_HEADER_H - 1, OTA_UI_W, 1, OTA_UI_LINE);
  dsp_setTextColor(YELLOW);
  dsp_setTextSize(1);
  dsp_setCursor(24, 5);
  dsp_print("FIRMWARE UPDATE");

  dsp_fillRect(0, 116, OTA_UI_W, 12, OTA_UI_HDR_BG);
  dsp_setTextColor(OTA_UI_WARN);
  dsp_setCursor(6, 119);
  dsp_print("Do not power off!");
}

/**
 * @brief Draw one text line in the OTA content area.
 *
 * @param y Y position
 * @param color Text color
 * @param text Line text
 */
inline void dspOtaDrawLine(uint16_t y, uint16_t color, const char *text) {
  if (!tft || !text) {
    return;
  }

  dsp_fillRect(4, y, OTA_UI_W - 8, 10, BLACK);
  dsp_setTextColor(color);
  dsp_setTextSize(1);
  dsp_setCursor(6, y);
  dsp_print(text);
}

/**
 * @brief Draw the horizontal progress bar and percentage.
 *
 * @param percent Progress 0-100
 */
inline void dspOtaDrawBar(uint8_t percent) {
  uint16_t fillW;

  if (!tft) {
    return;
  }

  if (percent > 100U) {
    percent = 100U;
  }

  fillW = map(percent, 0, 100, 0, OTA_UI_BAR_W);
  dsp_fillRect(OTA_UI_BAR_X, OTA_UI_BAR_Y, OTA_UI_BAR_W, OTA_UI_BAR_H, OTA_UI_BAR_BG);
  if (fillW > 0U) {
    dsp_fillRect(OTA_UI_BAR_X, OTA_UI_BAR_Y, fillW, OTA_UI_BAR_H, OTA_UI_BAR_FG);
  }

  char pct[6];
  snprintf(pct, sizeof(pct), "%u%%", (unsigned)percent);
  dsp_fillRect(OTA_UI_BAR_X + OTA_UI_BAR_W - 30, OTA_UI_BAR_Y - 10, 30, 8, BLACK);
  dsp_setTextColor(WHITE);
  dsp_setCursor(OTA_UI_BAR_X + OTA_UI_BAR_W - 28, OTA_UI_BAR_Y - 9);
  dsp_print(pct);
}

#endif /* BLUETFT */

/**
 * @brief Show the OTA start screen with version and size information.
 *
 * @param targetVersion Target firmware version label (may be empty)
 * @param currentFirmware Current on-device FIRMWARE string
 * @param totalBytes Expected download size, or 0 if unknown
 */
inline void dspOtaDisplayBegin(const char *targetVersion, const char *currentFirmware, size_t totalBytes) {
  char currentVer[16];
  char sizeLine[24];
  char etaLine[20];
  char versionLine[40];

  if (!tft) {
    return;
  }

  enableBacklight(true);
  dspOtaTotalBytes = totalBytes;
  dspOtaLastPercent = 255;
  dspOtaLastDrawMs = 0;
  dspOtaTargetVersion[0] = '\0';

  if (targetVersion && targetVersion[0] != '\0') {
    strncpy(dspOtaTargetVersion, targetVersion, sizeof(dspOtaTargetVersion) - 1);
    dspOtaTargetVersion[sizeof(dspOtaTargetVersion) - 1] = '\0';
  }

  dspOtaExtractVersionLabel(currentFirmware, currentVer, sizeof(currentVer));

#if defined(BLUETFT)
  dspOtaDrawChrome();

  if (dspOtaTargetVersion[0] != '\0') {
    snprintf(versionLine, sizeof(versionLine), "%s  ->  %s", currentVer, dspOtaTargetVersion);
  } else {
    snprintf(versionLine, sizeof(versionLine), "Current: %s", currentVer);
  }
  dspOtaDrawLine(22, WHITE, versionLine);

  if (totalBytes > 0) {
    char sizeVal[16];
    dspOtaFormatBytes(totalBytes, sizeVal, sizeof(sizeVal));
    snprintf(sizeLine, sizeof(sizeLine), "Size: %s", sizeVal);
    dspOtaFormatEta(dspOtaEstimateSeconds(totalBytes), etaLine, sizeof(etaLine));
    dspOtaDrawLine(34, CYAN, sizeLine);
    snprintf(etaLine, sizeof(etaLine), "Est. time: %s", etaLine);
    dspOtaDrawLine(46, CYAN, etaLine);
  } else {
    dspOtaDrawLine(34, CYAN, "Size: detecting...");
    dspOtaDrawLine(46, CYAN, "Est. time: ~2 min");
  }

  dspOtaDrawLine(60, GREEN, "Starting update...");
  dspOtaDrawBar(0);
  dspOtaDrawLine(102, WHITE, "Please wait");
#else
  char verLine[40];
  dsp_erase();
  dsp_setTextSize(1);
  dsp_setTextColor(YELLOW);
  dsp_setCursor(0, 0);
  dsp_println("FIRMWARE UPDATE");
  dsp_setTextColor(WHITE);
  if (dspOtaTargetVersion[0] != '\0') {
    snprintf(verLine, sizeof(verLine), "%s -> %s", currentVer, dspOtaTargetVersion);
  } else {
    snprintf(verLine, sizeof(verLine), "Current: %s", currentVer);
  }
  dsp_setCursor(0, 12);
  dsp_println(verLine);
  if (totalBytes > 0) {
    char line[32];
    char sizeVal[16];
    dspOtaFormatBytes(totalBytes, sizeVal, sizeof(sizeVal));
    snprintf(line, sizeof(line), "Size: %s", sizeVal);
    dsp_setCursor(0, 24);
    dsp_println(line);
  }
  dsp_setTextColor(RED);
  dsp_setCursor(0, 100);
  dsp_println("Do not power off!");
#endif
}

/**
 * @brief Update total byte count once known (e.g. after HTTP headers).
 *
 * @param totalBytes Firmware size in bytes
 */
inline void dspOtaDisplaySetTotalBytes(size_t totalBytes) {
  char sizeLine[28];
  char etaLine[28];

  if (!tft || totalBytes == 0) {
    return;
  }

  dspOtaTotalBytes = totalBytes;

#if defined(BLUETFT)
  char sizeVal[16];
  dspOtaFormatBytes(totalBytes, sizeVal, sizeof(sizeVal));
  snprintf(sizeLine, sizeof(sizeLine), "Size: %s", sizeVal);
  dspOtaDrawLine(34, CYAN, sizeLine);
  dspOtaFormatEta(dspOtaEstimateSeconds(totalBytes), etaLine, sizeof(etaLine));
  snprintf(etaLine, sizeof(etaLine), "Est. time: %s", etaLine);
  dspOtaDrawLine(46, CYAN, etaLine);
#endif
}

/**
 * @brief Show the current OTA phase label.
 *
 * @param phaseLabel Short phase description (e.g. "Connecting...")
 */
inline void dspOtaDisplayPhase(const char *phaseLabel) {
  if (!tft || !phaseLabel) {
    return;
  }

#if defined(BLUETFT)
  dspOtaDrawLine(60, GREEN, phaseLabel);
#else
  dsp_setTextColor(GREEN);
  dsp_setCursor(0, 40);
  dsp_println(phaseLabel);
#endif
}

/**
 * @brief Update download/install progress on the display.
 *
 * @param done Bytes written so far
 * @param total Total bytes (0 = unknown)
 * @param phaseLabel Current phase label
 */
inline void dspOtaDisplayProgress(size_t done, size_t total, const char *phaseLabel) {
  char detail[36];
  uint8_t percent = 0;
  uint32_t now = millis();

  if (!tft) {
    return;
  }

  if (total > 0U) {
    percent = (uint8_t)min(100UL, (done * 100UL) / total);
  } else if (dspOtaTotalBytes > 0U) {
    total = dspOtaTotalBytes;
    percent = (uint8_t)min(100UL, (done * 100UL) / total);
  }

  if (percent == dspOtaLastPercent && (now - dspOtaLastDrawMs) < 350U) {
    return;
  }

  dspOtaLastPercent = percent;
  dspOtaLastDrawMs = now;

#if defined(BLUETFT)
  if (phaseLabel) {
    dspOtaDrawLine(60, GREEN, phaseLabel);
  }

  dspOtaDrawBar(percent);

  if (total > 0U) {
    char doneStr[12];
    char totalStr[12];
    dspOtaFormatBytes(done, doneStr, sizeof(doneStr));
    dspOtaFormatBytes(total, totalStr, sizeof(totalStr));
    snprintf(detail, sizeof(detail), "%s / %s", doneStr, totalStr);
  } else {
    dspOtaFormatBytes(done, detail, sizeof(detail));
  }
  dspOtaDrawLine(102, WHITE, detail);
#else
  char line[40];
  snprintf(line, sizeof(line), "%s %u%%", phaseLabel ? phaseLabel : "Progress", (unsigned)percent);
  dsp_setTextColor(WHITE);
  dsp_setCursor(0, 52);
  dsp_println(line);
#endif
}

/**
 * @brief Show a failure message on the OTA screen.
 *
 * @param reason Short error description
 */
inline void dspOtaDisplayFailed(const char *reason) {
  if (!tft) {
    return;
  }

#if defined(BLUETFT)
  dspOtaDrawChrome();
  dspOtaDrawLine(40, RED, "Update failed");
  if (reason && reason[0] != '\0') {
    dspOtaDrawLine(54, WHITE, reason);
  }
  dspOtaDrawLine(68, CYAN, "Radio continues");
  dspOtaDrawLine(80, CYAN, "on current firmware");
#else
  dsp_erase();
  dsp_setTextColor(RED);
  dsp_setCursor(0, 20);
  dsp_println("Update failed");
  if (reason) {
    dsp_setTextColor(WHITE);
    dsp_setCursor(0, 36);
    dsp_println(reason);
  }
#endif
}

/**
 * @brief Show success screen before reboot.
 */
inline void dspOtaDisplaySuccess(void) {
  if (!tft) {
    return;
  }

#if defined(BLUETFT)
  dspOtaDrawChrome();
  if (dspOtaTargetVersion[0] != '\0') {
    char line[32];
    snprintf(line, sizeof(line), "Installed %s", dspOtaTargetVersion);
    dspOtaDrawLine(44, GREEN, line);
  } else {
    dspOtaDrawLine(44, GREEN, "Update complete");
  }
  dspOtaDrawBar(100);
  dspOtaDrawLine(68, WHITE, "Rebooting...");
  dspOtaDrawLine(80, CYAN, "Please wait");
#else
  dsp_erase();
  dsp_setTextColor(GREEN);
  dsp_setCursor(0, 30);
  dsp_println("Update complete");
  dsp_setTextColor(WHITE);
  dsp_setCursor(0, 46);
  dsp_println("Rebooting...");
#endif
}

/**
 * @brief Show informational message (no update needed, etc.).
 *
 * @param title Title line
 * @param detail Detail line
 */
inline void dspOtaDisplayInfo(const char *title, const char *detail) {
  if (!tft) {
    return;
  }

#if defined(BLUETFT)
  dspOtaDrawChrome();
  if (title) {
    dspOtaDrawLine(44, WHITE, title);
  }
  if (detail) {
    dspOtaDrawLine(58, CYAN, detail);
  }
#else
  dsp_erase();
  if (title) {
    dsp_println(title);
  }
  if (detail) {
    dsp_println(detail);
  }
#endif
}

/**
 * @brief Restore normal dashboard after aborted OTA (BLUETFT only).
 */
inline void dspOtaDisplayRestoreDashboard(void) {
#if defined(BLUETFT)
  if (tft) {
    dsp_erase();
    dspDashboardDrawChrome();
    dspDashboardInvalidateTitle();
    dspDashboardInvalidateStation();
  }
#endif
}

#endif /* DSP_OTA_DISPLAY_H */
