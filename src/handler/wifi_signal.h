/**
 * @file wifi_signal.h
 * @brief WiFi scan cache and signal strength JSON for configured networks.
 */

#ifndef WIFI_SIGNAL_H
#define WIFI_SIGNAL_H

#include <WiFi.h>
#include "title_history.h"

#define WIFI_SIGNAL_CACHE_MS 15000
#define WIFI_SCAN_CACHE_MAX 32
#define WIFI_SCAN_MAX_MS 4000

struct WifiScanCacheEntry {
  char ssid[33];
  int8_t rssi;
};

static WifiScanCacheEntry wifiScanCache[WIFI_SCAN_CACHE_MAX];
static uint8_t wifiScanCacheCount = 0;
static uint32_t wifiScanCacheMs = 0;

/**
 * @brief Store SSID/RSSI pairs from the current WiFi.scanNetworks() result.
 *
 * @param count Number of networks reported by the last scan
 */
static void wifiSignalStoreScanResults(int count) {
  wifiScanCacheCount = 0;
  wifiScanCacheMs = millis();

  if (count <= 0) {
    return;
  }

  for (int i = 0; i < count && wifiScanCacheCount < WIFI_SCAN_CACHE_MAX; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0 || ssid.length() > 32) {
      continue;
    }

    WifiScanCacheEntry &entry = wifiScanCache[wifiScanCacheCount++];
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
static uint8_t ensureWifiScanResults(bool allowFreshScan) {
  uint32_t now = millis();

  if (wifiScanCacheCount > 0 && (now - wifiScanCacheMs) < WIFI_SIGNAL_CACHE_MS) {
    return wifiScanCacheCount;
  }

  if (!allowFreshScan) {
    return wifiScanCacheCount;
  }

  WiFi.scanDelete();
  int count = WiFi.scanNetworks(false, true, false, WIFI_SCAN_MAX_MS);
  if (count < 0) {
    dbgprint("wifi scan failed: %d", count);
    return wifiScanCacheCount;
  }

  wifiSignalStoreScanResults(count);
  return wifiScanCacheCount;
}

/**
 * @brief Find the strongest RSSI for an SSID in the scan cache.
 *
 * @param target SSID to look up
 * @param found  Set to true when the SSID was found
 * @return int RSSI in dBm, or 0 when not found
 */
static int findCachedRssiForSsid(const char *target, bool &found) {
  found = false;
  int bestRssi = 0;

  if (!target || target[0] == '\0') {
    return 0;
  }

  for (uint8_t i = 0; i < wifiScanCacheCount; i++) {
    if (strcmp(wifiScanCache[i].ssid, target) != 0) {
      continue;
    }

    int rssi = wifiScanCache[i].rssi;
    if (!found || rssi > bestRssi) {
      bestRssi = rssi;
      found = true;
    }
  }

  return bestRssi;
}

/**
 * @brief Map RSSI in dBm to a 0-100 quality percentage.
 *
 * @param rssi Signal strength in dBm
 * @return uint8_t Quality percentage (0 = poor, 100 = excellent)
 */
static uint8_t rssiToQualityPercent(int rssi) {
  if (rssi >= -50) {
    return 100;
  }
  if (rssi <= -100) {
    return 0;
  }
  return (uint8_t)(2 * (rssi + 100));
}

/**
 * @brief Append one configured WiFi network to the JSON networks array.
 *
 * @param networks Target JSON array
 * @param ssid     Configured SSID
 * @param allowFreshScan Whether a new scan may be started for lookup
 */
static void appendWifiSignalEntry(JsonArray &networks, const char *ssid, bool allowFreshScan) {
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
    ensureWifiScanResults(allowFreshScan);
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

/**
 * @brief Build JSON with signal strength for all configured WiFi networks.
 *
 * @param allowFreshScan When false, use cache and live RSSI only (fast path)
 * @return String JSON document
 */
static String buildWifiSignalsJson(bool allowFreshScan = true) {
  uint32_t scanAge = wifiScanCacheCount > 0 ? (millis() - wifiScanCacheMs) : 0;

  JsonDocument doc;
  doc["scan_age_ms"] = scanAge;
  JsonArray networks = doc["networks"].to<JsonArray>();

  if (wifilist.size() > 0) {
    for (size_t j = 0; j < wifilist.size(); j++) {
      appendWifiSignalEntry(networks, wifilist[j].ssid, allowFreshScan);
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
      appendWifiSignalEntry(networks, ssid.c_str(), allowFreshScan);
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
static String appendWifiSignalsToConfigJson(const String &json, bool allowFreshScan) {
  JsonDocument configDoc;
  if (deserializeJson(configDoc, json)) {
    return json;
  }

  JsonDocument signalDoc;
  if (deserializeJson(signalDoc, buildWifiSignalsJson(allowFreshScan))) {
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

#endif
