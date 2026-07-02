/**
 * @file wifi_display.h
 * @brief Refresh WiFi status on the TFT dashboard.
 */

#ifndef WIFI_DISPLAY_H
#define WIFI_DISPLAY_H

#if defined(BLUETFT)
#include "display/dsp_dashboard.h"
#endif

static uint32_t wifiDisplayLastMs = 0;

/**
 * @brief Refresh WiFi SSID and signal strength on the TFT dashboard.
 */
void updateWifiStatusDisplay(void) {
  if (millis() - wifiDisplayLastMs < 5000) {
    return;
  }
  wifiDisplayLastMs = millis();

#if defined(BLUETFT)
  dspDashboardDrawWifiStatus();
#else
  if (datamode != STOPPED) {
    return;
  }

  char line[40];
  int rssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  snprintf(line, sizeof(line), "%s %ddBm", ipaddress.c_str(), rssi);
  showTextOnTFT(0, line);
#endif
}

#endif
