/**
 * @file live_status.h
 * @brief Shared JSON builders for getJSON, SSE, and diagnostics.
 */

#ifndef LIVE_STATUS_H
#define LIVE_STATUS_H

#include "config/features_config.h"
#include "audio/title_history.h"
#include "audio/vu_meter.h"
#include "audio/stream_watchdog.h"
#include "ota_check.h"
#include <esp_ota_ops.h>
#include <esp_chip_info.h>
#include <esp_system.h>
#include <rom/rtc.h>

/**
 * @brief Convert WiFi mode enum to readable string.
 *
 * @param mode Current WiFi mode
 * @return const char* Human-readable mode label
 */
static const char *diagWifiModeToString(wifi_mode_t mode) {
  switch (mode) {
  case WIFI_OFF:
    return "OFF";
  case WIFI_STA:
    return "STA";
  case WIFI_AP:
    return "AP";
  case WIFI_AP_STA:
    return "AP_STA";
  default:
    return "UNKNOWN";
  }
}

/**
 * @brief Convert reset reason enum to readable string.
 *
 * @param reason Reset reason from ESP-IDF
 * @return const char* Human-readable reason label
 */
static const char *diagResetReasonToString(esp_reset_reason_t reason) {
  switch (reason) {
  case ESP_RST_UNKNOWN:
    return "UNKNOWN";
  case ESP_RST_POWERON:
    return "POWERON";
  case ESP_RST_EXT:
    return "EXTERNAL_PIN";
  case ESP_RST_SW:
    return "SOFTWARE";
  case ESP_RST_PANIC:
    return "PANIC";
  case ESP_RST_INT_WDT:
    return "INT_WDT";
  case ESP_RST_TASK_WDT:
    return "TASK_WDT";
  case ESP_RST_WDT:
    return "OTHER_WDT";
  case ESP_RST_DEEPSLEEP:
    return "DEEPSLEEP";
  case ESP_RST_BROWNOUT:
    return "BROWNOUT";
  case ESP_RST_SDIO:
    return "SDIO";
  default:
    return "UNMAPPED";
  }
}

/**
 * @brief Append partition information as a nested JSON object.
 *
 * @param json Target JSON string
 * @param keyName Object key name
 * @param part Partition pointer
 */
static void diagAppendPartitionJson(String &json, const char *keyName, const esp_partition_t *part) {
  json += "\"" + String(keyName) + "\":";
  if (!part) {
    json += "null";
    return;
  }

  json += "{";
  json += "\"label\":\"" + jsonEscapeString(part->label) + "\",";
  json += "\"type\":" + String((int)part->type) + ",";
  json += "\"subtype\":" + String((int)part->subtype) + ",";
  json += "\"address\":" + String((uint32_t)part->address) + ",";
  json += "\"size\":" + String((uint32_t)part->size);
  json += "}";
}

/**
 * @brief Build the live status JSON payload used by polling and SSE.
 *
 * @return String JSON object body (without wrapping braces if history already included)
 */
