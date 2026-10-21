/**
 * @file radio_config.h
 * @brief Unified radio configuration stored as JSON in NVS.
 */

#ifndef RADIO_CONFIG_H
#define RADIO_CONFIG_H

#include <ArduinoJson.h>
#include "alarm_config.h"
#include "features_config.h"
#include "radio_config_merge.h"
#include "hardware_profile.h"
#include "firmware_config.h"

void applyClockConfiguration(bool restartNtp);

/**
 * @brief Overlay radioFeatures from dedicated feat_* NVS keys (authoritative).
 */
void applyFeatureNvsKeyOverrides(void) {
  if (nvsKeyExists("feat_dark_start")) {
    radioFeatures.dark_start = constrain(nvsGetValue("feat_dark_start").toInt(), 0, 23);
  }
  if (nvsKeyExists("feat_dark_end")) {
    radioFeatures.dark_end = constrain(nvsGetValue("feat_dark_end").toInt(), 0, 23);
  }
  if (nvsKeyExists("feat_vu_meter")) {
    radioFeatures.vu_meter = nvsGetValue("feat_vu_meter").toInt() != 0;
  }
  if (nvsKeyExists("feat_rot180")) {
    radioFeatures.display_rotate_180 = nvsGetValue("feat_rot180").toInt() != 0;
  }
  if (nvsKeyExists("feat_watchdog")) {
    radioFeatures.stream_watchdog = nvsGetValue("feat_watchdog").toInt() != 0;
  }
  if (nvsKeyExists("feat_alarm_fade")) {
    radioFeatures.alarm_fade_sec = constrain(nvsGetValue("feat_alarm_fade").toInt(), 5, 600);
  }
  if (nvsKeyExists("feat_alarm_fb")) {
    radioFeatures.alarm_fallback_sound =
        normalizeAlarmFallbackSoundId(nvsGetValue("feat_alarm_fb"));
  }
  if (nvsKeyExists("feat_fb_int") || nvsKeyExists("feat_alarm_fb_int")) {
    // feat_alarm_fb_int was >15 chars (NVS key limit); prefer short key.
    String raw = nvsKeyExists("feat_fb_int") ? nvsGetValue("feat_fb_int")
                                             : nvsGetValue("feat_alarm_fb_int");
    int intervalSec = raw.toInt();
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
    radioFeatures.alarm_fallback_interval_sec = (uint16_t)best;
  }
  if (nvsKeyExists("feat_ota_url")) {
    String otaUrl = nvsGetValue("feat_ota_url");
    if (otaUrl.length() > 0) {
      radioFeatures.ota_check_url = otaUrl;
    }
  }
}

#define RADIO_CONFIG_MAX_LEN 8192
#define RADIO_CONFIG_NVS_KEY "radio_config"
#define RADIO_CONFIG_VERSION 3
#define RADIO_CONFIG_MASKED_PASSWORD "********"

/**
 * @brief Read the unified radio configuration JSON from NVS.
 *
 * @return String JSON document or empty string if not found
 */
String nvsGetRadioConfig() {
  static char buf[RADIO_CONFIG_MAX_LEN];
  size_t len = RADIO_CONFIG_MAX_LEN;

  nvsOpen();
  buf[0] = '\0';
  nvserr = nvs_get_str(nvshandle, RADIO_CONFIG_NVS_KEY, buf, &len);
  if (nvserr) {
    return String("");
  }
  return String(buf);
}

/**
 * @brief Store the unified radio configuration JSON in NVS.
 *
 * @param json Serialized configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t nvsSetRadioConfig(const String &json) {
  if (json.length() == 0 || json.length() >= RADIO_CONFIG_MAX_LEN) {
    dbgprint("nvsSetRadioConfig length failed!");
    return ESP_ERR_NVS_INVALID_LENGTH;
  }

  nvsOpen();
  nvserr = nvs_set_str(nvshandle, RADIO_CONFIG_NVS_KEY, json.c_str());
  if (nvserr) {
    dbgprint("nvsSetRadioConfig failed!");
  }
  return nvserr;
}

/**
 * @brief Split a preset NVS value into URL and display name.
 *
 * @param val   Raw preset value
 * @param url   Output stream URL
 * @param name  Output display name
 */
static void splitPresetValue(const String &val, String &url, String &name) {
  int inx = val.indexOf('#');

  if (inx > 0) {
    url = val.substring(0, inx);
    name = val.substring(inx + 1);
    url.trim();
    name.trim();
  } else {
    url = val;
    url.trim();
    name = "";
  }
}

/**
 * @brief Build the preset NVS value from URL and display name.
 *
 * @param url   Stream URL
 * @param name  Display name
 * @return String NVS value
 */
static String buildPresetValue(const String &url, const String &name) {
  if (name.length() > 0) {
    return url + String(" # ") + name;
  }
  return url;
}

/**
 * @brief Append factory default radio presets to a JSON array.
 *
 * @param presets Target preset array
 */
