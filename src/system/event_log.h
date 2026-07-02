#ifndef EVENT_LOG_H
#define EVENT_LOG_H

/**
 * @file event_log.h
 * @brief Persistent log of the last boot and shutdown events.
 *
 * Keeps a small ring buffer in NVS so the user can review, on a Web UI subpage,
 * why and when the device last shut down and came back up:
 *  - Planned shutdowns (user request, firmware update, NVS erase, stall
 *    recovery) are recorded synchronously right before the restart.
 *  - Unexpected shutdowns (power loss, brownout, crash, watchdog) are inferred
 *    on the next boot from esp_reset_reason() and recorded with an unknown time.
 *  - Every boot is recorded. If the NTP clock is not yet available the entry is
 *    stored with "unknown" time and later patched once the time is synced.
 */

#include <ArduinoJson.h>
#include <esp_system.h>
#include <time.h>

#include "system/event_log_util.h"

#define EVENT_LOG_NVS_KEY "event_log"
#define EVENT_LOG_MAX_LEN 4096
#define EVENT_LOG_MAX_ENTRIES 20

/** Planned shutdown reason set before an intentional restart. */
static String eventLogPlannedReason = "";
/** True while the most recent boot entry still needs a real timestamp. */
static bool eventLogBootTimePending = false;
/** millis() when boot logging happened, used to stop retrying after a timeout. */
static uint32_t eventLogBootLoggedAt = 0;
/** Guard so the shutdown entry is written only once per restart. */
static bool eventLogShutdownWritten = false;

/**
 * @brief Read the raw event-log JSON document from NVS.
 *
 * @return String Stored JSON, or an empty structure when nothing is stored yet
 */
String eventLogGetJson() {
  static char buf[EVENT_LOG_MAX_LEN];
  size_t len = EVENT_LOG_MAX_LEN;

  nvsOpen();
  buf[0] = '\0';
  esp_err_t err = nvs_get_str(nvshandle, EVENT_LOG_NVS_KEY, buf, &len);
  if (err != ESP_OK || buf[0] == '\0') {
    return String("{\"events\":[]}");
  }
  return String(buf);
}

/**
 * @brief Persist the event-log JSON document to NVS.
 *
 * @param json Serialized event-log document
 * @return esp_err_t ESP_OK on success
 */
static esp_err_t eventLogSetJson(const String &json) {
  if (json.length() == 0 || json.length() >= EVENT_LOG_MAX_LEN) {
    return ESP_ERR_NVS_INVALID_LENGTH;
  }
  nvsOpen();
  esp_err_t err = nvs_set_str(nvshandle, EVENT_LOG_NVS_KEY, json.c_str());
  if (err == ESP_OK) {
    err = nvs_commit(nvshandle);
  }
  return err;
}

/**
 * @brief Format the current local time for a log entry.
 *
 * @param timeStr Output timestamp ("dd.mm.yyyy hh:mm:ss" or "" if unknown)
 * @param epoch   Output unix epoch seconds (0 if unknown)
 * @return true when a synced clock was available, false otherwise
 */