static String buildLiveStatusJson(void) {
  String json = "{";
  String stationName = metaGetStation();
  String songTitle = metaGetTitle();
  json += "\"station\":\"" + jsonEscapeString(stationName.c_str()) + "\",";
  json += "\"title\":\"" + jsonEscapeString(songTitle.c_str()) + "\",";
  json += "\"firmware\":\"" + jsonEscapeString(FIRMWARE) + "\",";
  json += "\"volume\":" + String(ini_block.reqvol) + ",";
  json += "\"time\":\"" + jsonEscapeString(timetxt) + "\",";
  json += "\"date\":\"" + jsonEscapeString(timetxt) + "\",";
  json += "\"ip\":\"" + jsonEscapeString(ipaddress.c_str()) + "\",";
  json += "\"rssi\":" + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + ",";
  json += "\"wifi_ssid\":\"" + jsonEscapeString(WiFi.SSID().c_str()) + "\",";
  json += "\"playing\":" + String(datamode == STOPPED ? "false" : "true") + ",";
  json += "\"playback_state\":\"" + String(datamode == STOPPED ? "STOP" : "PLAY") + "\",";
  json += "\"stream_kbps\":" + String(mbitrate > 0 ? mbitrate : bitrate) + ",";
  json += "\"dark_mode\":" + String(isDarkModeActive() ? "true" : "false") + ",";
  json += "\"snooze_sec\":" + String(sleeptimer_ms > 0 ? sleeptimer_ms / 1000 : 0) + ",";
  json += "\"vu_level\":" + String(getVuMeterLevel()) + ",";
  json += "\"preset\":" + String(ini_block.newpreset) + ",";
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
  esp_chip_info_t chipInfo;
  esp_chip_info(&chipInfo);

  const wifi_mode_t wifiMode = WiFi.getMode();
  const wl_status_t wifiStatus = WiFi.status();
  const esp_reset_reason_t resetReason = esp_reset_reason();
  const esp_partition_t *runningPart = esp_ota_get_running_partition();
  const esp_partition_t *bootPart = esp_ota_get_boot_partition();
  const esp_partition_t *otaSlot = esp_ota_get_next_update_partition(NULL);
  const int cpu0ResetCode = (int)esp_rom_get_reset_reason(0);
  const int cpu1ResetCode = (int)esp_rom_get_reset_reason(1);
  const uint32_t uptimeSec = millis() / 1000UL;
  const uint32_t uptimeDays = uptimeSec / 86400UL;
  const uint32_t uptimeHours = (uptimeSec % 86400UL) / 3600UL;
  const uint32_t uptimeMinutes = (uptimeSec % 3600UL) / 60UL;

  String json = "{";
  json += "\"firmware\":\"" + jsonEscapeString(FIRMWARE) + "\",";
  json += "\"build_date\":\"" + jsonEscapeString(__DATE__) + "\",";
  json += "\"build_time\":\"" + jsonEscapeString(__TIME__) + "\",";
  json += "\"sdk_version\":\"" + jsonEscapeString(ESP.getSdkVersion()) + "\",";
  json += "\"chip_model\":\"" + jsonEscapeString(ESP.getChipModel()) + "\",";
  json += "\"chip_revision\":" + String((int)chipInfo.revision) + ",";
  json += "\"chip_cores\":" + String((int)chipInfo.cores) + ",";
  json += "\"cpu_freq_mhz\":" + String((int)ESP.getCpuFreqMHz()) + ",";
  json += "\"reset_reason\":\"" + jsonEscapeString(diagResetReasonToString(resetReason)) + "\",";
  json += "\"reset_reason_code\":" + String((int)resetReason) + ",";
  json += "\"reset_reason_cpu0_code\":" + String(cpu0ResetCode) + ",";
  json += "\"reset_reason_cpu1_code\":" + String(cpu1ResetCode) + ",";
  json += "\"flash_chip_size\":" + String((uint32_t)ESP.getFlashChipSize()) + ",";
  json += "\"flash_chip_speed\":" + String((uint32_t)ESP.getFlashChipSpeed()) + ",";
  json += "\"flash_chip_mode\":" + String((int)ESP.getFlashChipMode()) + ",";
  json += "\"sketch_size\":" + String((uint32_t)ESP.getSketchSize()) + ",";
  json += "\"free_sketch_space\":" + String((uint32_t)ESP.getFreeSketchSpace()) + ",";
  json += "\"heap_total\":" + String((uint32_t)ESP.getHeapSize()) + ",";
  json += "\"heap_free\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"heap_min\":" + String(ESP.getMinFreeHeap()) + ",";
  json += "\"heap_largest_block\":" + String((uint32_t)ESP.getMaxAllocHeap()) + ",";
  json += "\"psram_size\":" + String((uint32_t)ESP.getPsramSize()) + ",";
  json += "\"psram_free\":" + String((uint32_t)ESP.getFreePsram()) + ",";
  json += "\"uptime_sec\":" + String(uptimeSec) + ",";
  json += "\"uptime_pretty\":\"" + String(uptimeDays) + "d " + String(uptimeHours) + "h " + String(uptimeMinutes) + "m\",";
  json += "\"wifi_mode\":\"" + String(diagWifiModeToString(wifiMode)) + "\",";
  json += "\"wifi_status\":" + String((int)wifiStatus) + ",";
  json += "\"wifi_connected\":" + String(wifiStatus == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"wifi_ssid\":\"" + jsonEscapeString(WiFi.SSID().c_str()) + "\",";
  json += "\"wifi_bssid\":\"" + jsonEscapeString(WiFi.BSSIDstr().c_str()) + "\",";
  json += "\"wifi_channel\":" + String((int)WiFi.channel()) + ",";
  json += "\"wifi_mac\":\"" + jsonEscapeString(WiFi.macAddress().c_str()) + "\",";
  json += "\"hostname\":\"" + jsonEscapeString(WiFi.getHostname() ? WiFi.getHostname() : "") + "\",";
  json += "\"ip\":\"" + jsonEscapeString(ipaddress.c_str()) + "\",";
  json += "\"gateway\":\"" + jsonEscapeString(WiFi.gatewayIP().toString().c_str()) + "\",";
  json += "\"subnet\":\"" + jsonEscapeString(WiFi.subnetMask().toString().c_str()) + "\",";
  json += "\"dns\":\"" + jsonEscapeString(WiFi.dnsIP().toString().c_str()) + "\",";
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
  json += "\"ota_latest_version\":\"" + jsonEscapeString(otaChecker.latestVersion().c_str()) + "\",";
  json += "\"ota_capable\":" + String(otaSlot ? "true" : "false") + ",";
  json += "\"ota_slot_bytes\":" + String(otaSlot ? otaSlot->size : 0) + ",";
  json += "\"partitions\":{";
  diagAppendPartitionJson(json, "running", runningPart);
  json += ",";
  diagAppendPartitionJson(json, "boot", bootPart);
  json += ",";
  diagAppendPartitionJson(json, "next_ota", otaSlot);
  json += "},";
  json += "\"sketch_bytes\":" + String(ESP.getSketchSize());
  json += "}";
  return json;
}

#endif