static void appendDefaultPresets(JsonArray &presets) {
  struct PresetDefault {
    int slot;
    const char *url;
    const char *name;
  };

  static const PresetDefault defaults[] = {
      {0, "charthits-high.rautemusik.fm", "0 - Rautemusik"},
      {1, "stream.sunshine-live.de/live/mp3-192", "1 - Sunshine Live"},
      {2, "listen.technobase.fm/tunein-mp3", "2 - Technobase.FM (HandsUp & Dance)"},
      {3, "stream.slam.nl/slam_mp3", "3 - SLAM Live"},
      {4, "ic.radiomonster.fm/dance.ultra", "4 - RadioMonster Dance"},
      {5, "ice1.somafm.com/groovesalad-128-mp3", "5 - SomaFM"},
      {6, "s2-webradio.antenne.de/dance-xxl", "6 - Antenne Bayern Dance XXL"},
      {7, "stream.antenne1.de/a1stg/livestream2.mp3", "7 - Antenne 1"},
      {8, "topradio-de-hz-fal-stream03-cluster01.radiohost.de/kissfm_128", "8 - Kiss FM"},
      {9, "streamhq.top100station.com/top100station-high.mp3", "9 - TOP 100 Station"},
      {10, "stream.antenne1.de/80er/livestream2.mp3", "10 - Antenne 1 80ger"},
      {11, "play.antenne.de/dance-xxl.m3u", "11 - Antenne 1 Dance"},
      {12, "liveradio.swr.de/sw331ch/swr3/play.aac", "12 - SWR3"},
  };

  uint16_t i;
  for (i = 0; i < sizeof(defaults) / sizeof(defaults[0]); i++) {
    JsonObject obj = presets.add<JsonObject>();
    obj["slot"] = defaults[i].slot;
    obj["url"] = defaults[i].url;
    obj["name"] = defaults[i].name;
  }
}

/**
 * @brief Append factory default GPIO actions to a JSON array.
 *
 * @param gpio Target GPIO array
 */
static void appendDefaultGpio(JsonArray &gpio) {
  const HardwareProfile *profile = findHardwareProfile(HW_PROFILE_DEFAULT);
  if (profile == NULL) {
    return;
  }
  for (size_t i = 0; i < profile->gpio_count; i++) {
    JsonObject obj = gpio.add<JsonObject>();
    obj["pin"] = profile->gpio[i].pin;
    obj["command"] = profile->gpio[i].command;
  }
}

/**
 * @brief Extract alarms JSON from a full radio configuration document.
 *
 * @param json Full radio configuration JSON
 * @return String Alarm-only JSON document
 */
static String extractAlarmsJson(const String &json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) {
    return String("");
  }

  JsonDocument alarmsDoc;
  alarmsDoc["version"] = doc["version"] | 1;
  alarmsDoc["alarms"].set(doc["alarms"]);

  String output;
  serializeJson(alarmsDoc, output);
  return output;
}

/**
 * @brief Extract alarm JSON from an already parsed radio configuration document.
 *
 * @param doc Parsed radio configuration document
 * @return String Alarm-only JSON document
 */
static String extractAlarmsJsonFromDoc(JsonDocument &doc) {
  JsonDocument alarmsDoc;
  alarmsDoc["version"] = doc["version"] | 1;
  alarmsDoc["alarms"].set(doc["alarms"]);

  String output;
  serializeJson(alarmsDoc, output);
  return output;
}

/**
 * @brief Read the configuration version from a JSON value.
 *
 * @param version Version field from a radio configuration document
 * @return int Version number, or 0 if missing/invalid
 */
static int readConfigVersion(JsonVariantConst version) {
  if (version.isNull()) {
    return 0;
  }
  if (version.is<int>()) {
    return version.as<int>();
  }
  if (version.is<long>()) {
    return (int)version.as<long>();
  }
  if (version.is<double>()) {
    return (int)version.as<double>();
  }
  return 0;
}

/**
 * @brief Append one factory-default weekday alarm to a JSON array.
 *
 * @param alarms Target alarm array
 */
static void appendDefaultAlarm(JsonArray &alarms) {
  JsonObject alarm = alarms.add<JsonObject>();
  alarm["id"] = 1;
  alarm["enabled"] = true;
  alarm["start"] = "6:30";
  alarm["stop"] = "7:30";
  JsonArray days = alarm["days"].to<JsonArray>();
  days.add("mon");
  days.add("tue");
  days.add("wed");
  days.add("thu");
  days.add("fri");
}

/**
 * @brief Overlay the alarms array from the authoritative alrm_config NVS key.
 *
 * The radio_config blob can carry a stale copy (e.g. factory 6:30-7:30) while
 * alrm_config holds the schedule the firmware actually runs. Partial saves
 * (features, clock, tone) must never persist that stale copy back into NVS.
 *
 * @param doc Target radio configuration document (modified in place)
 */
static void seedAlarmsFromAuthoritativeStore(JsonDocument &doc) {
  String alarmJson = ini_block.alrm_config.length() > 0 ? ini_block.alrm_config : nvsGetAlarmConfig();
  if (alarmJson.length() == 0) {
    return;
  }

  JsonDocument alarmsDoc;
  if (deserializeJson(alarmsDoc, alarmJson) || alarmsDoc.overflowed()) {
    return;
  }
  if (!alarmsDoc["alarms"].is<JsonArray>() || alarmsDoc["alarms"].size() == 0) {
    return;
  }

  doc["alarms"].set(alarmsDoc["alarms"]);
}

/**
 * @brief Fill missing top-level radio configuration fields with firmware defaults.
 *
 * Existing values are preserved. New fields introduced by firmware updates are added
 * so older NVS documents remain writable after a firmware upgrade.
 *
 * @param doc Parsed radio configuration document
 */
