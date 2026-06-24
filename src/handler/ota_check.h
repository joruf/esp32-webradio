/**
 * @file ota_check.h
 * @brief Check remote (or embedded) OTA manifest for firmware updates.
 */

#ifndef OTA_CHECK_H
#define OTA_CHECK_H

#include <HTTPClient.h>
#include "features_config.h"
#include "title_history.h"
#include "ota_version_embedded.h"

bool otaUpdateAvailable = false;
bool otaManifestFromRemote = false;
String otaLatestVersion = "";
String otaLatestReleased = "";
String otaLatestBinUrl = "";
String otaVersionHistoryJson = "[]";
String otaCheckError = "";

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
 * @brief Parse an OTA manifest JSON document into runtime fields.
 *
 * @param body Raw JSON body
 * @return true if a version field was found
 */
static bool applyOtaManifestJson(const String &body) {
  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    return false;
  }

  otaLatestVersion = String((const char *)(doc["version"] | ""));
  otaLatestReleased = String((const char *)(doc["released"] | ""));
  otaLatestBinUrl = String((const char *)(doc["bin_url"] | ""));
  otaVersionHistoryJson = "[]";
  if (doc["history"].is<JsonArray>()) {
    serializeJson(doc["history"], otaVersionHistoryJson);
  }

  return otaLatestVersion.length() > 0;
}

/**
 * @brief Load the firmware-bundled OTA manifest.
 *
 * @return true if parsing succeeded
 */
static bool loadEmbeddedOtaManifest(void) {
  return applyOtaManifestJson(FPSTR(ota_version_embedded_json));
}

/**
 * @brief Query the configured OTA metadata URL for a newer firmware version.
 *
 * Falls back to the embedded manifest when the remote URL is unreachable.
 *
 * @param force If true, ignore the hourly cache interval
 */
void otaCheckForUpdate(bool force) {
  if (!force && millis() - otaLastCheckMs < 3600000UL) {
    return;
  }

  otaLastCheckMs = millis();
  otaUpdateAvailable = false;
  otaManifestFromRemote = false;
  otaLatestVersion = "";
  otaLatestReleased = "";
  otaLatestBinUrl = "";
  otaVersionHistoryJson = "[]";
  otaCheckError = "";

  bool remoteOk = false;

  if (WiFi.status() == WL_CONNECTED && features.ota_check_url.length() > 0) {
    HTTPClient http;
    http.setTimeout(8000);
    if (!http.begin(features.ota_check_url)) {
      otaCheckError = "Invalid OTA URL";
      dbgprint("OTA check: invalid URL");
    } else {
      int code = http.GET();
      if (code == HTTP_CODE_OK) {
        String body = http.getString();
        if (applyOtaManifestJson(body)) {
          remoteOk = true;
          otaManifestFromRemote = true;
        } else {
          otaCheckError = "Invalid JSON from remote URL";
          dbgprint("OTA check: invalid JSON");
        }
      } else {
        otaCheckError = "HTTP " + String(code);
        dbgprint("OTA check failed: HTTP %d", code);
      }
      http.end();
    }
  } else if (WiFi.status() != WL_CONNECTED) {
    otaCheckError = "WiFi disconnected";
  } else {
    otaCheckError = "No OTA URL configured";
  }

  if (!remoteOk) {
    otaLatestVersion = "";
    otaLatestReleased = "";
    otaLatestBinUrl = "";
    otaVersionHistoryJson = "[]";
    if (loadEmbeddedOtaManifest()) {
      dbgprint("OTA check: using embedded manifest (%s)", otaCheckError.c_str());
    }
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
  json += ",\"remote_ok\":";
  json += otaManifestFromRemote ? "true" : "false";
  json += ",\"check_error\":\"";
  json += jsonEscapeString(otaCheckError.c_str());
  json += "\",\"latest_version\":\"";
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
