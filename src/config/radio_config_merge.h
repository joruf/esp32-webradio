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
    doc["alarms"].set(patch["alarms"]);
  }
  if (patch["wifi"].is<JsonArray>()) {
    doc["wifi"].set(patch["wifi"]);
  }
  if (patch["presets"].is<JsonArray>()) {
    doc["presets"].set(patch["presets"]);
  }
  if (patch["gpio"].is<JsonArray>()) {
    doc["gpio"].set(patch["gpio"]);
  }
}

#endif /* RADIO_CONFIG_MERGE_H */