static void ensureRadioConfigStructure(JsonDocument &doc) {
  int version = readConfigVersion(doc["version"]);
  if (version < RADIO_CONFIG_VERSION) {
    doc["version"] = RADIO_CONFIG_VERSION;
  }

  if (!doc["alarms"].is<JsonArray>()) {
    doc["alarms"].to<JsonArray>();
  }
  JsonArray alarms = doc["alarms"].to<JsonArray>();
  if (alarms.size() == 0) {
    seedAlarmsFromAuthoritativeStore(doc);
    alarms = doc["alarms"].as<JsonArray>();
  }
  if (alarms.size() == 0) {
    appendDefaultAlarm(alarms);
  }

  if (!doc["clock"].is<JsonObject>()) {
    JsonObject clock = doc["clock"].to<JsonObject>();
    clock["server"] = "pool.ntp.org";
    clock["offset"] = 1;
    clock["dst"] = 1;
  } else {
    JsonObject clock = doc["clock"].as<JsonObject>();
    const char *server = clock["server"] | "pool.ntp.org";
    if (!server || strlen(server) == 0) {
      clock["server"] = "pool.ntp.org";
    }
    if (clock["offset"].isNull()) {
      clock["offset"] = 1;
    }
    if (clock["dst"].isNull()) {
      clock["dst"] = 1;
    }
  }

  if (!doc["wifi"].is<JsonArray>()) {
    doc["wifi"].to<JsonArray>();
  }

  if (!doc["presets"].is<JsonArray>() || doc["presets"].as<JsonArray>().size() == 0) {
    JsonArray presets = doc["presets"].to<JsonArray>();
    presets.clear();
    appendDefaultPresets(presets);
  }

  if (!doc["gpio"].is<JsonArray>() || doc["gpio"].as<JsonArray>().size() == 0) {
    JsonArray gpio = doc["gpio"].to<JsonArray>();
    gpio.clear();
    appendDefaultGpio(gpio);
  }

  if (!doc["tone"].is<JsonObject>()) {
    JsonObject tone = doc["tone"].to<JsonObject>();
    tone["ha"] = 0;
    tone["hf"] = 0;
    tone["la"] = 0;
    tone["lf"] = 0;
  }

  if (doc["volume"].isNull()) {
    doc["volume"] = RADIO_DEFAULT_VOLUME;
  }
#if defined(ST7701_28B)
  else if (doc["volume"].as<int>() <= 0) {
    doc["volume"] = RADIO_DEFAULT_VOLUME;
  }
#endif
  if (doc["preset"].isNull()) {
    doc["preset"] = 6;
  }
}

/**
 * @brief Ensure the optional features object exists and contains all current fields.
 *
 * Fills missing keys from firmware defaults, then syncs runtime from the document.
 * Does not read individual feat_* NVS keys (those are applied at boot / rebuild).
 *
 * @param doc Parsed radio configuration document
 */
static void ensureRadioConfigFeatures(JsonDocument &doc) {
  // Snapshot defaults into the document for any missing keys without wiping
  // values already present in doc. Using setFeatureDefaults()+applyFrom used to
  // depend on applyFrom successfully reading every string field.
  FeatureSettings defaults;
  defaults.dark_start = 20;
  defaults.dark_end = 7;
  defaults.vu_meter = true;
  defaults.display_rotate_180 = false;
  defaults.stream_watchdog = true;
  defaults.alarm_fade_sec = 15;
  defaults.alarm_fallback_sound = "beep";
  defaults.alarm_fallback_interval_sec = 5;
  defaults.ota_check_url =
      "https://raw.githubusercontent.com/joruf/esp32-webradio/main/ota/version.json";

  JsonObject feat =
      doc["features"].is<JsonObject>() ? doc["features"].as<JsonObject>() : doc["features"].to<JsonObject>();
  if (feat["dark_start"].isNull()) {
    feat["dark_start"] = defaults.dark_start;
  }
  if (feat["dark_end"].isNull()) {
    feat["dark_end"] = defaults.dark_end;
  }
  if (feat["vu_meter"].isNull()) {
    feat["vu_meter"] = defaults.vu_meter;
  }
  if (feat["display_rotate_180"].isNull()) {
    feat["display_rotate_180"] = defaults.display_rotate_180;
  }
  if (feat["stream_watchdog"].isNull()) {
    feat["stream_watchdog"] = defaults.stream_watchdog;
  }
  if (feat["alarm_fade_sec"].isNull()) {
    feat["alarm_fade_sec"] = defaults.alarm_fade_sec;
  }
  {
    const char *sound = feat["alarm_fallback_sound"].as<const char *>();
    if (!sound || strlen(sound) == 0) {
      feat["alarm_fallback_sound"] = defaults.alarm_fallback_sound;
    }
  }
  if (feat["alarm_fallback_interval_sec"].isNull()) {
    feat["alarm_fallback_interval_sec"] = defaults.alarm_fallback_interval_sec;
  }
  {
    const char *url = feat["ota_check_url"].as<const char *>();
    if (!url || strlen(url) == 0) {
      feat["ota_check_url"] = defaults.ota_check_url;
    }
  }

  applyFeaturesFromDoc(doc);
  appendFeaturesToDoc(doc);
}

/**
 * @brief Return true if a stored radio_config blob should be rebuilt from NVS keys.
 *
 * @param stored Serialized radio_config JSON from NVS
 * @return bool
 */
