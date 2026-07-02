/**
 * @file wifi_signal.h
 * @brief WiFi scan cache and signal strength JSON for configured networks.
 */

#ifndef WIFI_SIGNAL_H
#define WIFI_SIGNAL_H

#include <WiFi.h>
#include "audio/title_history.h"

#define WIFI_SIGNAL_CACHE_MS 15000
#define WIFI_SCAN_CACHE_MAX 32
#define WIFI_SCAN_MAX_MS 4000

/**
 * @brief Cached WiFi scan results and JSON export for configured networks.
 */
class WifiSignalCache {
public:
  struct ScanEntry {
    char ssid[33];
    int8_t rssi;
  };

  /**
   * @brief Store SSID/RSSI pairs from the current WiFi.scanNetworks() result.
   *
   * @param count Number of networks reported by the last scan
   */
  void storeScanResults(int count) {
    cacheCount_ = 0;
    cacheMs_ = millis();

    if (count <= 0) {
      return;
    }

    for (int i = 0; i < count && cacheCount_ < WIFI_SCAN_CACHE_MAX; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() == 0 || ssid.length() > 32) {
        continue;
      }

      ScanEntry &entry = cache_[cacheCount_++];
      ssid.toCharArray(entry.ssid, sizeof(entry.ssid));
      entry.rssi = (int8_t)WiFi.RSSI(i);
    }
  }

  /**
   * @brief Run a WiFi scan or reuse cached scan results.
   *
   * @param allowFreshScan When false, only use existing cache (no new scan)
   * @return uint8_t Number of cached networks
   */
  uint8_t ensureScanResults(bool allowFreshScan) {
    uint32_t now = millis();

    if (cacheCount_ > 0 && (now - cacheMs_) < WIFI_SIGNAL_CACHE_MS) {
      return cacheCount_;
    }

    if (!allowFreshScan) {
      return cacheCount_;
    }

    WiFi.scanDelete();
    int count = WiFi.scanNetworks(false, true, false, WIFI_SCAN_MAX_MS);
    if (count < 0) {
      dbgprint("wifi scan failed: %d", count);
      return cacheCount_;
    }

    storeScanResults(count);
    return cacheCount_;
  }

  /**
   * @brief Build JSON with signal strength for all configured WiFi networks.
   *
   * @param allowFreshScan When false, use cache and live RSSI only (fast path)
   * @return String JSON document
   */
  String buildJson(bool allowFreshScan = true) {
    uint32_t scanAge = cacheCount_ > 0 ? (millis() - cacheMs_) : 0;

    JsonDocument doc;
    doc["scan_age_ms"] = scanAge;
    JsonArray networks = doc["networks"].to<JsonArray>();

    if (wifilist.size() > 0) {
      for (size_t j = 0; j < wifilist.size(); j++) {
        appendNetworkEntry(networks, wifilist[j].ssid, allowFreshScan);
      }
    } else {
      char key[10];
      for (uint8_t i = 0; i < 100; i++) {
        sprintf(key, "wifi_%02d", i);
        if (!nvsKeyExists(key)) {
          break;
        }

        String entry = nvsGetValue(key);
        int sep = entry.indexOf('/');
        if (sep <= 0) {
          continue;
        }

        String ssid = entry.substring(0, sep);
        appendNetworkEntry(networks, ssid.c_str(), allowFreshScan);
      }
    }

    String output;
    serializeJson(doc, output);
    return output;
  }

  /**
   * @brief Add wifi_signals to a radio configuration JSON document.
   *
   * @param json Original configuration JSON
   * @param allowFreshScan Whether a new WiFi scan may be started
   * @return String Extended JSON document
   */
  String appendToConfigJson(const String &json, bool allowFreshScan) {
    JsonDocument configDoc;
    if (deserializeJson(configDoc, json)) {
      return json;
    }

    JsonDocument signalDoc;
    if (deserializeJson(signalDoc, buildJson(allowFreshScan))) {
      return json;
    }

    configDoc["wifi_signals"] = signalDoc["networks"];

    String output;
    serializeJson(configDoc, output);
    if (output.length() == 0 || configDoc.overflowed()) {
      return json;
    }

    return output;
  }

private:
  ScanEntry cache_[WIFI_SCAN_CACHE_MAX];
  uint8_t cacheCount_ = 0;
  uint32_t cacheMs_ = 0;

  static uint8_t rssiToQualityPercent(int rssi) {
    if (rssi >= -50) {
      return 100;
    }
    if (rssi <= -100) {
      return 0;
    }
    return (uint8_t)(2 * (rssi + 100));
  }

  int findCachedRssiForSsid(const char *target, bool &found) const {
    found = false;
    int bestRssi = 0;

    if (!target || target[0] == '\0') {
      return 0;
    }

    for (uint8_t i = 0; i < cacheCount_; i++) {
      if (strcmp(cache_[i].ssid, target) != 0) {
        continue;
      }

      int rssi = cache_[i].rssi;
      if (!found || rssi > bestRssi) {
        bestRssi = rssi;
        found = true;
      }
    }

    return bestRssi;
  }

  void appendNetworkEntry(JsonArray &networks, const char *ssid, bool allowFreshScan) {
    if (!ssid || ssid[0] == '\0') {
      return;
    }

    String connectedSsid = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "";
    bool connected = connectedSsid.length() > 0 && connectedSsid == String(ssid);
    bool available = false;
    int rssi = 0;

    if (connected) {
      available = true;
      rssi = WiFi.RSSI();
    } else {
      ensureScanResults(allowFreshScan);
      rssi = findCachedRssiForSsid(ssid, available);
    }

    JsonObject obj = networks.add<JsonObject>();
    obj["ssid"] = ssid;
    obj["available"] = available;
    obj["connected"] = connected;
    if (available) {
      obj["rssi"] = rssi;
      obj["quality"] = rssiToQualityPercent(rssi);
    }
  }
};

WifiSignalCache wifiSignals;

inline void wifiSignalStoreScanResults(int count) {
  wifiSignals.storeScanResults(count);
}

inline String buildWifiSignalsJson(bool allowFreshScan = false) {
  return wifiSignals.buildJson(allowFreshScan);
}

inline String appendWifiSignalsToConfigJson(const String &json, bool allowFreshScan) {
  return wifiSignals.appendToConfigJson(json, allowFreshScan);
}

#endif
