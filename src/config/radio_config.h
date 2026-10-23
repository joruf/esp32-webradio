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
  if (nvsKeyExists("feat_colstart")) {
    radioFeatures.display_colstart =
        (uint8_t)constrain(nvsGetValue("feat_colstart").toInt(), 0, 7);
  }
  if (nvsKeyExists("feat_rowstart")) {
    radioFeatures.display_rowstart =
        (uint8_t)constrain(nvsGetValue("feat_rowstart").toInt(), 0, 7);
  }
  if (nvsKeyExists("feat_enc_inv")) {
    radioFeatures.encoder_invert = nvsGetValue("feat_enc_inv").toInt() != 0;
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
  defaults.display_colstart = 0;
  defaults.display_rowstart = 0;
  defaults.encoder_invert = false;
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
  if (feat["display_colstart"].isNull()) {
    feat["display_colstart"] = defaults.display_colstart;
  }
  if (feat["display_rowstart"].isNull()) {
    feat["display_rowstart"] = defaults.display_rowstart;
  }
  if (feat["encoder_invert"].isNull()) {
    feat["encoder_invert"] = defaults.encoder_invert;
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
static bool applyRadioConfigDoc(JsonDocument &doc, bool maskWifiPasswords, bool updateAlarms,
                                bool skipWifiPersist = false) {
  ensureRadioConfigStructure(doc);
  doc.remove("wifi_signals");

  uint16_t i;
  char key[16];

  nvsOpen();

  static String savedWifi[100];

  JsonArray wifi = doc["wifi"].as<JsonArray>();
  i = 0;
  if (skipWifiPersist) {
    // wifi_XX already written by persistWifiJsonToNvs(); keep keys intact.
    for (i = 0; i < 100; i++) {
      sprintf(key, "wifi_%02d", i);
      if (!nvsKeyExists(key)) {
        break;
      }
    }
  } else {
  for (i = 0; i < 100; i++) {
    sprintf(key, "wifi_%02d", i);
    savedWifi[i] = nvsKeyExists(key) ? nvsGetValue(key) : "";
  }

  for (i = 0; i < 100; i++) {
    sprintf(key, "wifi_%02d", i);
    if (nvsKeyExists(key)) {
      nvs_erase_key(nvshandle, key);
    }
  }
  i = 0;
  for (JsonObject obj : wifi) {
    // Copy out of the JsonDocument immediately; later String/NVS work can move heap.
    String ssid = String(obj["ssid"] | "");
    String pass = String(obj["password"] | "");
    ssid.trim();
    if (ssid.length() == 0) {
      continue;
    }
    if (maskWifiPasswords &&
        (pass.length() == 0 || pass == RADIO_CONFIG_MASKED_PASSWORD)) {
      uint16_t w;
      for (w = 0; w < 100; w++) {
        if (savedWifi[w].length() == 0) {
          continue;
        }
        int sep = savedWifi[w].indexOf('/');
        if (sep > 0 && savedWifi[w].substring(0, sep) == ssid) {
          pass = savedWifi[w].substring(sep + 1);
          break;
        }
      }
    }
    // Never persist a masked placeholder as the real password. Empty passwords
    // are allowed (open networks) and must still be saved so offline/hidden
    // SSIDs entered in the UI are always kept regardless of scan results.
    if (pass == RADIO_CONFIG_MASKED_PASSWORD) {
      dbgprint("applyRadioConfig: skip WiFi SSID '%s' (masked password, no NVS match)",
               ssid.c_str());
      continue;
    }
    // Deduplicate SSIDs (keep first entry).
    bool dupSsid = false;
    for (uint16_t prev = 0; prev < i; prev++) {
      char prevKey[16];
      sprintf(prevKey, "wifi_%02d", prev);
      String prevVal = nvsKeyExists(prevKey) ? nvsGetValue(prevKey) : "";
      int sep = prevVal.indexOf('/');
      if (sep > 0 && prevVal.substring(0, sep) == ssid) {
        dupSsid = true;
        break;
      }
    }
    if (dupSsid) {
      continue;
    }
    sprintf(key, "wifi_%02d", i);
    nvsAddValue(key, ssid + String("/") + pass);
    // Keep the in-memory document in sync so the radio_config blob stores the
    // real password (web clients still receive a masked copy via getconfig).
    obj["ssid"] = ssid;
    obj["password"] = pass;
    i++;
  }
  } // !skipWifiPersist

  // Rebuild wifi[] from the wifi_XX keys so the blob matches NVS (drops dups /
  // masked-only entries that were not persisted).
  doc.remove("wifi");
  {
    JsonArray cleaned = doc["wifi"].to<JsonArray>();
    for (uint16_t w = 0; w < 100; w++) {
      sprintf(key, "wifi_%02d", w);
      if (!nvsKeyExists(key)) {
        break;
      }
      String entry = nvsGetValue(key);
      int sep = entry.indexOf('/');
      if (sep <= 0) {
        continue;
      }
      JsonObject obj = cleaned.add<JsonObject>();
      if (obj.isNull()) {
        break;
      }
      obj["ssid"] = entry.substring(0, sep);
      obj["password"] = entry.substring(sep + 1);
    }
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
  nvsAddValue("feat_colstart",
              String((int)constrain((int)(feat["display_colstart"] | 0), 0, 7)));
  nvsAddValue("feat_rowstart",
              String((int)constrain((int)(feat["display_rowstart"] | 0), 0, 7)));
  nvsAddValue("feat_enc_inv", String((int)(feat["encoder_invert"] | false)));
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
/**
 * @brief Persist a wifi JSON array to wifi_XX NVS keys (small document only).
 *
 * Writes in place (overwrite wifi_00..N, then erase trailing keys). Erase-all-
 * then-rewrite was dropping new SSIDs when NVS pages were full: erased slots
 * are not reclaimed until later, so wifi_01 failed while the stale radio_config
 * blob kept duplicate entries for getconfig.
 *
 * Masked passwords are restored from the previous wifi_XX values. Empty
 * passwords (open networks) and SSIDs that are not currently in range are kept.
 *
 * @param wifiJson Serialized JSON array of {ssid,password} objects
 * @param maskWifiPasswords Restore masked passwords from existing NVS entries
 * @return Number of wifi_XX keys written, or -1 on parse failure
 */
static int persistWifiJsonToNvs(const String &wifiJson, bool maskWifiPasswords) {
  // Wrap bare "[...]" in an object. On-device JsonDocument::is<JsonArray>() for an
  // array root rejected valid patches (save failed while wj>0).
  String wrapped = String("{\"wifi\":") + wifiJson + String("}");
  JsonDocument wifiDoc;
  if (deserializeJson(wifiDoc, wrapped) || wifiDoc.overflowed() || !wifiDoc["wifi"].is<JsonArray>()) {
    dbgprint("persistWifiJsonToNvs: parse failed (len=%d)", wifiJson.length());
    return -1;
  }

  nvsOpen();

  static String savedWifi[100];
  static String toWrite[100];
  char key[16];
  uint16_t i;
  uint16_t nWrite = 0;

  for (i = 0; i < 100; i++) {
    sprintf(key, "wifi_%02d", i);
    savedWifi[i] = nvsKeyExists(key) ? nvsGetValue(key) : "";
    toWrite[i] = "";
  }

  for (JsonObjectConst obj : wifiDoc["wifi"].as<JsonArrayConst>()) {
    String ssid = String(obj["ssid"] | "");
    String pass = String(obj["password"] | "");
    ssid.trim();
    if (ssid.length() == 0) {
      continue;
    }
    if (maskWifiPasswords &&
        (pass.length() == 0 || pass == RADIO_CONFIG_MASKED_PASSWORD)) {
      for (uint16_t w = 0; w < 100; w++) {
        if (savedWifi[w].length() == 0) {
          continue;
        }
        int sep = savedWifi[w].indexOf('/');
        if (sep > 0 && savedWifi[w].substring(0, sep) == ssid) {
          pass = savedWifi[w].substring(sep + 1);
          break;
        }
      }
    }
    if (pass == RADIO_CONFIG_MASKED_PASSWORD) {
      dbgprint("persistWifiJsonToNvs: skip '%s' (masked, no NVS match)", ssid.c_str());
      continue;
    }
    bool dupSsid = false;
    for (uint16_t prev = 0; prev < nWrite; prev++) {
      int sep = toWrite[prev].indexOf('/');
      if (sep > 0 && toWrite[prev].substring(0, sep) == ssid) {
        dupSsid = true;
        break;
      }
    }
    if (dupSsid) {
      continue;
    }
    String entry = ssid + String("/") + pass;
    if (entry.length() >= NVSBUFSIZE) {
      dbgprint("persistWifiJsonToNvs: skip '%s' (entry too long)", ssid.c_str());
      continue;
    }
    if (nWrite >= 100) {
      break;
    }
    toWrite[nWrite++] = entry;
  }

  // Count existing wifi_XX keys. Growing the list needs a free NVS page; the
  // large radio_config blob often occupies the last space, so free it first
  // when we need additional keys (caller rebuilds the blob afterwards).
  uint16_t existingCount = 0;
  for (i = 0; i < 100; i++) {
    sprintf(key, "wifi_%02d", i);
    if (!nvsKeyExists(key)) {
      break;
    }
    existingCount++;
  }
  if (nWrite > existingCount && nvsKeyExists(RADIO_CONFIG_NVS_KEY)) {
    dbgprint("persistWifiJsonToNvs: growing %u→%u, freeing radio_config for space",
             existingCount, nWrite);
    nvs_erase_key(nvshandle, RADIO_CONFIG_NVS_KEY);
    nvs_commit(nvshandle);
  }

  uint16_t written = 0;
  for (i = 0; i < nWrite; i++) {
    sprintf(key, "wifi_%02d", i);
    esp_err_t err = nvsAddValue(key, toWrite[i]);
    if (err != ESP_OK) {
      dbgprint("persistWifiJsonToNvs: %s write failed (%d), freeing radio_config", key, (int)err);
      if (nvsKeyExists(RADIO_CONFIG_NVS_KEY)) {
        nvs_erase_key(nvshandle, RADIO_CONFIG_NVS_KEY);
        nvs_commit(nvshandle);
      }
      err = nvsAddValue(key, toWrite[i]);
    }
    if (err != ESP_OK) {
      dbgprint("persistWifiJsonToNvs: %s write failed (%d)", key, (int)err);
      break;
    }
    written++;
  }

  for (i = written; i < 100; i++) {
    sprintf(key, "wifi_%02d", i);
    if (!nvsKeyExists(key)) {
      break;
    }
    nvs_erase_key(nvshandle, key);
  }

  dbgprint("persistWifiJsonToNvs: wrote %u of %u entries", written, nWrite);
  return (int)written;
}

/**
 * @brief Overlay a top-level object-array onto @p doc via JSON text round-trip.
 *
 * @param doc Destination configuration document
 * @param key Top-level key ("presets", "gpio", "alarms")
 * @param arrayJson Serialized JSON array text
 * @return true if the array was applied (including empty arrays)
 */
static bool applySerializedArrayPatch(JsonDocument &doc, const char *key, const String &arrayJson) {
  String wrapped = String("{\"a\":") + arrayJson + String("}");
  JsonDocument isolated;
  if (deserializeJson(isolated, wrapped) || isolated.overflowed() || !isolated["a"].is<JsonArray>()) {
    dbgprint("applyRadioConfigMerge: %s array parse failed (len=%d)", key, arrayJson.length());
    return false;
  }

  doc.remove(key);
  JsonArray dst = doc[key].to<JsonArray>();
  JsonArrayConst srcArr = isolated["a"].as<JsonArrayConst>();
  const size_t expected = srcArr.size();
  for (JsonObjectConst src : srcArr) {
    JsonObject out = dst.add<JsonObject>();
    if (out.isNull()) {
      dbgprint("applyRadioConfigMerge: %s add failed at %u/%u", key, dst.size(), expected);
      return false;
    }
    for (JsonPairConst kv : src) {
      if (kv.value().is<const char *>()) {
        out[kv.key().c_str()] = String(kv.value().as<const char *>());
      } else if (kv.value().is<bool>()) {
        out[kv.key().c_str()] = kv.value().as<bool>();
      } else if (kv.value().is<long>() || kv.value().is<int>() || kv.value().is<double>()) {
        out[kv.key().c_str()] = kv.value();
      } else if (kv.value().is<JsonArrayConst>()) {
        JsonArray nested = out[kv.key().c_str()].to<JsonArray>();
        for (JsonVariantConst v : kv.value().as<JsonArrayConst>()) {
          if (v.is<const char *>()) {
            nested.add(String(v.as<const char *>()));
          } else {
            nested.add(v);
          }
        }
      } else {
        out[kv.key().c_str()] = kv.value();
      }
    }
  }

  if (dst.size() != expected) {
    dbgprint("applyRadioConfigMerge: %s size mismatch %u!=%u", key, dst.size(), expected);
    return false;
  }

  dbgprint("applyRadioConfigMerge: applied %s (%u entries, json %d bytes)", key, dst.size(),
           arrayJson.length());
  return true;
}

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

  // Pull large array patches out as JSON text before merging scalars/objects.
  // This keeps mergeRadioConfigPatch lean and applies arrays via a dedicated path.
  String wifiJson;
  String presetsJson;
  String gpioJson;
  String alarmsJson;
  const bool patchHasWifi = patch["wifi"].is<JsonArray>();
  const bool patchHasPresets = patch["presets"].is<JsonArray>();
  const bool patchHasGpio = patch["gpio"].is<JsonArray>();
  const bool patchHasAlarms =
      patch["alarms"].is<JsonArray>() && patch["alarms"].as<JsonArray>().size() > 0;
  if (patchHasWifi) {
    serializeJson(patch["wifi"], wifiJson);
    dbgprint("applyRadioConfigMerge: wifi patch %u entries / %d bytes",
             patch["wifi"].as<JsonArray>().size(), wifiJson.length());
    patch.remove("wifi");
  } else if (!patch["wifi"].isNull()) {
    dbgprint("applyRadioConfigMerge: wifi key present but not JsonArray");
  }
  // Expose merge diagnostics to the HTTP save reply (declared in http.h).
  extern String gLastConfigSaveDetail;
  gLastConfigSaveDetail += String(" pWifi=") + String(patchHasWifi ? 1 : 0) + String(" wj=") +
                           String(wifiJson.length());
  if (patchHasPresets) {
    serializeJson(patch["presets"], presetsJson);
    patch.remove("presets");
  }
  if (patchHasGpio) {
    serializeJson(patch["gpio"], gpioJson);
    patch.remove("gpio");
  }
  if (patchHasAlarms) {
    serializeJson(patch["alarms"], alarmsJson);
    patch.remove("alarms");
  }

  // WiFi-only patches: never load the large radio_config document. Copying wifi
  // into that doc dropped entries under ESP32 heap pressure (Add Network no-op).
  // Do not use patch.size()==0: ArduinoJson may still report members after remove.
  const bool patchHasOtherScalars =
      patch["version"].is<int>() || patch["version"].is<long>() || patch["version"].is<double>() ||
      patch["volume"].is<int>() || patch["volume"].is<long>() || patch["volume"].is<double>() ||
      patch["preset"].is<int>() || patch["preset"].is<long>() || patch["preset"].is<double>() ||
      patch["clock"].is<JsonObject>() || patch["tone"].is<JsonObject>() ||
      patch["features"].is<JsonObject>();
  if (patchHasWifi && !patchHasPresets && !patchHasGpio && !patchHasAlarms && !patchHasOtherScalars) {
    int written = persistWifiJsonToNvs(wifiJson, maskWifiPasswords);
    if (written < 0) {
      eventLogRecordError("Config save failed (wifi parse)");
      return false;
    }
    gLastConfigSaveDetail += String(" wifiOnly=1 written=") + String(written);
    String rebuilt = buildRadioConfigJsonFromNvs(false);
    if (rebuilt.length() == 0 || rebuilt.length() >= RADIO_CONFIG_MAX_LEN) {
      eventLogRecordError("Config save failed (wifi blob rebuild)");
      return false;
    }
    esp_err_t blobErr = nvsSetRadioConfig(rebuilt);
    if (blobErr != ESP_OK) {
      // Blob was erased to free space for new wifi_XX; retry once after commit.
      dbgprint("applyRadioConfigMerge: wifi blob save failed (%d), retry", (int)blobErr);
      nvs_commit(nvshandle);
      blobErr = nvsSetRadioConfig(rebuilt);
    }
    if (blobErr != ESP_OK || nvsCommit() != ESP_OK) {
      // wifi_XX already holds the new networks; getconfig overlays from those
      // keys, so treat this as success even if the unified blob is temporarily
      // missing. A later config save rebuilds the blob.
      dbgprint("applyRadioConfigMerge: wifi keys saved (%d), blob deferred", written);
      gLastConfigSaveDetail += String(" blobDefer=1");
      nvsCommit();
      return true;
    }
    dbgprint("applyRadioConfigMerge: wifi-only save wrote %d networks", written);
    return true;
  }

  // Combined patches that include wifi: write wifi_XX first from the tiny JSON
  // document so a later doc→NVS pass cannot lose unseen SSIDs.
  int wifiWritten = -1;
  if (patchHasWifi) {
    wifiWritten = persistWifiJsonToNvs(wifiJson, maskWifiPasswords);
    if (wifiWritten < 0) {
      eventLogRecordError("Config save failed (wifi parse)");
      return false;
    }
    gLastConfigSaveDetail += String(" written=") + String(wifiWritten);
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
  if (!patchHasAlarms) {
    seedAlarmsFromAuthoritativeStore(doc);
  }

  mergeRadioConfigPatch(doc, patch);

  if (patchHasWifi) {
    // Mirror authoritative wifi_XX into doc for blob serialize; never trust a
    // heap-fragile deep-copy of the patch array into the large document.
    doc.remove("wifi");
    JsonArray dst = doc["wifi"].to<JsonArray>();
    char key[16];
    for (int w = 0; w < wifiWritten; w++) {
      sprintf(key, "wifi_%02d", w);
      if (!nvsKeyExists(key)) {
        break;
      }
      String entry = nvsGetValue(key);
      int sep = entry.indexOf('/');
      if (sep <= 0) {
        continue;
      }
      JsonObject obj = dst.add<JsonObject>();
      if (obj.isNull()) {
        break;
      }
      obj["ssid"] = entry.substring(0, sep);
      obj["password"] = entry.substring(sep + 1);
    }
    gLastConfigSaveDetail += String(" mirrored=") + String(dst.size());
  }
  if (patchHasPresets && !applySerializedArrayPatch(doc, "presets", presetsJson)) {
    eventLogRecordError("Config save failed (presets array)");
    return false;
  }
  if (patchHasGpio && !applySerializedArrayPatch(doc, "gpio", gpioJson)) {
    eventLogRecordError("Config save failed (gpio array)");
    return false;
  }
  if (patchHasAlarms && !applySerializedArrayPatch(doc, "alarms", alarmsJson)) {
    eventLogRecordError("Config save failed (alarms array)");
    return false;
  }

  // Never rewrite wifi_XX from the large doc here: either persistWifiJsonToNvs
  // already wrote them, or this patch must leave existing wifi_XX untouched.
  // skipWifiPersist also rebuilds doc wifi from NVS so a stale blob cannot
  // reintroduce duplicate LS rows on features-only saves.
  bool ok = applyRadioConfigDoc(doc, maskWifiPasswords, patchHasAlarms, true);
  if (!ok) {
    return false;
  }
  if (patchHasWifi) {
    // Ensure blob wifi matches wifi_XX even if doc mirror was truncated.
    String rebuilt = buildRadioConfigJsonFromNvs(false);
    if (rebuilt.length() > 0 && rebuilt.length() < RADIO_CONFIG_MAX_LEN) {
      nvsSetRadioConfig(rebuilt);
      nvsCommit();
    }
  }
  return true;
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
 * @brief Replace doc["wifi"] with the authoritative wifi_XX NVS keys.
 *
 * The radio_config blob can carry duplicate/stale SSIDs after a failed blob
 * write. getconfig must follow wifi_XX like alarms follow alrm_config.
 *
 * @param doc Parsed radio configuration document
 * @param maskPasswords If true, replace passwords with the masked placeholder
 */
static void overlayWifiFromNvsKeys(JsonDocument &doc, bool maskPasswords) {
  doc.remove("wifi");
  JsonArray wifi = doc["wifi"].to<JsonArray>();
  char key[16];
  for (uint16_t i = 0; i < 100; i++) {
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
    if (obj.isNull()) {
      break;
    }
    obj["ssid"] = entry.substring(0, sep);
    if (maskPasswords) {
      obj["password"] = RADIO_CONFIG_MASKED_PASSWORD;
    } else {
      obj["password"] = entry.substring(sep + 1);
    }
  }
}

/**
 * @brief Read unified config for web clients with masked WiFi passwords.
 *
 * Prefers the persisted `radio_config` blob to reduce NVS key scans. Falls back
 * to rebuilding from individual NVS keys only when the blob is missing/invalid.
 * WiFi always comes from wifi_XX keys so a stale blob cannot show duplicates.
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
      overlayWifiFromNvsKeys(doc, true);

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
