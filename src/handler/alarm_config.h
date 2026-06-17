/**
 * @file alarm_config.h
 * @brief Alarm schedule configuration stored as extensible JSON in NVS.
 */

#ifndef ALARM_CONFIG_H
#define ALARM_CONFIG_H

#include <ArduinoJson.h>

#define MAX_ALARMS 16
#define ALRM_CONFIG_MAX_LEN 2048
#define ALRM_CONFIG_NVS_KEY "alrm_config"

/**
 * @brief Single alarm schedule entry.
 */
struct AlarmEntry {
  uint8_t id;
  bool enabled;
  uint8_t start_hour;
  uint8_t start_min;
  uint8_t stop_hour;
  uint8_t stop_min;
  uint8_t days_mask;
};

/**
 * @brief Parsed alarm configuration held in RAM.
 */
struct AlarmSchedule {
  uint8_t version;
  uint8_t count;
  AlarmEntry entries[MAX_ALARMS];
};

AlarmSchedule alarm_schedule;

void migrateLegacyAlarmKeys();

/**
 * @brief Read the alarm configuration JSON from NVS.
 *
 * @return String JSON document or empty string if not found
 */
String nvsGetAlarmConfig() {
  static char buf[ALRM_CONFIG_MAX_LEN];
  size_t len = ALRM_CONFIG_MAX_LEN;

  nvsOpen();
  buf[0] = '\0';
  nvserr = nvs_get_str(nvshandle, ALRM_CONFIG_NVS_KEY, buf, &len);
  if (nvserr) {
    return String("");
  }
  return String(buf);
}

/**
 * @brief Store the alarm configuration JSON in NVS.
 *
 * @param json Serialized alarm configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t nvsSetAlarmConfig(const String &json) {
  if (json.length() == 0 || json.length() >= ALRM_CONFIG_MAX_LEN) {
    dbgprint("nvsSetAlarmConfig length failed!");
    return ESP_ERR_NVS_INVALID_LENGTH;
  }

  nvsOpen();
  nvserr = nvs_set_str(nvshandle, ALRM_CONFIG_NVS_KEY, json.c_str());
  if (nvserr) {
    dbgprint("nvsSetAlarmConfig failed!");
  }
  return nvserr;
}

/**
 * @brief Convert a weekday name to a bitmask bit.
 *
 * Bit 0 = Sunday, bit 1 = Monday, ... bit 6 = Saturday (matches tm_wday).
 *
 * @param day Three-letter weekday name (e.g. "mon")
 * @return uint8_t Bitmask bit, or 0 if unknown
 */
static uint8_t dayNameToBit(const char *day) {
  if (!day) {
    return 0;
  }

  if (strcasecmp(day, "sun") == 0) {
    return 1 << 0;
  }
  if (strcasecmp(day, "mon") == 0) {
    return 1 << 1;
  }
  if (strcasecmp(day, "tue") == 0) {
    return 1 << 2;
  }
  if (strcasecmp(day, "wed") == 0) {
    return 1 << 3;
  }
  if (strcasecmp(day, "thu") == 0) {
    return 1 << 4;
  }
  if (strcasecmp(day, "fri") == 0) {
    return 1 << 5;
  }
  if (strcasecmp(day, "sat") == 0) {
    return 1 << 6;
  }

  return 0;
}

/**
 * @brief Parse a time string in H:MM or HH:MM format.
 *
 * @param time     Time string
 * @param hour     Output hour (0-23)
 * @param minute   Output minute (0-59)
 * @return true if parsing succeeded
 */
static bool parseTimeString(const char *time, uint8_t &hour, uint8_t &minute) {
  if (!time || strcmp(time, "none") == 0 || strlen(time) < 3) {
    return false;
  }

  hour = (uint8_t)getValue(String(time), ':', 0).toInt();
  minute = (uint8_t)getValue(String(time), ':', 1).toInt();
  return true;
}

/**
 * @brief Reset the in-memory alarm schedule to one default weekday alarm.
 */
void setDefaultAlarmSchedule() {
  alarm_schedule.version = 1;
  alarm_schedule.count = 1;

  AlarmEntry &entry = alarm_schedule.entries[0];
  entry.id = 1;
  entry.enabled = true;
  entry.start_hour = 6;
  entry.start_min = 30;
  entry.stop_hour = 7;
  entry.stop_min = 30;
  entry.days_mask = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5);
}

/**
 * @brief Serialize the current alarm schedule to JSON.
 *
 * @return String JSON document
 */
String serializeAlarmConfig() {
  JsonDocument doc;
  doc["version"] = alarm_schedule.version;

  JsonArray alarms = doc["alarms"].to<JsonArray>();
  char time_buf[6];

  for (uint8_t i = 0; i < alarm_schedule.count; i++) {
    const AlarmEntry &entry = alarm_schedule.entries[i];
    JsonObject obj = alarms.add<JsonObject>();

    obj["id"] = entry.id;
    obj["enabled"] = entry.enabled;

    sprintf(time_buf, "%d:%02d", entry.start_hour, entry.start_min);
    obj["start"] = time_buf;

    sprintf(time_buf, "%d:%02d", entry.stop_hour, entry.stop_min);
    obj["stop"] = time_buf;

    JsonArray days = obj["days"].to<JsonArray>();
    const char *day_names[] = {"sun", "mon", "tue", "wed", "thu", "fri", "sat"};
    for (uint8_t d = 0; d < 7; d++) {
      if (entry.days_mask & (1 << d)) {
        days.add(day_names[d]);
      }
    }
  }

  String output;
  serializeJson(doc, output);
  return output;
}

