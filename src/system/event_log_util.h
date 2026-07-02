#ifndef EVENT_LOG_UTIL_H
#define EVENT_LOG_UTIL_H

#include <ArduinoJson.h>
#include <cstdint>

/**
 * @file event_log_util.h
 * @brief Hardware-independent helpers for the boot/shutdown event log.
 *
 * The event log is stored as a JSON document of the form
 * @code {"events":[ {entry}, {entry}, ... ]} @endcode with the oldest entry
 * first and the newest entry last. These helpers only depend on ArduinoJson so
 * they can be exercised by native unit tests as well as the firmware build.
 */

/**
 * @brief Drop the oldest entries until at most @p maxEntries remain.
 *
 * @param events    Array of event entries (oldest first)
 * @param maxEntries Maximum number of entries to keep
 */
inline void eventLogTrim(JsonArray events, size_t maxEntries) {
  if (events.isNull()) {
    return;
  }
  while (events.size() > maxEntries) {
    events.remove((size_t)0); // Remove the oldest entry
  }
}

/**
 * @brief Append a single event entry and keep the ring buffer bounded.
 *
 * String values are stored by reference (ArduinoJson does not copy const char*),
 * so callers must keep the passed buffers alive until the document is
 * serialized. The array is created on demand if it does not yet exist.
 *
 * @param doc        Target event-log document ({"events":[...]})
 * @param ev         Event kind ("boot" or "shutdown")
 * @param reason     Human-readable reason
 * @param firmware   Firmware version string at the time of the event
 * @param timeStr    Formatted timestamp, or "" when the time was unknown
 * @param epoch      Unix epoch seconds (0 when unknown)
 * @param timeKnown  True when the timestamp is based on a synced clock
 * @param maxEntries Maximum number of entries to keep after appending
 */
inline void eventLogAppendEntry(JsonDocument &doc, const char *ev, const char *reason, const char *firmware,
                                const char *timeStr, uint32_t epoch, bool timeKnown, size_t maxEntries) {
  JsonArray events =
      doc["events"].is<JsonArray>() ? doc["events"].as<JsonArray>() : doc["events"].to<JsonArray>();

  JsonObject entry = events.add<JsonObject>();
  entry["ev"] = ev ? ev : "";
  entry["reason"] = reason ? reason : "";
  entry["fw"] = firmware ? firmware : "";
  entry["time"] = timeStr ? timeStr : "";
  entry["epoch"] = epoch;
  entry["known"] = timeKnown;

  eventLogTrim(events, maxEntries);
}

#endif /* EVENT_LOG_UTIL_H */
