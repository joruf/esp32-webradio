/**
 * @file features_config.h
 * @brief Runtime feature flags (dark mode hours, VU meter, stream watchdog, OTA check URL).
 */

#ifndef FEATURES_CONFIG_H
#define FEATURES_CONFIG_H

#include <ArduinoJson.h>

#include "util/value_steps.h"

struct FeatureSettings {
  uint8_t dark_start;
  uint8_t dark_end;
  bool vu_meter;
  bool stream_watchdog;
  uint16_t alarm_fade_sec;
  String ota_check_url;
};

/**
 * @brief Runtime feature flags loaded from radio configuration JSON.
 */
class FeaturesConfig {
public:
  FeatureSettings data;

  /**
   * @brief Reset all feature fields to firmware defaults.
   */
  void setDefaults(void) {
    data.dark_start = 20;
    data.dark_end = 7;
    data.vu_meter = true;
    data.stream_watchdog = true;
    data.alarm_fade_sec = 15;
    data.ota_check_url = "https://raw.githubusercontent.com/joruf/esp32-webradio/main/ota/version.json";
  }

  /**
   * @brief Apply the optional "features" object from radio configuration JSON.
   *
   * @param doc Parsed radio configuration document
   */
  void applyFrom(JsonDocument &doc) {
    if (!doc["features"].is<JsonObject>()) {
      return;
    }

    JsonObject feat = doc["features"].as<JsonObject>();
    data.dark_start = constrain((int)(feat["dark_start"] | data.dark_start), 0, 23);
    data.dark_end = constrain((int)(feat["dark_end"] | data.dark_end), 0, 23);
    data.vu_meter = feat["vu_meter"] | data.vu_meter;
    data.stream_watchdog = feat["stream_watchdog"] | data.stream_watchdog;
    int fadeSec = (int)(feat["alarm_fade_sec"] | data.alarm_fade_sec);
    data.alarm_fade_sec = (uint16_t)snapValueToStep(fadeSec, 5, 5, 600);
    const char *url = feat["ota_check_url"] | nullptr;
    if (url && strlen(url) > 0) {
      data.ota_check_url = String(url);
    }
  }

  /**
   * @brief Append feature settings to a radio configuration JSON document.
   *
   * @param doc Target document
   */
  void appendTo(JsonDocument &doc) {
    JsonObject feat = doc["features"].to<JsonObject>();
    feat["dark_start"] = data.dark_start;
    feat["dark_end"] = data.dark_end;
    feat["vu_meter"] = data.vu_meter;
    feat["stream_watchdog"] = data.stream_watchdog;
    feat["alarm_fade_sec"] = data.alarm_fade_sec;
    feat["ota_check_url"] = data.ota_check_url;
  }

  /**
   * @brief Return true if local time is inside the configured dark-mode window.
   *
   * @return bool
   */
  bool isDarkModeActive(void) const {
    if (!timeinfo.tm_year) {
      return false;
    }

    const int hour = timeinfo.tm_hour;
    if (data.dark_start == data.dark_end) {
      return false;
    }

    if (data.dark_start < data.dark_end) {
      return hour >= data.dark_start && hour < data.dark_end;
    }

    return hour >= data.dark_start || hour < data.dark_end;
  }
};

FeaturesConfig featuresConfig;

#define features (featuresConfig.data)

inline void setFeatureDefaults(void) {
  featuresConfig.setDefaults();
}

inline void applyFeaturesFromDoc(JsonDocument &doc) {
  featuresConfig.applyFrom(doc);
}

inline void appendFeaturesToDoc(JsonDocument &doc) {
  featuresConfig.appendTo(doc);
}

inline bool isDarkModeActive(void) {
  return featuresConfig.isDarkModeActive();
}

#endif