static bool radioConfigNeedsUpgrade(const String &stored) {
  if (stored.length() == 0) {
    return true;
  }

  JsonDocument doc;
  if (deserializeJson(doc, stored) || doc.overflowed()) {
    return true;
  }

  if (readConfigVersion(doc["version"]) < RADIO_CONFIG_VERSION) {
    return true;
  }
  if (!doc["features"].is<JsonObject>()) {
    return true;
  }
  if (!doc["tone"].is<JsonObject>()) {
    return true;
  }
  if (!doc["alarms"].is<JsonArray>()) {
    return true;
  }
  if (!doc["clock"].is<JsonObject>()) {
    return true;
  }
  if (!doc["wifi"].is<JsonArray>()) {
    return true;
  }
  if (!doc["presets"].is<JsonArray>()) {
    return true;
  }
  if (!doc["gpio"].is<JsonArray>()) {
    return true;
  }
#if defined(ST7701_28B)
  if (!doc["volume"].isNull() && doc["volume"].as<int>() <= 0) {
    return true;
  }
#endif

  return false;
}

/**
 * @brief Validate unified radio configuration JSON.
 *
 * @param json JSON document
 * @return true if the document is valid
 */
bool isValidRadioConfigJson(const String &json) {
  if (json.length() == 0 || json.length() >= RADIO_CONFIG_MAX_LEN) {
    return false;
  }

  JsonDocument doc;
  if (deserializeJson(doc, json) || doc.overflowed()) {
    return false;
  }

  ensureRadioConfigStructure(doc);
  ensureRadioConfigFeatures(doc);

  return doc["alarms"].is<JsonArray>() && doc["clock"].is<JsonObject>() && doc["wifi"].is<JsonArray>() &&
         doc["presets"].is<JsonArray>() && doc["gpio"].is<JsonArray>() && doc["features"].is<JsonObject>();
}

/**
 * @brief Build factory-default radio configuration JSON without reading NVS.
 *
 * @return String JSON document
 */
static String buildFreshDefaultRadioConfigJson() {
  JsonDocument doc;

  doc["version"] = RADIO_CONFIG_VERSION;

  JsonArray alarms = doc["alarms"].to<JsonArray>();
  JsonObject alarm = alarms.add<JsonObject>();
  alarm["id"] = 1;
  alarm["enabled"] = true;
  alarm["start"] = "6:30";
  alarm["stop"] = "7:30";
  JsonArray days = alarm["days"].to<JsonArray>();
  days.add("mon");
  days.add("tue");
  days.add("wed");
  days.add("thu");
  days.add("fri");

  JsonObject clock = doc["clock"].to<JsonObject>();
  clock["server"] = "pool.ntp.org";
  clock["offset"] = 1;
  clock["dst"] = 1;

  doc["volume"] = RADIO_DEFAULT_VOLUME;
  doc["preset"] = 6;

  JsonObject tone = doc["tone"].to<JsonObject>();
  tone["ha"] = 0;
  tone["hf"] = 0;
  tone["la"] = 0;
  tone["lf"] = 0;

  doc["wifi"].to<JsonArray>();
  JsonArray presets = doc["presets"].to<JsonArray>();
  appendDefaultPresets(presets);
  JsonArray gpio = doc["gpio"].to<JsonArray>();
  appendDefaultGpio(gpio);
  setFeatureDefaults();
  appendFeaturesToDoc(doc);

  String output;
  serializeJson(doc, output);
  return output;
}

/**
 * @brief Build unified radio configuration JSON from individual NVS keys.
 *
 * @param maskWifiPasswords If true, WiFi passwords are masked in output
 * @return String JSON document
 */
