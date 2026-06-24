/**
 * @file live_status.h
 * @brief Shared JSON builders for getJSON, SSE, and diagnostics.
 */

#ifndef LIVE_STATUS_H
#define LIVE_STATUS_H

#include "features_config.h"
#include "title_history.h"
#include "vu_meter.h"
#include "stream_watchdog.h"
#include "ota_check.h"

/**
 * @brief Build the live status JSON payload used by polling and SSE.
 *
 * @return String JSON object body (without wrapping braces if history already included)
 */
static String buildLiveStatusJson(void) {
  String json = "{";
  json += "\"station\":\"" + jsonEscapeString(icyname.c_str()) + "\",";
  json += "\"title\":\"" + jsonEscapeString(icystreamtitle.c_str()) + "\",";
  json += "\"firmware\":\"" + jsonEscapeString(FIRMWARE) + "\",";
  json += "\"volume\":" + String(ini_block.reqvol) + ",";
  json += "\"time\":\"" + jsonEscapeString(timetxt) + "\",";
  json += "\"date\":\"" + jsonEscapeString(timetxt) + "\",";
  json += "\"ip\":\"" + jsonEscapeString(ipaddress.c_str()) + "\",";
  json += "\"rssi\":" + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + ",";
  json += "\"wifi_ssid\":\"" + jsonEscapeString(WiFi.SSID().c_str()) + "\",";
  json += "\"playing\":" + String(datamode == STOPPED ? "false" : "true") + ",";
  json += "\"dark_mode\":" + String(isDarkModeActive() ? "true" : "false") + ",";
  json += "\"snooze_sec\":" + String(sleeptimer_ms > 0 ? sleeptimer_ms / 1000 : 0) + ",";
  json += "\"vu_level\":" + String(getVuMeterLevel()) + ",";
  json += buildTitleHistoryJson();
  json += "}";
  return json;
}

/**
 * @brief Build extended diagnostics JSON.
 *
 * @return String JSON document
 */
static String buildDiagnosticsJson(void) {
  String json = "{";
  json += "\"firmware\":\"" + jsonEscapeString(FIRMWARE) + "\",";
  json += "\"heap_free\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"heap_min\":" + String(ESP.getMinFreeHeap()) + ",";
  json += "\"uptime_sec\":" + String(millis() / 1000) + ",";
  json += "\"wifi_mode\":\"" + String(WiFi.getMode() == WIFI_AP ? "AP" : "STA") + "\",";
  json += "\"ip\":\"" + jsonEscapeString(ipaddress.c_str()) + "\",";
  json += "\"rssi\":" + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + ",";
  json += "\"stream_state\":" + String((int)datamode) + ",";
  json += "\"stream_stall_ms\":" + String(getStreamStallMs()) + ",";
  json += "\"watchdog_enabled\":" + String(features.stream_watchdog ? "true" : "false") + ",";
  json += "\"vu_meter\":" + String(features.vu_meter ? "true" : "false") + ",";
  json += "\"vu_level\":" + String(getVuMeterLevel()) + ",";
  json += "\"snooze_sec\":" + String(sleeptimer_ms > 0 ? sleeptimer_ms / 1000 : 0) + ",";
  json += "\"alarm_count\":" + String(alarm_schedule.count) + ",";
  json += "\"preset\":" + String(ini_block.newpreset) + ",";
  json += "\"volume\":" + String(ini_block.reqvol) + ",";
  json += "\"ota_check_url\":\"" + jsonEscapeString(features.ota_check_url.c_str()) + "\",";
  json += "\"ota_update_available\":" + String(otaChecker.isUpdateAvailable() ? "true" : "false") + ",";
  json += "\"ota_latest_version\":\"" + jsonEscapeString(otaChecker.latestVersion().c_str()) + "\"";
  json += "}";
  return json;
}

#endif
