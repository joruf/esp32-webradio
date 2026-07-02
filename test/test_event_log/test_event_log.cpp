/**
 * @file test_event_log.cpp
 * @brief Unit tests for the boot/shutdown event-log ring buffer helpers.
 */

#include <unity.h>

#include <ArduinoJson.h>

#include "system/event_log_util.h"

void setUp(void) {}
void tearDown(void) {}

/** Appending keeps oldest-first order and stores all fields. */
void test_append_stores_entry(void) {
  JsonDocument doc;

  eventLogAppendEntry(doc, "boot", "power-on", "v1.0", "01.01.2026 00:00:00", 100, true, 20);

  JsonArray events = doc["events"].as<JsonArray>();
  TEST_ASSERT_EQUAL_INT(1, events.size());
  TEST_ASSERT_EQUAL_STRING("boot", events[0]["ev"].as<const char *>());
  TEST_ASSERT_EQUAL_STRING("power-on", events[0]["reason"].as<const char *>());
  TEST_ASSERT_EQUAL_STRING("v1.0", events[0]["fw"].as<const char *>());
  TEST_ASSERT_EQUAL_UINT32(100, events[0]["epoch"].as<uint32_t>());
  TEST_ASSERT_TRUE(events[0]["known"].as<bool>());
}

/** Entries are appended in call order (oldest first, newest last). */
void test_append_preserves_order(void) {
  JsonDocument doc;

  eventLogAppendEntry(doc, "shutdown", "user request", "v1.0", "", 0, false, 20);
  eventLogAppendEntry(doc, "boot", "planned restart", "v1.0", "", 0, false, 20);

  JsonArray events = doc["events"].as<JsonArray>();
  TEST_ASSERT_EQUAL_INT(2, events.size());
  TEST_ASSERT_EQUAL_STRING("shutdown", events[0]["ev"].as<const char *>());
  TEST_ASSERT_EQUAL_STRING("boot", events[1]["ev"].as<const char *>());
}

/** An unknown time is stored as empty string with known=false. */
void test_unknown_time_entry(void) {
  JsonDocument doc;

  eventLogAppendEntry(doc, "shutdown", "power loss", "v1.0", "", 0, false, 20);

  JsonArray events = doc["events"].as<JsonArray>();
  TEST_ASSERT_EQUAL_STRING("", events[0]["time"].as<const char *>());
  TEST_ASSERT_FALSE(events[0]["known"].as<bool>());
}

/** Appending beyond the cap drops the oldest entries (ring buffer). */
void test_ring_buffer_drops_oldest(void) {
  JsonDocument doc;
  const size_t maxEntries = 5;

  for (int i = 0; i < 8; i++) {
    char reason[8];
    snprintf(reason, sizeof(reason), "r%d", i);
    eventLogAppendEntry(doc, "boot", reason, "v1.0", "", (uint32_t)i, false, maxEntries);
  }

  JsonArray events = doc["events"].as<JsonArray>();
  TEST_ASSERT_EQUAL_INT((int)maxEntries, events.size());
  // The three oldest (0,1,2) were dropped; the window is now 3..7.
  TEST_ASSERT_EQUAL_UINT32(3, events[0]["epoch"].as<uint32_t>());
  TEST_ASSERT_EQUAL_UINT32(7, events[events.size() - 1]["epoch"].as<uint32_t>());
}

/** eventLogTrim() reduces an oversized array to the cap, keeping newest. */
void test_trim_directly(void) {
  JsonDocument doc;
  JsonArray events = doc["events"].to<JsonArray>();
  for (int i = 0; i < 10; i++) {
    events.add<JsonObject>()["epoch"] = i;
  }

  eventLogTrim(events, 4);

  TEST_ASSERT_EQUAL_INT(4, events.size());
  TEST_ASSERT_EQUAL_UINT32(6, events[0]["epoch"].as<uint32_t>());
  TEST_ASSERT_EQUAL_UINT32(9, events[3]["epoch"].as<uint32_t>());
}

/** Trimming a shorter-than-cap array leaves it unchanged. */
void test_trim_noop_when_small(void) {
  JsonDocument doc;
  JsonArray events = doc["events"].to<JsonArray>();
  events.add<JsonObject>()["epoch"] = 1;
  events.add<JsonObject>()["epoch"] = 2;

  eventLogTrim(events, 20);

  TEST_ASSERT_EQUAL_INT(2, events.size());
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_append_stores_entry);
  RUN_TEST(test_append_preserves_order);
  RUN_TEST(test_unknown_time_entry);
  RUN_TEST(test_ring_buffer_drops_oldest);
  RUN_TEST(test_trim_directly);
  RUN_TEST(test_trim_noop_when_small);
  return UNITY_END();
}