String buildRadioConfigJsonFromNvs(bool maskWifiPasswords) {
  JsonDocument doc;
  char key[16];
  uint16_t i;

  doc["version"] = RADIO_CONFIG_VERSION;

  JsonArray alarms = doc["alarms"].to<JsonArray>();
  String alarmJson = nvsGetAlarmConfig();
  if (alarmJson.length() > 0) {
    JsonDocument alarmDoc;
    if (!deserializeJson(alarmDoc, alarmJson)) {
      for (JsonObject obj : alarmDoc["alarms"].as<JsonArray>()) {
        alarms.add(obj);
      }
    }
  }
  if (alarms.size() == 0) {
    JsonObject obj = alarms.add<JsonObject>();
    obj["id"] = 1;
    obj["enabled"] = true;
    obj["start"] = "6:30";
    obj["stop"] = "7:30";
    JsonArray days = obj["days"].to<JsonArray>();
    days.add("mon");
    days.add("tue");
    days.add("wed");
    days.add("thu");
    days.add("fri");
  }

  JsonObject clock = doc["clock"].to<JsonObject>();
  clock["server"] = nvsKeyExists("clk_server") ? nvsGetValue("clk_server") : "pool.ntp.org";
  clock["offset"] = nvsKeyExists("clk_offset") ? nvsGetValue("clk_offset").toInt() : 1;
  clock["dst"] = nvsKeyExists("clk_dst") ? nvsGetValue("clk_dst").toInt() : 1;

  doc["volume"] = nvsKeyExists("volume") ? nvsGetValue("volume").toInt() : RADIO_DEFAULT_VOLUME;
  doc["preset"] = nvsKeyExists("preset") ? nvsGetValue("preset").toInt() : 6;

  JsonObject tone = doc["tone"].to<JsonObject>();
  tone["ha"] = nvsKeyExists("toneha") ? nvsGetValue("toneha").toInt() : 0;
  tone["hf"] = nvsKeyExists("tonehf") ? nvsGetValue("tonehf").toInt() : 0;
  tone["la"] = nvsKeyExists("tonela") ? nvsGetValue("tonela").toInt() : 0;
  tone["lf"] = nvsKeyExists("tonelf") ? nvsGetValue("tonelf").toInt() : 0;

  JsonArray wifi = doc["wifi"].to<JsonArray>();
  for (i = 0; i < 100; i++) {
    sprintf(key, "wifi_%02d", i);
    if (!nvsKeyExists(key)) {
      break;
    }
    String entry = nvsGetValue(key);
    int sep = entry.indexOf('/');
    if (sep <= 0) {
      continue;
    }
    JsonObject obj = wifi.add<JsonObject>();
    obj["ssid"] = entry.substring(0, sep);
    if (maskWifiPasswords) {
      obj["password"] = RADIO_CONFIG_MASKED_PASSWORD;
    } else {
      obj["password"] = entry.substring(sep + 1);
    }
  }

  JsonArray presets = doc["presets"].to<JsonArray>();
  for (i = 0; i < MAXPRESETS; i++) {
    sprintf(key, "preset_%02d", i);
    if (!nvsKeyExists(key)) {
      continue;
    }
    String val = nvsGetValue(key);
    if (val.length() == 0) {
      continue;
    }
    String url;
    String name;
    splitPresetValue(val, url, name);
    JsonObject obj = presets.add<JsonObject>();
    obj["slot"] = i;
    obj["url"] = url;
    obj["name"] = name;
  }

  JsonArray gpio = doc["gpio"].to<JsonArray>();
  for (i = 0; i < 100; i++) {
    sprintf(key, "gpio_%02d", i);
    if (!nvsKeyExists(key)) {
      continue;
    }
    String command = nvsGetValue(key);
    if (command.length() == 0) {
      continue;
    }
    JsonObject obj = gpio.add<JsonObject>();
    obj["pin"] = i;
    obj["command"] = command;
  }
  if (presets.size() == 0) {
    appendDefaultPresets(presets);
  }
  if (gpio.size() == 0) {
    appendDefaultGpio(gpio);
  }

  setFeatureDefaults();
  String storedCfg = nvsGetRadioConfig();
  if (storedCfg.length() > 0) {
    JsonDocument featDoc;
    if (!deserializeJson(featDoc, storedCfg)) {
      applyFeaturesFromDoc(featDoc);
    }
  }
  applyFeatureNvsKeyOverrides();
  appendFeaturesToDoc(doc);

  if (doc.overflowed()) {
    dbgprint("buildRadioConfigJsonFromNvs: JSON document overflow");
    return buildFreshDefaultRadioConfigJson();
  }

  String output;
  serializeJson(doc, output);
  if (output.length() == 0 || !isValidRadioConfigJson(output)) {
    dbgprint("buildRadioConfigJsonFromNvs: serialization failed");
    return buildFreshDefaultRadioConfigJson();
  }

  return output;
}

/**
 * @brief Load the current radio configuration document for merging.
 *
 * @param doc Target document
 * @return true if a usable document was loaded
 */
static bool loadRadioConfigDocument(JsonDocument &doc) {
  String stored = nvsGetRadioConfig();
  if (stored.length() > 0 && !deserializeJson(doc, stored) && !doc.overflowed()) {
    ensureRadioConfigStructure(doc);
    return true;
  }

  String rebuilt = buildRadioConfigJsonFromNvs(false);
  if (rebuilt.length() == 0 || deserializeJson(doc, rebuilt) || doc.overflowed()) {
    return false;
  }

  ensureRadioConfigStructure(doc);
  return true;
}

/**
 * @brief Sync volume, tone, preset, clock and features in a config document with live runtime state.
 *
 * Individual NVS keys and ini_block are authoritative for these values (encoder,
 * /?volume=, tone commands, station switch may update them without rewriting
 * the radio_config blob). Used when serving getconfig and before persisting the blob.
 *
 * @param doc Parsed radio configuration document (modified in place)
 */
static void syncDocWithPersistedRuntime(JsonDocument &doc) {
  doc["volume"] = ini_block.reqvol;
  doc["preset"] = (currentpreset >= 0) ? currentpreset : ini_block.newpreset;

  JsonObject tone = doc["tone"].to<JsonObject>();
  tone["ha"] = ini_block.rtone[0];
  tone["hf"] = ini_block.rtone[1];
  tone["la"] = ini_block.rtone[2];
  tone["lf"] = ini_block.rtone[3];

  JsonObject clock = doc["clock"].to<JsonObject>();
  clock["server"] = ini_block.clk_server.length() > 0 ? ini_block.clk_server : String("pool.ntp.org");
  clock["offset"] = ini_block.clk_offset;
  clock["dst"] = ini_block.clk_dst;

  appendFeaturesToDoc(doc);
}

/**
 * @brief Persist a prepared radio configuration document to NVS and runtime state.
 *
 * @param doc Parsed and normalized radio configuration document
 * @param maskWifiPasswords If true, masked WiFi passwords are restored from NVS
 * @param updateAlarms If true, persist the alarms array to alrm_config NVS; false
 *                     keeps the authoritative schedule untouched (partial saves)
 * @return true if configuration was accepted
 */