/**
 * @brief Parse alarm configuration JSON into the in-memory schedule.
 *
 * @param json JSON document
 * @return true if parsing succeeded
 */
bool parseAlarmConfig(const String &json) {
  if (json.length() == 0) {
    setDefaultAlarmSchedule();
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    dbgprint("parseAlarmConfig failed: %s", error.c_str());
    setDefaultAlarmSchedule();
    return false;
  }

  alarm_schedule.version = doc["version"] | 1;
  alarm_schedule.count = 0;

  JsonArray alarms = doc["alarms"].as<JsonArray>();
  for (JsonObject obj : alarms) {
    if (alarm_schedule.count >= MAX_ALARMS) {
      break;
    }

    AlarmEntry &entry = alarm_schedule.entries[alarm_schedule.count];
    entry.id = obj["id"] | (alarm_schedule.count + 1);
    entry.enabled = obj["enabled"] | true;

    const char *start = obj["start"] | "none";
    const char *stop = obj["stop"] | "none";
    if (!parseTimeString(start, entry.start_hour, entry.start_min)) {
      entry.start_hour = 0;
      entry.start_min = 0;
    }
    if (!parseTimeString(stop, entry.stop_hour, entry.stop_min)) {
      entry.stop_hour = 0;
      entry.stop_min = 0;
    }

    entry.days_mask = 0;
    JsonArray days = obj["days"].as<JsonArray>();
    for (const char *day : days) {
      entry.days_mask |= dayNameToBit(day);
    }

    alarm_schedule.count++;
  }

  if (alarm_schedule.count == 0) {
    setDefaultAlarmSchedule();
  }

  return true;
}

/**
 * @brief Validate alarm configuration JSON without changing runtime state.
 *
 * @param json JSON document
 * @return true if the document is valid
 */
bool isValidAlarmConfigJson(const String &json) {
  if (json.length() == 0 || json.length() >= ALRM_CONFIG_MAX_LEN) {
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    return false;
  }

  return doc["alarms"].is<JsonArray>();
}

/**
 * @brief Load alarm configuration from NVS into RAM.
 */
void loadAlarmConfigFromNvs() {
  String json = nvsGetAlarmConfig();
  if (json.length() > 0) {
    if (isValidAlarmConfigJson(json) && parseAlarmConfig(json)) {
      ini_block.alrm_config = json;
      dbgprint("BOOT: Alarm configuration loaded (%d schedule(s))", alarm_schedule.count);
      return;
    }
    dbgprint("BOOT: Invalid alrm_config in NVS, restoring default alarm schedule");
  }

  migrateLegacyAlarmKeys();
}

/**
 * @brief Migrate legacy alrm_start/alrm_stop NVS keys to JSON configuration.
 */
void migrateLegacyAlarmKeys() {
  String start = nvsKeyExists("alrm_start") ? nvsGetValue("alrm_start") : "";
  String stop = nvsKeyExists("alrm_stop") ? nvsGetValue("alrm_stop") : "";

  if (start.length() < 3 && stop.length() < 3) {
    setDefaultAlarmSchedule();
    ini_block.alrm_config = serializeAlarmConfig();
    nvsSetAlarmConfig(ini_block.alrm_config);
    return;
  }

  alarm_schedule.version = 1;
  alarm_schedule.count = 1;

  AlarmEntry &entry = alarm_schedule.entries[0];
  entry.id = 1;
  entry.enabled = true;
  if (!parseTimeString(start.c_str(), entry.start_hour, entry.start_min)) {
    entry.start_hour = 6;
    entry.start_min = 30;
  }
  if (!parseTimeString(stop.c_str(), entry.stop_hour, entry.stop_min)) {
    entry.stop_hour = 7;
    entry.stop_min = 30;
  }
  entry.days_mask = 0x7F;

  ini_block.alrm_config = serializeAlarmConfig();
  nvsSetAlarmConfig(ini_block.alrm_config);
  dbgprint("Migrated legacy alarm keys to alrm_config");
}

/**
 * @brief Apply alarm configuration from JSON and persist it to NVS.
 *
 * @param json JSON document
 * @return true if configuration was accepted
 */
bool applyAlarmConfig(const String &json) {
  if (json.length() == 0 || json.length() >= ALRM_CONFIG_MAX_LEN) {
    return false;
  }

  if (!parseAlarmConfig(json)) {
    return false;
  }

  for (uint8_t i = 0; i < alarm_schedule.count; i++) {
    const AlarmEntry &entry = alarm_schedule.entries[i];
    if (entry.enabled && entry.days_mask == 0) {
      dbgprint("Alarm %d enabled without weekdays", entry.id);
      return false;
    }
  }

  ini_block.alrm_config = json;
  return (nvsSetAlarmConfig(json) == ESP_OK);
}

#endif
