/**
 * @file radio_config.h
 * @brief Unified radio configuration stored as JSON in NVS.
 */

#ifndef RADIO_CONFIG_H
#define RADIO_CONFIG_H

#include <ArduinoJson.h>
#include "alarm_config.h"
#include "features_config.h"

void applyClockConfiguration(bool restartNtp);

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
  struct GpioDefault {
    int pin;
    const char *command;
  };

  static const GpioDefault defaults[] = {
      {0, "uppreset = 1"},
      {12, "upvolume = 2"},
      {13, "downvolume = 2"},
      {14, "stop"},
      {17, "resume"},
      {34, "station = icecast.omroep.nl:80/radio1-bb-mp3"},
  };

  uint16_t i;
  for (i = 0; i < sizeof(defaults) / sizeof(defaults[0]); i++) {
    JsonObject obj = gpio.add<JsonObject>();
    obj["pin"] = defaults[i].pin;
    obj["command"] = defaults[i].command;
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
  if (deserializeJson(doc, json)) {
    return false;
  }

  if (!doc["version"].is<int>()) {
    return false;
  }
  if (!doc["alarms"].is<JsonArray>()) {
    return false;
  }
  if (!doc["clock"].is<JsonObject>()) {
    return false;
  }
  if (!doc["wifi"].is<JsonArray>()) {
    return false;
  }
  if (!doc["presets"].is<JsonArray>()) {
    return false;
  }
  if (!doc["gpio"].is<JsonArray>()) {
    return false;
  }

  return true;
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

  doc["volume"] = 72;
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

  doc["volume"] = nvsKeyExists("volume") ? nvsGetValue("volume").toInt() : 72;
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
 * @brief Apply unified radio configuration JSON to NVS and runtime state.
 *
 * @param json JSON document
 * @param maskWifiPasswords If true, masked WiFi passwords are restored from NVS
 * @return true if configuration was accepted
 */
bool applyRadioConfig(const String &json, bool maskWifiPasswords) {
  if (!isValidRadioConfigJson(json)) {
    dbgprint("applyRadioConfig: invalid JSON");
    return false;
  }

  JsonDocument doc;
  if (deserializeJson(doc, json)) {
    dbgprint("applyRadioConfig: parse failed");
    return false;
  }

  uint16_t i;
  char key[16];

  nvsOpen();

  String *savedWifi = new String[100];
  if (savedWifi == nullptr) {
    dbgprint("applyRadioConfig: out of memory");
    return false;
  }

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
    if (pass.length() == 0) {
      continue;
    }
    sprintf(key, "wifi_%02d", i);
    nvsAddValue(key, String(ssid) + String("/") + pass);
    i++;
  }

  delete[] savedWifi;

  JsonObject clock = doc["clock"];
  String previousClkServer = ini_block.clk_server;
  nvsAddValue("clk_server", String(clock["server"] | "pool.ntp.org"));
  nvsAddValue("clk_offset", String(clock["offset"] | 1));
  nvsAddValue("clk_dst", String(clock["dst"] | 1));
  ini_block.clk_server = String(clock["server"] | "pool.ntp.org");
  ini_block.clk_offset = clock["offset"] | 1;
  ini_block.clk_dst = clock["dst"] | 1;
  bool restartNtp = (ini_block.clk_server != previousClkServer);

  uint8_t volume = doc["volume"] | 72;
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

  String alarmsJson = extractAlarmsJson(json);
  if (alarmsJson.length() > 0) {
    if (!applyAlarmConfig(alarmsJson)) {
      dbgprint("applyRadioConfig: alarm save failed");
      return false;
    }
  }

  applyFeaturesFromDoc(doc);

  String outputJson;
  serializeJson(doc, outputJson);
  if (outputJson.length() == 0) {
    return false;
  }

  if (nvsSetRadioConfig(outputJson) != ESP_OK) {
    return false;
  }

  ini_block.alrm_config = alarmsJson.length() > 0 ? alarmsJson : ini_block.alrm_config;
  applyClockConfiguration(restartNtp);
  return true;
}

/**
 * @brief Ensure unified radio configuration exists in NVS.
 */
void migrateToRadioConfig() {
  if (nvsKeyExists(RADIO_CONFIG_NVS_KEY)) {
    return;
  }

  dbgprint("BOOT: Migrating NVS keys to radio_config JSON");
  String json = buildRadioConfigJsonFromNvs(false);
  nvsSetRadioConfig(json);
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