static bool eventLogFormatNow(String &timeStr, uint32_t &epoch) {
  if (!timeinfo.tm_year) {
    timeStr = "";
    epoch = 0;
    return false;
  }

  char buf[24];
  snprintf(buf, sizeof(buf), "%02d.%02d.%04d %02d:%02d:%02d", timeinfo.tm_mday, timeinfo.tm_mon + 1,
           timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  timeStr = String(buf);
  epoch = (uint32_t)time(nullptr);
  return true;
}

/**
 * @brief Append a single event to the persisted log.
 *
 * @param ev        Event kind ("boot" or "shutdown")
 * @param reason    Human-readable reason
 * @param timeKnown Whether the current time is valid (else logged as unknown)
 */
static void eventLogAppendPersisted(const char *ev, const char *reason, bool timeKnown) {
  JsonDocument doc;
  String stored = eventLogGetJson();
  if (deserializeJson(doc, stored) || !doc["events"].is<JsonArray>()) {
    doc.clear();
    doc["events"].to<JsonArray>();
  }

  String timeStr;
  uint32_t epoch = 0;
  bool known = timeKnown && eventLogFormatNow(timeStr, epoch);
  if (!timeKnown) {
    timeStr = "";
    epoch = 0;
  }

  eventLogAppendEntry(doc, ev, reason, FIRMWARE, timeStr.c_str(), epoch, known, EVENT_LOG_MAX_ENTRIES);

  String out;
  serializeJson(doc, out);
  eventLogSetJson(out);
}

/**
 * @brief Map an ESP reset reason to a shutdown label (for unexpected restarts).
 *
 * @param reason Reset reason from ESP-IDF
 * @return const char* Human-readable shutdown cause
 */
static const char *eventLogUnexpectedShutdownReason(esp_reset_reason_t reason) {
  switch (reason) {
  case ESP_RST_POWERON:
    return "power loss";
  case ESP_RST_BROWNOUT:
    return "brownout (power dip)";
  case ESP_RST_PANIC:
    return "crash (panic)";
  case ESP_RST_INT_WDT:
  case ESP_RST_TASK_WDT:
  case ESP_RST_WDT:
    return "watchdog reset";
  case ESP_RST_EXT:
    return "external reset";
  case ESP_RST_DEEPSLEEP:
    return "deep sleep wake";
  default:
    return "unexpected";
  }
}

/**
 * @brief Map an ESP reset reason to a boot label.
 *
 * @param reason Reset reason from ESP-IDF
 * @return const char* Human-readable boot cause
 */
static const char *eventLogBootReason(esp_reset_reason_t reason) {
  switch (reason) {
  case ESP_RST_SW:
    return "planned restart";
  case ESP_RST_POWERON:
    return "power-on";
  case ESP_RST_BROWNOUT:
    return "brownout (power dip)";
  case ESP_RST_PANIC:
    return "recovered from crash";
  case ESP_RST_INT_WDT:
  case ESP_RST_TASK_WDT:
  case ESP_RST_WDT:
    return "recovered from watchdog";
  case ESP_RST_EXT:
    return "external reset";
  case ESP_RST_DEEPSLEEP:
    return "deep sleep wake";
  default:
    return "unknown";
  }
}

/**
 * @brief Record the boot event (and any inferred unexpected shutdown).
 *
 * Call once during setup after NVS is ready. When the previous restart was not
 * a planned software restart, an "unexpected" shutdown entry (with unknown time)
 * is added before the boot entry to keep shutdown/boot pairs readable.
 */
void eventLogRecordBoot() {
  esp_reset_reason_t reason = esp_reset_reason();
  bool planned = (reason == ESP_RST_SW);

  JsonDocument doc;
  String stored = eventLogGetJson();
  if (deserializeJson(doc, stored) || !doc["events"].is<JsonArray>()) {
    doc.clear();
    doc["events"].to<JsonArray>();
  }

  if (!planned) {
    // No synchronous shutdown record exists for this restart: infer one. The
    // exact time it happened is unknown, so it is logged without a timestamp.
    eventLogAppendEntry(doc, "shutdown", eventLogUnexpectedShutdownReason(reason), FIRMWARE, "", 0, false,
                        EVENT_LOG_MAX_ENTRIES);
  }

  String timeStr;
  uint32_t epoch = 0;
  bool known = eventLogFormatNow(timeStr, epoch);
  eventLogAppendEntry(doc, "boot", eventLogBootReason(reason), FIRMWARE, timeStr.c_str(), epoch, known,
                      EVENT_LOG_MAX_ENTRIES);

  String out;
  serializeJson(doc, out);
  eventLogSetJson(out);

  eventLogBootTimePending = !known; // Patch the timestamp once NTP is available
  eventLogBootLoggedAt = millis();
}

/**
 * @brief Store the reason for the next planned shutdown/restart.
 *
 * @param reason Human-readable reason (e.g. "user request", "firmware update")
 */
void eventLogSetShutdownReason(const char *reason) {
  eventLogPlannedReason = String(reason ? reason : "");
}

/**
 * @brief Record a planned shutdown entry right before an intentional restart.
 *
 * Uses the reason set via eventLogSetShutdownReason(); defaults to "requested".
 * Safe to call more than once; only the first call per restart is written.
 */
void eventLogRecordShutdown() {
  if (eventLogShutdownWritten) {
    return;
  }
  eventLogShutdownWritten = true;

  const char *reason = eventLogPlannedReason.length() > 0 ? eventLogPlannedReason.c_str() : "requested";
  eventLogAppendPersisted("shutdown", reason, true);
}

/**
 * @brief Patch the pending boot entry once the NTP clock becomes available.
 *
 * Call regularly from the main loop. Does nothing until the boot timestamp is
 * pending and a synced clock is available, or until a timeout elapses.
 */
void eventLogProcess() {
  if (!eventLogBootTimePending) {
    return;
  }

  // Stop trying after ~5 minutes; the boot entry then stays "unknown time".
  if ((millis() - eventLogBootLoggedAt) > 300000UL) {
    eventLogBootTimePending = false;
    return;
  }

  if (!timeinfo.tm_year) {
    return; // Clock not synced yet
  }

  String timeStr;
  uint32_t epoch = 0;
  if (!eventLogFormatNow(timeStr, epoch)) {
    return;
  }

  JsonDocument doc;
  String stored = eventLogGetJson();
  if (deserializeJson(doc, stored) || !doc["events"].is<JsonArray>()) {
    eventLogBootTimePending = false;
    return;
  }

  JsonArray events = doc["events"].as<JsonArray>();
  if (events.size() == 0) {
    eventLogBootTimePending = false;
    return;
  }

  JsonObject last = events[events.size() - 1];
  if (last["ev"] == "boot" && last["known"].as<bool>() == false) {
    last["time"] = timeStr;
    last["epoch"] = epoch;
    last["known"] = true;

    String out;
    serializeJson(doc, out);
    eventLogSetJson(out);
  }

  eventLogBootTimePending = false;
}

#endif /* EVENT_LOG_H */
