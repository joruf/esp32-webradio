/**
 * @file test_config_merge.cpp
 * @brief Unit tests for mergeRadioConfigPatch().
 *
 * These guard the "only the changed parameter is persisted" behaviour: a partial
 * patch must never disturb parameters it does not carry.
 */

#include <unity.h>

#include <ArduinoJson.h>

#include "config/radio_config_merge.h"

void setUp(void) {}
void tearDown(void) {}

/** Build a representative base configuration document. */
static void buildBase(JsonDocument &doc) {
  doc["version"] = 3;
  doc["volume"] = 42;
  doc["preset"] = 3;

  JsonObject tone = doc["tone"].to<JsonObject>();
  tone["ha"] = 1;
  tone["hf"] = 2;
  tone["la"] = 3;
  tone["lf"] = 4;

  JsonObject clock = doc["clock"].to<JsonObject>();
  clock["server"] = "pool.ntp.org";
  clock["offset"] = 1;
  clock["dst"] = 1;

  JsonObject features = doc["features"].to<JsonObject>();
  features["alarm_fade_sec"] = 15;
  features["vu_meter"] = true;

  JsonArray alarms = doc["alarms"].to<JsonArray>();
  JsonObject alarm = alarms.add<JsonObject>();
  alarm["id"] = 1;
  alarm["enabled"] = true;
}

