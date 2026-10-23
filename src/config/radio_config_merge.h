#ifndef RADIO_CONFIG_MERGE_H
#define RADIO_CONFIG_MERGE_H

#include <ArduinoJson.h>

/**
 * @file radio_config_merge.h
 * @brief Pure JSON merge helper for the unified radio configuration.
 *
 * This logic only depends on ArduinoJson (no hardware/NVS access), so it can be
 * exercised directly by native unit tests as well as the firmware build.
 */

/**
 * @brief Replace a document array with a deep copy of a patch array.
 *
 * Removes the old array first so ArduinoJson can reclaim memory before
 * allocating the copy. A plain JsonArray::set() on a large base document often
 * fails under ESP32 heap pressure and silently leaves the old array in place
 * (WiFi / presets / GPIO saves appeared to succeed but never persisted).
 *
 * @param doc   Target document
 * @param key   Top-level array key
 * @param patch Source array (must remain valid for the duration of the copy)
 * @return true if the array was replaced
 */
inline bool replaceRadioConfigArray(JsonDocument &doc, const char *key, JsonArrayConst patch) {
  // Remove first so destination memory can be reused. Copy object entries
  // field-by-field (avoids JsonVariant deep-copy failures on large docs).
  doc.remove(key);
  JsonArray target = doc[key].to<JsonArray>();
  for (JsonVariantConst item : patch) {
    if (item.is<JsonObjectConst>()) {
      JsonObjectConst obj = item.as<JsonObjectConst>();
      JsonObject out = target.add<JsonObject>();
      for (JsonPairConst kv : obj) {
        out[kv.key().c_str()] = kv.value();
      }
    } else if (!target.add(item) && !item.isNull()) {
      return false;
    }
  }
  return true;
}

/**
 * @brief Merge only the keys present in @p patch into @p doc.
 *
 * Scalar keys (version, volume, preset) and object keys (clock, tone, features)
 * are shallow-merged per field. Array keys (alarms, wifi, presets, gpio) are
 * replaced wholesale when present. Keys absent from @p patch are left untouched
 * so a partial patch never disturbs unrelated parameters.
 *
 * @param doc   Target document (modified in place)
 * @param patch Patch document with one or more top-level keys
 */
inline void mergeRadioConfigPatch(JsonDocument &doc, JsonDocument &patch) {
  if (patch["version"].is<int>() || patch["version"].is<long>() || patch["version"].is<double>()) {
    doc["version"] = patch["version"];
  }
  if (patch["volume"].is<int>() || patch["volume"].is<long>() || patch["volume"].is<double>()) {
    doc["volume"] = patch["volume"];
  }
  if (patch["preset"].is<int>() || patch["preset"].is<long>() || patch["preset"].is<double>()) {
    doc["preset"] = patch["preset"];
  }

  if (patch["clock"].is<JsonObject>()) {
    // Preserve existing fields: only overwrite the keys carried by the patch.
    JsonObject target =
        doc["clock"].is<JsonObject>() ? doc["clock"].as<JsonObject>() : doc["clock"].to<JsonObject>();
    for (JsonPair kv : patch["clock"].as<JsonObject>()) {
      target[kv.key().c_str()] = kv.value();
    }
  }

  if (patch["tone"].is<JsonObject>()) {
    JsonObject target =
        doc["tone"].is<JsonObject>() ? doc["tone"].as<JsonObject>() : doc["tone"].to<JsonObject>();
    for (JsonPair kv : patch["tone"].as<JsonObject>()) {
      target[kv.key().c_str()] = kv.value();
    }
  }

  if (patch["features"].is<JsonObject>()) {
    JsonObject target =
        doc["features"].is<JsonObject>() ? doc["features"].as<JsonObject>() : doc["features"].to<JsonObject>();
    for (JsonPair kv : patch["features"].as<JsonObject>()) {
      target[kv.key().c_str()] = kv.value();
    }
  }

  if (patch["alarms"].is<JsonArray>()) {
    JsonArrayConst patchAlarms = patch["alarms"].as<JsonArrayConst>();
    // An empty array means "no alarm changes" (partial save), not "delete all alarms".
    if (patchAlarms.size() > 0) {
      replaceRadioConfigArray(doc, "alarms", patchAlarms);
    }
  }
  if (patch["wifi"].is<JsonArray>()) {
    replaceRadioConfigArray(doc, "wifi", patch["wifi"].as<JsonArrayConst>());
  }
  if (patch["presets"].is<JsonArray>()) {
    replaceRadioConfigArray(doc, "presets", patch["presets"].as<JsonArrayConst>());
  }
  if (patch["gpio"].is<JsonArray>()) {
    replaceRadioConfigArray(doc, "gpio", patch["gpio"].as<JsonArrayConst>());
  }
}

#endif /* RADIO_CONFIG_MERGE_H */
