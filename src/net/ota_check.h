/**
 * @file ota_check.h
 * @brief Check remote (or embedded) OTA manifest for firmware updates.
 */

#ifndef OTA_CHECK_H
#define OTA_CHECK_H

#include <HTTPClient.h>
#include <esp_ota_ops.h>
#include "config/features_config.h"
#include "audio/title_history.h"
#include "ota_version_embedded.h"

/**
 * @brief OTA manifest lookup, version comparison, and status JSON builder.
 */
class OtaChecker {
public:
  /**
   * @brief Query the configured OTA metadata URL for a newer firmware version.
   *
   * Falls back to the embedded manifest when the remote URL is unreachable.
   *
   * @param force If true, ignore the hourly cache interval
   */
  void check(bool force) {
    if (!force && millis() - lastCheckMs_ < 3600000UL) {
      ensureEmbeddedStatus();
      return;
    }

    lastCheckMs_ = millis();
    updateAvailable_ = false;
    manifestFromRemote_ = false;
    latestVersion_ = "";
    latestReleased_ = "";
    latestBinUrl_ = "";
    versionHistoryJson_ = "[]";
    checkError_ = "";

    bool remoteOk = false;

    if (WiFi.status() == WL_CONNECTED && radioFeatures.ota_check_url.length() > 0) {
      HTTPClient http;
      http.setTimeout(8000);
      if (!http.begin(radioFeatures.ota_check_url)) {
        checkError_ = "Invalid OTA URL";
        dbgprint("OTA check: invalid URL");
      } else {
        int code = http.GET();
        if (code == HTTP_CODE_OK) {
          String body = http.getString();
          if (applyManifestJson(body)) {
            remoteOk = true;
            manifestFromRemote_ = true;
          } else {
            checkError_ = "Invalid JSON from remote URL";
            dbgprint("OTA check: invalid JSON");
          }
        } else {
          checkError_ = "HTTP " + String(code);
          dbgprint("OTA check failed: HTTP %d", code);
        }
        http.end();
      }
    } else if (WiFi.status() != WL_CONNECTED) {
      checkError_ = "WiFi disconnected";
    } else {
      checkError_ = "No OTA URL configured";
    }

    if (!remoteOk) {
      latestVersion_ = "";
      latestReleased_ = "";
      latestBinUrl_ = "";
      versionHistoryJson_ = "[]";
      if (loadEmbeddedManifest()) {
        dbgprint("OTA check: using embedded manifest (%s)", checkError_.c_str());
      }
    }

    applyUpdateAvailability();
  }

  /**
   * @brief Build JSON for OTA status and update check results.
   *
   * @return String JSON document
   */
  String buildCheckJson(void) const {
    String json = "{";
    json += "\"current_firmware\":\"" + JsonUtil::escape(FIRMWARE) + "\",";
    json += "\"update_available\":";
    json += updateAvailable_ ? "true" : "false";
    json += ",\"remote_ok\":";
    json += manifestFromRemote_ ? "true" : "false";
    json += ",\"check_error\":\"";
    json += JsonUtil::escape(checkError_.c_str());
    json += "\",\"latest_version\":\"";
    json += JsonUtil::escape(latestVersion_.c_str());
    json += "\",\"latest_released\":\"";
    json += JsonUtil::escape(latestReleased_.c_str());
    json += "\",\"bin_url\":\"";
    json += JsonUtil::escape(latestBinUrl_.c_str());
    json += "\",\"ota_check_url\":\"";
    json += JsonUtil::escape(radioFeatures.ota_check_url.c_str());
    json += "\",\"ota_max_size\":";
    const esp_partition_t *otaPart = esp_ota_get_next_update_partition(NULL);
    json += String(otaPart ? (uint32_t)otaPart->size : 0UL);
    json += ",\"history\":";
    json += versionHistoryJson_.length() ? versionHistoryJson_ : "[]";
    json += "}";
    return json;
  }

  bool isUpdateAvailable(void) const {
    return updateAvailable_;
  }

  bool isManifestFromRemote(void) const {
    return manifestFromRemote_;
  }

  const String &latestVersion(void) const {
    return latestVersion_;
  }

  const String &latestReleased(void) const {
    return latestReleased_;
  }

  const String &latestBinUrl(void) const {
    return latestBinUrl_;
  }

  const String &checkError(void) const {
    return checkError_;
  }

  const String &versionHistoryJson(void) const {
    return versionHistoryJson_;
  }

  /**
   * @brief Build the firmware bin URL for a specific release version.
   *
   * Derives the URL from the latest bin_url template by replacing the latest
   * version token with the requested one. The available versions are discovered
   * online by the Web UI (repository listing), so this only needs to construct
   * the URL for a valid version label pointing at the same releases folder.
   * This enables installing older releases (downgrade) in addition to the newest.
   *
   * @param version Target version label (e.g. "v2.30")
   * @return String Bin URL, or empty string if the version cannot be derived
   */
  String binUrlForVersion(const String &version) const {
    if (version.length() == 0 || !version.startsWith("v")) {
      return ""; // Guard against empty/garbage version tokens
    }
    if (version == latestVersion_) {
      return latestBinUrl_;
    }
    if (latestBinUrl_.length() == 0 || latestVersion_.length() == 0) {
      return "";
    }
    if (latestBinUrl_.indexOf(latestVersion_) < 0) {
      return ""; // Template does not contain the version token to swap
    }
    String url = latestBinUrl_;
    url.replace(latestVersion_, version); // Same releases folder, different file
    return url;
  }

private:
  void applyUpdateAvailability(void) {
    updateAvailable_ = false;

    if (latestVersion_.length() == 0) {
      return;
    }

    if (compareFirmwareVersions(latestVersion_, String(FIRMWARE)) > 0) {
      updateAvailable_ = true;
      dbgprint("OTA update available: %s", latestVersion_.c_str());
    }
  }

  void ensureEmbeddedStatus(void) {
    if (latestVersion_.length() > 0) {
      return;
    }

    manifestFromRemote_ = false;
    checkError_ = "";
    if (loadEmbeddedManifest()) {
      applyUpdateAvailability();
    }
  }

  bool updateAvailable_ = false;
  bool manifestFromRemote_ = false;
  String latestVersion_;
  String latestReleased_;
  String latestBinUrl_;
  String versionHistoryJson_ = "[]";
  String checkError_;
  uint32_t lastCheckMs_ = 0;

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

  bool applyManifestJson(const String &body) {
    JsonDocument doc;
    if (deserializeJson(doc, body)) {
      return false;
    }

    latestVersion_ = String((const char *)(doc["version"] | ""));
    latestReleased_ = String((const char *)(doc["released"] | ""));
    latestBinUrl_ = String((const char *)(doc["bin_url"] | ""));
    versionHistoryJson_ = "[]";
    if (doc["history"].is<JsonArray>()) {
      serializeJson(doc["history"], versionHistoryJson_);
    }

    return latestVersion_.length() > 0;
  }

  bool loadEmbeddedManifest(void) {
    return applyManifestJson(FPSTR(ota_version_embedded_json));
  }
};

OtaChecker otaChecker;

inline void otaCheckForUpdate(bool force) {
  otaChecker.check(force);
}

inline String buildOtaCheckJson(void) {
  return otaChecker.buildCheckJson();
}

#endif