static bool applyRadioConfigDoc(JsonDocument &doc, bool maskWifiPasswords, bool updateAlarms) {
  ensureRadioConfigStructure(doc);
  doc.remove("wifi_signals");

  uint16_t i;
  char key[16];

  nvsOpen();

  static String savedWifi[100];

  for (i = 0; i < 100; i++) {
    sprintf(key, "wifi_%02d", i);
    savedWifi[i] = nvsKeyExists(key) ? nvsGetValue(key) : "";
  }

  JsonArray wifi = doc["wifi"].as<JsonArray>();
  for (i = 0; i < 100; i++) {
    sprintf(key, "wifi_%02d", i);
    if (nvsKeyExists(key)) {
      nvs_erase_key(nvshandle, key);
    }
  }
  i = 0;
  for (JsonObject obj : wifi) {
    const char *ssid = obj["ssid"] | "";
    const char *password = obj["password"] | "";
    if (strlen(ssid) == 0) {
      continue;
    }
    String pass = String(password);
    if (maskWifiPasswords &&
        (pass.length() == 0 || pass == RADIO_CONFIG_MASKED_PASSWORD)) {
      uint16_t w;
      for (w = 0; w < 100; w++) {
        if (savedWifi[w].length() == 0) {
          continue;
        }
        int sep = savedWifi[w].indexOf('/');
        if (sep > 0 && savedWifi[w].substring(0, sep) == String(ssid)) {
          pass = savedWifi[w].substring(sep + 1);
          break;
        }
      }
    }
    // Never persist a masked placeholder as the real password.
    if (pass.length() == 0 || pass == RADIO_CONFIG_MASKED_PASSWORD) {
      continue;
    }
    // Deduplicate SSIDs (keep first entry with a real password).
    bool dupSsid = false;
    for (uint16_t prev = 0; prev < i; prev++) {
      char prevKey[16];
      sprintf(prevKey, "wifi_%02d", prev);
      String prevVal = nvsKeyExists(prevKey) ? nvsGetValue(prevKey) : "";
      int sep = prevVal.indexOf('/');
      if (sep > 0 && prevVal.substring(0, sep) == String(ssid)) {
        dupSsid = true;
        break;
      }
    }
    if (dupSsid) {
      continue;
    }
    sprintf(key, "wifi_%02d", i);
    nvsAddValue(key, String(ssid) + String("/") + pass);
    i++;
  }

  JsonObject clock = doc["clock"];
  String previousClkServer = ini_block.clk_server;
  nvsAddValue("clk_server", String(clock["server"] | "pool.ntp.org"));
  nvsAddValue("clk_offset", String(clock["offset"] | 1));
  nvsAddValue("clk_dst", String(clock["dst"] | 1));
  ini_block.clk_server = String(clock["server"] | "pool.ntp.org");
  ini_block.clk_offset = clock["offset"] | 1;
  ini_block.clk_dst = clock["dst"] | 1;
  bool restartNtp = (ini_block.clk_server != previousClkServer);

  uint8_t volume = doc["volume"] | RADIO_DEFAULT_VOLUME;
#if defined(ST7701_28B)
  if (volume == 0) {
    volume = RADIO_DEFAULT_VOLUME;
  }
#endif
  if (volume > 100) {
    volume = 100;
  }
  nvsAddValue("volume", String(volume));
  ini_block.reqvol = volume;

  int16_t preset = doc["preset"] | 0;
  if (preset < 0) {
    preset = 0;
  }
  if (preset >= MAXPRESETS) {
    preset = MAXPRESETS - 1;
  }
  nvsAddValue("preset", String(preset));
  ini_block.newpreset = preset;

  JsonObject tone = doc["tone"];
  nvsAddValue("toneha", String(tone["ha"] | 0));
  nvsAddValue("tonehf", String(tone["hf"] | 0));
  nvsAddValue("tonela", String(tone["la"] | 0));
  nvsAddValue("tonelf", String(tone["lf"] | 0));
  ini_block.rtone[0] = tone["ha"] | 0;
  ini_block.rtone[1] = tone["hf"] | 0;
  ini_block.rtone[2] = tone["la"] | 0;
  ini_block.rtone[3] = tone["lf"] | 0;
  reqtone = true;

  for (i = 0; i < MAXPRESETS; i++) {
    sprintf(key, "preset_%02d", i);
    if (nvsKeyExists(key)) {
      nvs_erase_key(nvshandle, key);
    }
  }
  for (JsonObject obj : doc["presets"].as<JsonArray>()) {
    int slot = obj["slot"] | -1;
    const char *url = obj["url"] | "";
    if (slot < 0 || slot >= MAXPRESETS || strlen(url) == 0) {
      continue;
    }
    sprintf(key, "preset_%02d", slot);
    nvsAddValue(key, buildPresetValue(String(url), String(obj["name"] | "")));
  }

  for (i = 0; i < 100; i++) {
    sprintf(key, "gpio_%02d", i);
    if (nvsKeyExists(key)) {
      nvs_erase_key(nvshandle, key);
    }
  }
  for (JsonObject obj : doc["gpio"].as<JsonArray>()) {
    int pin = obj["pin"] | -1;
    const char *command = obj["command"] | "";
    if (pin < 0 || pin > 99 || strlen(command) == 0) {
      continue;
    }
    sprintf(key, "gpio_%02d", pin);
    nvsAddValue(key, String(command));
  }

  String alarmsJson = extractAlarmsJsonFromDoc(doc);
  if (updateAlarms && alarmsJson.length() > 0) {
    if (!applyAlarmConfig(alarmsJson)) {
      dbgprint("applyRadioConfig: alarm save rejected, keeping existing schedule");
      JsonDocument alarmsDoc;
      if (ini_block.alrm_config.length() > 0 &&
          !deserializeJson(alarmsDoc, ini_block.alrm_config)) {
        doc["alarms"].set(alarmsDoc["alarms"]);
      }
    } else {
      JsonDocument alarmsDoc;
      if (!deserializeJson(alarmsDoc, ini_block.alrm_config)) {
        doc["alarms"].set(alarmsDoc["alarms"]);
      }
    }
  }

  ensureRadioConfigFeatures(doc);
  JsonObject feat = doc["features"];
  nvsAddValue("feat_dark_start", String((int)(feat["dark_start"] | 20)));
  nvsAddValue("feat_dark_end", String((int)(feat["dark_end"] | 7)));
  nvsAddValue("feat_vu_meter", String((int)(feat["vu_meter"] | true)));
  nvsAddValue("feat_rot180", String((int)(feat["display_rotate_180"] | false)));
  nvsAddValue("feat_watchdog", String((int)(feat["stream_watchdog"] | true)));
  nvsAddValue("feat_alarm_fade", String((int)(feat["alarm_fade_sec"] | 15)));
  {
    const char *fb = feat["alarm_fallback_sound"] | "beep";
    nvsAddValue("feat_alarm_fb", normalizeAlarmFallbackSoundId(String(fb)));
  }
  nvsAddValue("feat_fb_int", String((int)(feat["alarm_fallback_interval_sec"] | 5)));
  nvsAddValue("feat_ota_url", String((const char *)(feat["ota_check_url"] | "")));

  // Apply the feature settings to the live in-memory struct as well, so changes
  // (alarm fade duration, dark mode, VU meter, watchdog, OTA URL) take effect
  // immediately without requiring a reboot.
  applyFeaturesFromDoc(doc);

  syncDocWithPersistedRuntime(doc);

  String outputJson;
  serializeJson(doc, outputJson);
  if (outputJson.length() == 0 || outputJson.length() >= RADIO_CONFIG_MAX_LEN) {
    dbgprint("applyRadioConfig: radio_config blob skipped (length=%d)", outputJson.length());
  } else if (nvsSetRadioConfig(outputJson) != ESP_OK) {
    dbgprint("applyRadioConfig: radio_config blob save failed, keeping key-based config");
  }

  if (nvsCommit() != ESP_OK) {
    dbgprint("applyRadioConfig: NVS commit failed");
    eventLogRecordError("Config save failed (NVS commit)");
    return false;
  }

  applyClockConfiguration(restartNtp);
  return true;
}

