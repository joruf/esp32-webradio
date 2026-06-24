/**
 * @file wifi_display.h
 * @brief Show IP address and WiFi signal on the TFT periodically.
 */

#ifndef WIFI_DISPLAY_H
#define WIFI_DISPLAY_H

static uint32_t wifiDisplayLastMs = 0;

/**
 * @brief Refresh network info on TFT segment 0 when playback is stopped.
 */
void updateWifiStatusDisplay(void) {
  if (millis() - wifiDisplayLastMs < 10000) {
    return;
  }
  wifiDisplayLastMs = millis();

  if (datamode != STOPPED) {
    return;
  }

  char line[40];
  int rssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  snprintf(line, sizeof(line), "%s %ddBm", ipaddress.c_str(), rssi);
  showTextOnTFT(0, line);
}

#endif
