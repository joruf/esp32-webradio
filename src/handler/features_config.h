/**
 * @file features_config.h
 * @brief Runtime feature flags (dark mode hours, VU meter, stream watchdog, OTA check URL).
 */

#ifndef FEATURES_CONFIG_H
#define FEATURES_CONFIG_H

#include <ArduinoJson.h>

struct FeatureSettings {
  uint8_t dark_start;
  uint8_t dark_end;
  bool vu_meter;
  bool stream_watchdog;
  uint16_t alarm_fade_sec;
  String ota_check_url;
};

FeatureSettings features;

static void setFeatureDefaults(void) {
  features.dark_start = 20;
  features.dark_end = 7;
  features.vu_meter = true;
  features.stream_watchdog = true;
  features.alarm_fade_sec = 90;
  features.ota_check_url = "https://raw.githubusercontent.com/joruf/ESP32-Webradio/main/ota/version.json";
}

/**
 * @brief Apply the optional "features" object from radio configuration JSON.
 *
 * @param doc Parsed radio configuration document
 */
static void applyFeaturesFromDoc(JsonDocument &doc) {
  if (!doc["features"].is<JsonObject>()) {
    return;
  }

  JsonObject feat = doc["features"].as<JsonObject>();
  features.dark_start = constrain((int)(feat["dark_start"] | features.dark_start), 0, 23);
  features.dark_end = constrain((int)(feat["dark_end"] | features.dark_end), 0, 23);
  features.vu_meter = feat["vu_meter"] | features.vu_meter;
  features.stream_watchdog = feat["stream_watchdog"] | features.stream_watchdog;
  features.alarm_fade_sec = constrain((int)(feat["alarm_fade_sec"] | features.alarm_fade_sec), 5, 600);
  const char *url = feat["ota_check_url"] | nullptr;
  if (url && strlen(url) > 0) {
    features.ota_check_url = String(url);
  }
}

/**
 * @brief Append feature settings to a radio configuration JSON document.
 *
 * @param doc Target document
 */
static void appendFeaturesToDoc(JsonDocument &doc) {
  JsonObject feat = doc["features"].to<JsonObject>();
  feat["dark_start"] = features.dark_start;
  feat["dark_end"] = features.dark_end;
  feat["vu_meter"] = features.vu_meter;
  feat["stream_watchdog"] = features.stream_watchdog;
  feat["alarm_fade_sec"] = features.alarm_fade_sec;
  feat["ota_check_url"] = features.ota_check_url;
}

/**
 * @brief Return true if local time is inside the configured dark-mode window.
 */
bool isDarkModeActive(void) {
  if (!timeinfo.tm_year) {
    return false;
  }

  const int hour = timeinfo.tm_hour;
  if (features.dark_start == features.dark_end) {
    return false;
  }

  if (features.dark_start < features.dark_end) {
    return hour >= features.dark_start && hour < features.dark_end;
  }

  return hour >= features.dark_start || hour < features.dark_end;
}

#endif