/**
 * @brief Merge a partial JSON patch into the stored config and persist it.
 *
 * @param patchJson JSON document with one or more top-level keys to update
 * @param maskWifiPasswords If true, masked WiFi passwords are restored from NVS
 * @return true if configuration was accepted
 */
bool applyRadioConfigMerge(const String &patchJson, bool maskWifiPasswords) {
  if (patchJson.length() == 0 || patchJson.length() >= RADIO_CONFIG_MAX_LEN) {
    dbgprint("applyRadioConfigMerge: invalid length");
    return false;
  }

  JsonDocument patch;
  if (deserializeJson(patch, patchJson) || patch.overflowed()) {
    dbgprint("applyRadioConfigMerge: parse failed");
    return false;
  }

  JsonDocument doc;
  if (!loadRadioConfigDocument(doc)) {
    dbgprint("applyRadioConfigMerge: unable to load base config");
    eventLogRecordError("Config save failed (base config unreadable)");
    return false;
  }

  // The stored config blob is not rewritten on every runtime volume/tone/preset
  // change (encoder, /?volume=, tone commands, station switch). Seed the merge
  // base with the live runtime values so a partial patch that does not carry
  // them (e.g. the scheduler/display pages send only "features") does not revert
  // the current volume, tone or station. A patch that actually contains these
  // keys still wins in mergeRadioConfigPatch() below.
  doc["volume"] = ini_block.reqvol;
  JsonObject baseTone = doc["tone"].to<JsonObject>();
  baseTone["ha"] = ini_block.rtone[0];
  baseTone["hf"] = ini_block.rtone[1];
  baseTone["la"] = ini_block.rtone[2];
  baseTone["lf"] = ini_block.rtone[3];
  JsonObject baseClock = doc["clock"].to<JsonObject>();
  baseClock["server"] = ini_block.clk_server.length() > 0 ? ini_block.clk_server : String("pool.ntp.org");
  baseClock["offset"] = ini_block.clk_offset;
  baseClock["dst"] = ini_block.clk_dst;
  appendFeaturesToDoc(doc);
  // Keep the currently playing station. currentpreset is the live value; fall
  // back to the requested preset while a switch is still in progress (-1).
  doc["preset"] = (currentpreset >= 0) ? currentpreset : ini_block.newpreset;

  // Partial patches never carry alarms. Seed from the authoritative alrm_config
  // key so a stale radio_config blob cannot overwrite the live schedule.
  const bool patchUpdatesAlarms =
      patch["alarms"].is<JsonArray>() && patch["alarms"].as<JsonArray>().size() > 0;
  if (!patchUpdatesAlarms) {
    seedAlarmsFromAuthoritativeStore(doc);
  }

  mergeRadioConfigPatch(doc, patch);
  return applyRadioConfigDoc(doc, maskWifiPasswords, patchUpdatesAlarms);
}

