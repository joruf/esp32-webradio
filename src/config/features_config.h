/**
 * @file features_config.h
 * @brief Runtime feature flags (dark mode, VU meter, display rotation, stream watchdog, OTA URL).
 */

#ifndef FEATURES_CONFIG_H
#define FEATURES_CONFIG_H

#include <ArduinoJson.h>

#include "util/value_steps.h"

struct FeatureSettings {
  uint8_t dark_start;
  uint8_t dark_end;
  bool vu_meter;
  /** Rotate TFT landscape output by 180 degrees (cable/mount orientation). */
  bool display_rotate_180;
  bool stream_watchdog;
  uint16_t alarm_fade_sec;
  /**
   * Offline alarm tone after stream watchdog gives up.
   * IDs: beep | chirp | birds | water | cascade | arcade
   */
  String alarm_fallback_sound;
  /** Seconds between wake-tone cycles while the alarm is still active. */
  uint16_t alarm_fallback_interval_sec;
  String ota_check_url;
};

/**
 * @brief Return true if sound is a supported offline wake tone ID.
 */
inline bool isValidAlarmFallbackSoundId(const String &sound) {
  return sound.equalsIgnoreCase("beep") || sound.equalsIgnoreCase("chirp") ||
         sound.equalsIgnoreCase("birds") || sound.equalsIgnoreCase("water") ||
         sound.equalsIgnoreCase("cascade") || sound.equalsIgnoreCase("arcade");
}

/**
 * @brief Normalize a sound ID; legacy mario/tetris map to arcade, others to beep.
 */
inline String normalizeAlarmFallbackSoundId(const String &sound) {
  String s = sound;
  s.toLowerCase();
  if (s == "mario" || s == "tetris") {
    return String("arcade");
  }
  if (isValidAlarmFallbackSoundId(s)) {
    return s;
  }
  return String("beep");
}

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
    data.display_rotate_180 = false;
    data.stream_watchdog = true;
    data.alarm_fade_sec = 15;
    data.alarm_fallback_sound = "beep";
    data.alarm_fallback_interval_sec = 5;
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
    data.display_rotate_180 = feat["display_rotate_180"] | data.display_rotate_180;
    data.stream_watchdog = feat["stream_watchdog"] | data.stream_watchdog;
    int fadeSec = (int)(feat["alarm_fade_sec"] | data.alarm_fade_sec);
    data.alarm_fade_sec = (uint16_t)snapValueToStep(fadeSec, 5, 5, 600);
    // Do not use `| nullptr`: ArduinoJson resolves that to operator|<nullptr_t>,
    // which always returns null even when the string key is present.
    const char *fallback = feat["alarm_fallback_sound"].as<const char *>();
    if (fallback && strlen(fallback) > 0) {
      data.alarm_fallback_sound = normalizeAlarmFallbackSoundId(String(fallback));
    }
    int intervalSec = (int)(feat["alarm_fallback_interval_sec"] | data.alarm_fallback_interval_sec);
    static const int kIntervals[] = {2, 3, 5, 10, 15, 30, 60};
    int best = 5;
    int bestDiff = 9999;
    for (unsigned i = 0; i < sizeof(kIntervals) / sizeof(kIntervals[0]); i++) {
      int d = abs(intervalSec - kIntervals[i]);
      if (d < bestDiff) {
        bestDiff = d;
        best = kIntervals[i];
      }
    }
    data.alarm_fallback_interval_sec = (uint16_t)best;
    const char *url = feat["ota_check_url"].as<const char *>();
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
    // Prefer as<> over to<> so an existing features object is not cleared
    // (clearing orphans string allocations in the JsonDocument pool).
    JsonObject feat = doc["features"].is<JsonObject>() ? doc["features"].as<JsonObject>()
                                                       : doc["features"].to<JsonObject>();
    feat["dark_start"] = data.dark_start;
    feat["dark_end"] = data.dark_end;
    feat["vu_meter"] = data.vu_meter;
    feat["display_rotate_180"] = data.display_rotate_180;
    feat["stream_watchdog"] = data.stream_watchdog;
    feat["alarm_fade_sec"] = data.alarm_fade_sec;
    feat["alarm_fallback_sound"] = data.alarm_fallback_sound;
    feat["alarm_fallback_interval_sec"] = data.alarm_fallback_interval_sec;
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

/** Runtime feature flags (avoid macro name `features`, clashes with ESP-IDF). */
#define radioFeatures (featuresConfig.data)

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