/** A features-only patch must not change volume, preset, tone or clock. */
void test_features_patch_keeps_other_params(void) {
  JsonDocument base;
  buildBase(base);

  JsonDocument patch;
  patch["features"]["alarm_fade_sec"] = 30;

  mergeRadioConfigPatch(base, patch);

  TEST_ASSERT_EQUAL_INT(30, base["features"]["alarm_fade_sec"].as<int>());
  TEST_ASSERT_EQUAL_INT(42, base["volume"].as<int>());
  TEST_ASSERT_EQUAL_INT(3, base["preset"].as<int>());
  TEST_ASSERT_EQUAL_INT(1, base["tone"]["ha"].as<int>());
  TEST_ASSERT_TRUE(base["features"]["vu_meter"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("pool.ntp.org", base["clock"]["server"].as<const char *>());
}

/** A volume-only patch changes volume and leaves everything else intact. */
void test_volume_patch_only_changes_volume(void) {
  JsonDocument base;
  buildBase(base);

  JsonDocument patch;
  patch["volume"] = 80;

  mergeRadioConfigPatch(base, patch);

  TEST_ASSERT_EQUAL_INT(80, base["volume"].as<int>());
  TEST_ASSERT_EQUAL_INT(3, base["preset"].as<int>());
  TEST_ASSERT_EQUAL_INT(15, base["features"]["alarm_fade_sec"].as<int>());
}

/** A preset-only patch changes only the preset (station). */
void test_preset_patch_only_changes_preset(void) {
  JsonDocument base;
  buildBase(base);

  JsonDocument patch;
  patch["preset"] = 7;

  mergeRadioConfigPatch(base, patch);

  TEST_ASSERT_EQUAL_INT(7, base["preset"].as<int>());
  TEST_ASSERT_EQUAL_INT(42, base["volume"].as<int>());
}

/** Tone patches merge per field, preserving unspecified tone fields. */
void test_tone_patch_merges_per_field(void) {
  JsonDocument base;
  buildBase(base);

  JsonDocument patch;
  patch["tone"]["ha"] = 9;

  mergeRadioConfigPatch(base, patch);

  TEST_ASSERT_EQUAL_INT(9, base["tone"]["ha"].as<int>());
  TEST_ASSERT_EQUAL_INT(2, base["tone"]["hf"].as<int>());
  TEST_ASSERT_EQUAL_INT(3, base["tone"]["la"].as<int>());
  TEST_ASSERT_EQUAL_INT(4, base["tone"]["lf"].as<int>());
}

/** Clock patches merge per field, preserving unspecified clock fields. */
void test_clock_patch_merges_per_field(void) {
  JsonDocument base;
  buildBase(base);

  JsonDocument patch;
  patch["clock"]["offset"] = 2;

  mergeRadioConfigPatch(base, patch);

  TEST_ASSERT_EQUAL_INT(2, base["clock"]["offset"].as<int>());
  TEST_ASSERT_EQUAL_STRING("pool.ntp.org", base["clock"]["server"].as<const char *>());
  TEST_ASSERT_EQUAL_INT(1, base["clock"]["dst"].as<int>());
}

/** Non-empty alarms patches replace the whole array. */
void test_alarms_patch_replaces_array(void) {
  JsonDocument base;
  buildBase(base);

  JsonDocument patch;
  JsonArray alarms = patch["alarms"].to<JsonArray>();
  JsonObject a1 = alarms.add<JsonObject>();
  a1["id"] = 5;
  JsonObject a2 = alarms.add<JsonObject>();
  a2["id"] = 6;

  mergeRadioConfigPatch(base, patch);

  TEST_ASSERT_EQUAL_INT(2, base["alarms"].as<JsonArray>().size());
  TEST_ASSERT_EQUAL_INT(5, base["alarms"][0]["id"].as<int>());
}

/** An empty alarms array in a patch must not wipe existing schedules. */
void test_empty_alarms_patch_is_noop(void) {
  JsonDocument base;
  buildBase(base);
  JsonArray baseAlarms = base["alarms"].to<JsonArray>();
  baseAlarms.clear();
  JsonObject existing = baseAlarms.add<JsonObject>();
  existing["id"] = 9;

  JsonDocument patch;
  patch["alarms"].to<JsonArray>();

  mergeRadioConfigPatch(base, patch);

  TEST_ASSERT_EQUAL_INT(1, base["alarms"].as<JsonArray>().size());
  TEST_ASSERT_EQUAL_INT(9, base["alarms"][0]["id"].as<int>());
}

/** An empty patch changes nothing. */
void test_empty_patch_is_noop(void) {
  JsonDocument base;
  buildBase(base);

  JsonDocument patch;

  mergeRadioConfigPatch(base, patch);

  TEST_ASSERT_EQUAL_INT(42, base["volume"].as<int>());
  TEST_ASSERT_EQUAL_INT(3, base["preset"].as<int>());
  TEST_ASSERT_EQUAL_INT(15, base["features"]["alarm_fade_sec"].as<int>());
}

/**
 * Features string fields must be readable after a merge.
 * ArduinoJson `| nullptr` uses operator|<nullptr_t> and always returns null.
 */
void test_features_string_fields_readable_after_merge(void) {
  JsonDocument base;
  buildBase(base);
  base["features"]["alarm_fallback_sound"] = "beep";
  base["features"]["ota_check_url"] = "https://example.com/old.json";

  JsonDocument patch;
  patch["features"]["alarm_fallback_sound"] = "arcade";
  patch["features"]["alarm_fallback_interval_sec"] = 10;
  patch["features"]["ota_check_url"] = "https://example.com/new.json";

  mergeRadioConfigPatch(base, patch);

  JsonObject feat = base["features"].as<JsonObject>();
  TEST_ASSERT_EQUAL_STRING("arcade", feat["alarm_fallback_sound"].as<const char *>());
  TEST_ASSERT_EQUAL_INT(10, feat["alarm_fallback_interval_sec"].as<int>());
  TEST_ASSERT_EQUAL_STRING("https://example.com/new.json",
                           feat["ota_check_url"].as<const char *>());

  // Guard the known pitfall that broke Offline wake sound persistence.
  const char *broken = feat["alarm_fallback_sound"] | nullptr;
  TEST_ASSERT_NULL(broken);
  const char *ok = feat["alarm_fallback_sound"].as<const char *>();
  TEST_ASSERT_EQUAL_STRING("arcade", ok);
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_features_patch_keeps_other_params);
  RUN_TEST(test_volume_patch_only_changes_volume);
  RUN_TEST(test_preset_patch_only_changes_preset);
  RUN_TEST(test_tone_patch_merges_per_field);
  RUN_TEST(test_clock_patch_merges_per_field);
  RUN_TEST(test_alarms_patch_replaces_array);
  RUN_TEST(test_empty_alarms_patch_is_noop);
  RUN_TEST(test_empty_patch_is_noop);
  RUN_TEST(test_features_string_fields_readable_after_merge);
  return UNITY_END();
}