/**
 * @brief Merge and persist only the features section of the radio configuration.
 *
 * @param json JSON document containing a "features" object
 * @return true if feature settings were saved
 */
bool applyFeaturesConfig(const String &json) {
  return applyRadioConfigMerge(json, false);
}

/**
 * @brief Apply unified radio configuration JSON to NVS and runtime state.
 *
 * @param json JSON document (full or partial)
 * @param maskWifiPasswords If true, masked WiFi passwords are restored from NVS
 * @return true if configuration was accepted
 */
bool applyRadioConfig(const String &json, bool maskWifiPasswords) {
  return applyRadioConfigMerge(json, maskWifiPasswords);
}

/**
 * @brief Ensure unified radio configuration exists in NVS.
 */
void migrateToRadioConfig() {
  if (!nvsKeyExists(RADIO_CONFIG_NVS_KEY)) {
    dbgprint("BOOT: Creating radio_config JSON from NVS keys");
    nvsSetRadioConfig(buildRadioConfigJsonFromNvs(false));
    nvsCommit();
    return;
  }

  String stored = nvsGetRadioConfig();
  if (radioConfigNeedsUpgrade(stored)) {
    dbgprint("BOOT: Upgrading radio_config JSON to v%d", RADIO_CONFIG_VERSION);
    String json = buildRadioConfigJsonFromNvs(false);
    if (json.length() > 0) {
      nvsSetRadioConfig(json);
      nvsCommit();
    }
  }
}

/**
 * @brief Update the alarms array inside the unified radio_config NVS document.
 *
 * @return true if the radio_config blob was updated
 */
bool syncAlarmsToRadioConfig() {
  if (ini_block.alrm_config.length() == 0) {
    return false;
  }

  JsonDocument doc;
  if (!loadRadioConfigDocument(doc)) {
    return false;
  }

  JsonDocument alarmsDoc;
  if (deserializeJson(alarmsDoc, ini_block.alrm_config) || alarmsDoc.overflowed()) {
    return false;
  }

  doc["alarms"].set(alarmsDoc["alarms"]);
  // Keep live feature settings in the blob. Do not run ensureRadioConfigFeatures
  // here: that path used to reset globals and could rewrite string features to
  // defaults when the blob was stale.
  appendFeaturesToDoc(doc);

  String outputJson;
  serializeJson(doc, outputJson);
  if (outputJson.length() == 0 || outputJson.length() >= RADIO_CONFIG_MAX_LEN) {
    return false;
  }

  if (nvsSetRadioConfig(outputJson) != ESP_OK) {
    return false;
  }

  return nvsCommit() == ESP_OK;
}

/**
 * @brief Mask WiFi passwords before sending config JSON to web clients.
 *
 * @param doc Parsed radio configuration document
 */
static void maskWifiPasswordsForWeb(JsonDocument &doc) {
  if (!doc["wifi"].is<JsonArray>()) {
    return;
  }

  for (JsonObject net : doc["wifi"].as<JsonArray>()) {
    if (!net["password"].isNull()) {
      net["password"] = RADIO_CONFIG_MASKED_PASSWORD;
    }
  }
}

/**
 * @brief Read unified config for web clients with masked WiFi passwords.
 *
 * Prefers the persisted `radio_config` blob to reduce NVS key scans. Falls back
 * to rebuilding from individual NVS keys only when the blob is missing/invalid.
 *
 * @return String JSON document safe for web clients
 */
String getRadioConfigJsonForWeb() {
  String stored = nvsGetRadioConfig();
  if (stored.length() > 0) {
    JsonDocument doc;
    if (!deserializeJson(doc, stored) && !doc.overflowed()) {
      ensureRadioConfigStructure(doc);
      if (!doc["features"].is<JsonObject>()) {
        doc["features"].to<JsonObject>();
      }
      maskWifiPasswordsForWeb(doc);

      // Alarms are owned by the authoritative alrm_config key (that is what the
      // firmware actually schedules). The copy embedded in the radio_config blob
      // can be stale, so always overlay the real schedule before serving it to
      // the web UI. alarmDoc must stay in scope until serializeJson() below.
      String alarmJson = ini_block.alrm_config.length() > 0 ? ini_block.alrm_config : nvsGetAlarmConfig();
      JsonDocument alarmDoc;
      if (alarmJson.length() > 0 && !deserializeJson(alarmDoc, alarmJson) && !alarmDoc.overflowed() &&
          alarmDoc["alarms"].is<JsonArray>()) {
        doc["alarms"].set(alarmDoc["alarms"]);
      }

      syncDocWithPersistedRuntime(doc);

      String output;
      serializeJson(doc, output);
      if (output.length() > 0 && output.length() < RADIO_CONFIG_MAX_LEN) {
        return output;
      }
    }
  }

  return buildRadioConfigJsonFromNvs(true);
}

/**
 * @brief Return the default unified radio configuration JSON.
 *
 * @return String JSON document
 */
String getDefaultRadioConfigJson() {
  return buildFreshDefaultRadioConfigJson();
}

#endif
