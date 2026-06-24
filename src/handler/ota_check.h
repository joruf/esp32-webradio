/**
 * @file ota_check.h
 * @brief Check GitHub (or custom URL) for newer firmware releases.
 */

#ifndef OTA_CHECK_H
#define OTA_CHECK_H

#include <HTTPClient.h>
#include "features_config.h"

bool otaUpdateAvailable = false;
String otaLatestVersion = "";
String otaLatestReleased = "";
String otaLatestBinUrl = "";
String otaVersionHistoryJson = "[]";

static uint32_t otaLastCheckMs = 0;

/**
 * @brief Compare dotted firmware version strings (major.minor).
 *
 * @param remote Remote version label
 * @param local  Local FIRMWARE macro value
 * @return int Positive if remote is newer
 */
static int compareFirmwareVersions(const String &remote, const String &local) {
  float remoteVal = 0.0f;
  float localVal = 0.0f;

  int remotePos = remote.indexOf('v');
  if (remotePos >= 0) {
    remoteVal = remote.substring(remotePos + 1).toFloat();
  }
  int localPos = String(FIRMWARE).indexOf('v');
  if (localPos >= 0) {
    localVal = String(FIRMWARE).substring(localPos + 1).toFloat();
  }

  if (remoteVal > localVal) {
    return 1;
  }
  if (remoteVal < localVal) {
    return -1;
  }
  return 0;
}

/**
 * @brief Query the configured OTA metadata URL for a newer firmware version.
 *
 * @param force If true, ignore the hourly cache interval
 */
void otaCheckForUpdate(bool force) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!force && millis() - otaLastCheckMs < 3600000UL) {
    return;
  }

  otaLastCheckMs = millis();
  otaUpdateAvailable = false;
  otaLatestVersion = "";
  otaLatestReleased = "";
  otaLatestBinUrl = "";
  otaVersionHistoryJson = "[]";

  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(features.ota_check_url)) {
    dbgprint("OTA check: invalid URL");
    return;
  }

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    dbgprint("OTA check failed: HTTP %d", code);
    http.end();
    return;
  }

  String body = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    dbgprint("OTA check: invalid JSON");
    return;
  }

  otaLatestVersion = String((const char *)(doc["version"] | ""));
  otaLatestReleased = String((const char *)(doc["released"] | ""));
  otaLatestBinUrl = String((const char *)(doc["bin_url"] | ""));
  if (doc["history"].is<JsonArray>()) {
    serializeJson(doc["history"], otaVersionHistoryJson);
  }
  if (otaLatestVersion.length() == 0) {
    return;
  }

  if (compareFirmwareVersions(otaLatestVersion, String(FIRMWARE)) > 0) {
    otaUpdateAvailable = true;
    dbgprint("OTA update available: %s", otaLatestVersion.c_str());
  }
}

/**
 * @brief Build JSON for OTA status and update check results.
 *
 * @return String JSON document
 */
static String buildOtaCheckJson(void) {
  String json = "{";
  json += "\"current_firmware\":\"" + jsonEscapeString(FIRMWARE) + "\",";
  json += "\"update_available\":";
  json += otaUpdateAvailable ? "true" : "false";
  json += ",\"latest_version\":\"";
  json += jsonEscapeString(otaLatestVersion.c_str());
  json += "\",\"latest_released\":\"";
  json += jsonEscapeString(otaLatestReleased.c_str());
  json += "\",\"bin_url\":\"";
  json += jsonEscapeString(otaLatestBinUrl.c_str());
  json += "\",\"ota_check_url\":\"";
  json += jsonEscapeString(features.ota_check_url.c_str());
  json += "\",\"history\":";
  json += otaVersionHistoryJson.length() ? otaVersionHistoryJson : "[]";
  json += "}";
  return json;
}

#endif
