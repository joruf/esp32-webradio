#ifndef DSP_STATUS_ICONS_H
#define DSP_STATUS_ICONS_H

/**
 * @file dsp_status_icons.h
 * @brief Small header icons: WiFi link and Web UI traffic (ST7701 dashboard).
 */

#include "globals/globals.h"

#if defined(RADIO_DASHBOARD_TFT)

#if defined(ST7701_28B)
#  define DSP_ICON_WIFI_W 18U
#  define DSP_ICON_WIFI_H 14U
#  define DSP_ICON_COMM_W 14U
#  define DSP_ICON_COMM_H 14U
#  define DSP_ICON_GAP 6U
#else
#  define DSP_ICON_WIFI_W 12U
#  define DSP_ICON_WIFI_H 10U
#  define DSP_ICON_COMM_W 10U
#  define DSP_ICON_COMM_H 10U
#  define DSP_ICON_GAP 4U
#endif

/**
 * @brief Draw a minimal WiFi glyph (arcs + dot), eBike-style header indicator.
 *
 * @param x Left X in header row
 * @param y Top Y in header row
 * @param connected WL_CONNECTED
 * @param qualityPercent 0..100 when connected
 * @param bg Background color under the icon (header chrome)
 */
inline void dspStatusDrawWifiIcon(uint16_t x, uint16_t y, bool connected, uint8_t qualityPercent,
                                  uint16_t bg) {
  if (!tft) {
    return;
  }

  const uint16_t w = DSP_ICON_WIFI_W;
  const uint16_t h = DSP_ICON_WIFI_H;
  dsp_fillRect(x, y, w, h, bg);

  if (!connected) {
    const uint16_t off = 0x7BEFU;
    dsp_fillRect(x + 2, y + h - 3, w - 4, 2, off);
    dsp_fillRect(x + w / 2U - 1, y + 2, 2, h - 4, off);
    return;
  }

  const uint16_t strong = 0x07E0U;
  const uint16_t weak = 0xFD20U;
  const uint16_t col = (qualityPercent >= 50) ? strong : weak;
  const uint8_t bars = (qualityPercent >= 75) ? 3U : (qualityPercent >= 45) ? 2U : 1U;

  dsp_fillRect(x + w / 2U - 1, y + h - 3, 2, 2, col);
  if (bars >= 2) {
    dsp_fillRect(x + 4, y + h - 7, 2, 4, col);
    dsp_fillRect(x + w - 6, y + h - 7, 2, 4, col);
  }
  if (bars >= 3) {
    dsp_fillRect(x + 2, y + h - 11, 2, 3, col);
    dsp_fillRect(x + w - 4, y + h - 11, 2, 3, col);
    dsp_fillRect(x + w / 2U - 1, y + 1, 2, 2, col);
  }
}

/**
 * @brief Draw Web UI traffic indicator (up/down chevrons when active).
 *
 * @param x Left X in header row
 * @param y Top Y in header row
 * @param active Recent HTTP/SSE activity
 * @param bg Background color under the icon
 */
inline void dspStatusDrawWebTrafficIcon(uint16_t x, uint16_t y, bool active, uint16_t bg) {
  if (!tft) {
    return;
  }

  const uint16_t w = DSP_ICON_COMM_W;
  const uint16_t h = DSP_ICON_COMM_H;
  dsp_fillRect(x, y, w, h, bg);

  const uint16_t idle = 0x528AU;
  const uint16_t live = 0x5D1FU;
  const uint16_t col = active ? live : idle;
  const uint16_t cx = x + w / 2U;

  dsp_fillRect(cx - 1, y + 2, 2, 2, col);
  dsp_fillRect(cx - 3, y + 4, 2, 2, col);
  dsp_fillRect(cx + 1, y + 4, 2, 2, col);
  dsp_fillRect(cx - 1, y + h - 4, 2, 2, col);
  dsp_fillRect(cx - 3, y + h - 6, 2, 2, col);
  dsp_fillRect(cx + 1, y + h - 6, 2, 2, col);
}

#endif // RADIO_DASHBOARD_TFT

#endif
