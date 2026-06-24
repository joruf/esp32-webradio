// OTA-Update: https://github.com/esp8266/Arduino/issues/4400

#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <esp_ota_ops.h>

#include "display/dsp_ota_display.h"
#include "ota_check.h"

/**
 * @brief Check whether the partition table provides an OTA slot large enough.
 *
 * @param imageSize Expected firmware size in bytes, or 0 to skip size check
 * @return true when HTTP/OTA firmware updates can be written
 */
inline bool isOtaUpdateCapable(size_t imageSize = 0) {
  const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
  if (!part) {
    return false;
  }
  if (imageSize > 0 && imageSize > part->size) {
    return false;
  }
  return true;
}

/**
 * @brief Scan for content-length information in header line.
 *
 * @param metalinebf Header line to scan
 */
void scan_content_length(const char *metalinebf) {
  if (strstr(metalinebf, "Content-Length")) {
    clength = atoi(metalinebf + 15);
    dbgprint("Content-Length is %d", clength);
    if (clength > 0) {
      dspOtaDisplaySetTotalBytes((size_t)clength);
    }
  }
}

/**
 * @brief Write an OTA stream to flash with on-screen progress updates.
 *
 * @param stream Network stream to read from
 * @param totalBytes Expected total size, or 0 if unknown
 * @param phaseLabel Label shown during this phase
 * @return size_t Bytes written to the OTA partition
 */
static size_t otaWriteStreamWithProgress(Stream &stream, size_t totalBytes, const char *phaseLabel) {
  uint8_t buff[1024];
  size_t written = 0;
  uint32_t lastUiMs = 0;
  uint8_t lastPct = 255;

  dspOtaDisplayPhase(phaseLabel);

  uint32_t noDataSince = millis();

  while (true) {
    int avail = stream.available();
    if (avail <= 0) {
      if (totalBytes > 0U && written >= totalBytes) {
        break;
      }
      if ((millis() - noDataSince) > 60000U) {
        break;
      }
      delay(1);
      continue;
    }

    noDataSince = millis();
    size_t chunk = stream.readBytes(buff, (size_t)avail > sizeof(buff) ? sizeof(buff) : (size_t)avail);
    if (chunk == 0) {
      delay(1);
      continue;
    }

    size_t nw = Update.write(buff, chunk);
    if (nw != chunk) {
      dbgprint("OTA write error at %u", (unsigned)written);
      break;
    }

    written += nw;

    uint32_t now = millis();
    uint8_t pct = 255;
    if (totalBytes > 0) {
      pct = (uint8_t)min(100UL, (written * 100UL) / totalBytes);
    }
    if (pct != lastPct || (now - lastUiMs) >= 350U) {
      dspOtaDisplayProgress(written, totalBytes, phaseLabel);
      lastUiMs = now;
      lastPct = pct;
    }
  }

  return written;
}

/**
 * @brief Show the OTA start screen using manifest data when available.
 *
 * @param totalBytes Known firmware size, or 0
 */
static void otaShowStartScreen(size_t totalBytes) {
  const char *target = otaChecker.latestVersion().c_str();
  if (target[0] == '\0') {
    target = nullptr;
  }
  dspOtaDisplayBegin(target, FIRMWARE, totalBytes);
}

/**
 * @brief Update via WiFi has been started by Arduino IDE or update request.
 */
void otastart() {
  dbgprint("OTA update Started");
}

/**
 * @brief Wait for NEXTION ACK response.
 *
 * @param timeout Maximum time to wait in milliseconds
 * @return int The received ACK byte, or -1 on timeout
 */
static int waitForNextionAck(uint32_t timeout) {
  uint32_t start = millis();
  while (!nxtserial->available()) {
    if (millis() - start > timeout) {
      return -1;
    }
    delay(20);
  }
  return nxtserial->read();
}

/**
 * @brief Update NEXTION image from OTA stream.
 *
 * @param clength Size of the update data
 * @return true if update was successful, false otherwise
 */
bool do_nextion_update(uint32_t clength) {
  bool res = false;
  uint32_t k;
  int c;
  uint32_t remaining = clength;

  if (!nxtserial) {
    return false;
  }

  vTaskDelete(xspftask);
  delay(1000);

  nxtserial->printf("\xFF\xFF\xFF");

  for (int i = 0; i < 100; i++) {
    if (nxtserial->available()) {
      nxtserial->read();
    }
    delay(20);
  }

  nxtserial->printf("whmi-wri %d,9600,0\xFF\xFF\xFF", clength);

  c = waitForNextionAck(2000);
  if (c != 0x05) {
    return false;
  }

  dspOtaDisplaySetTotalBytes(clength);
  dspOtaDisplayPhase("Updating display...");

  while (remaining) {
    k = (remaining > 4096U) ? 4096U : remaining;
    k = otaclient.read(tmpbuff, k);
    dbgprint("TFT file, read %d bytes", k);
    nxtserial->write(tmpbuff, k);

    c = waitForNextionAck(2000);
    if (c != 0x05) {
      break;
    }
    remaining -= k;
    dspOtaDisplayProgress(clength - remaining, clength, "Updating display...");
  }

  otaclient.flush();

  if (remaining == 0) {
    dbgprint("Update successfully completed");
    res = true;
  }

  return res;
}

/**
 * @brief Update software from OTA stream.
 *
 * @param clength Size of the update data
 * @return true if update was successful, false otherwise
 */
bool do_software_update(uint32_t clength) {
  bool res = false;
  size_t written;

  if (!Update.begin(clength)) {
    dbgprint("Not enough space to begin OTA: %s", Update.errorString());
    otaclient.flush();
    dspOtaDisplayFailed(Update.errorString());
    return false;
  }

  dbgprint("Begin OTA update, length is %d", clength);
  dspOtaDisplaySetTotalBytes(clength);
  dspOtaDisplayPhase("Installing...");

  written = otaWriteStreamWithProgress(otaclient, clength, "Installing...");
  otaclient.flush();

  if (written != clength) {
    dbgprint("Write failed! %u of %u", (unsigned)written, (unsigned)clength);
    Update.abort();
    dspOtaDisplayFailed("Write failed");
    return false;
  }

  dbgprint("Written %u bytes successfully", (unsigned)written);

  if (Update.end()) {
    dbgprint("OTA done");
    if (Update.isFinished()) {
      dbgprint("Update successfully completed");
      res = true;
    } else {
      dbgprint("Update not finished!");
      dspOtaDisplayFailed("Not finished");
    }
  } else {
    dbgprint("Error Occurred. Error %s", Update.getError());
    dspOtaDisplayFailed(Update.errorString());
  }

  return res;
}

/**
 * @brief Parse HTTP response headers for OTA update.
 *
 * @param line Current header line
 * @param newlstmod Output buffer for Last-Modified timestamp
 * @return true if headers are complete (empty line), false otherwise
 */
static bool parseOtaHeaders(const String &line, String &newlstmod) {
  if (line.startsWith("HTTP/1.1")) {
    if (line.indexOf(" 200 ") < 0) {
      dspOtaDisplayFailed("Invalid HTTP status");
      dbgprint("Got a non 200 status code from server!");
      delay(3000);
      return false;
    }
  }

  scan_content_length(line.c_str());

  if (line.startsWith("Last-Modified: ")) {
    newlstmod = line.substring(15);
  }

  return true;
}

/**
 * @brief Connect to update server and request the binary file.
 *
 * @param updatehost Hostname of the update server
 * @param binfile Path to the binary file
 * @return true if connection successful, false otherwise
 */
static bool connectToUpdateServer(const char *updatehost, const char *binfile) {
  dspOtaDisplayPhase("Connecting...");

  dbgprint("Connecting to %s for %s", updatehost, binfile);

  if (!otaclient.connect(updatehost, 80)) {
    dspOtaDisplayFailed("Connect failed");
    dbgprint("Connect to updatehost failed!");
    delay(3000);
    return false;
  }

  dspOtaDisplayPhase("Requesting file...");

  otaclient.printf("GET %s HTTP/1.1\r\n"
                   "Host: %s\r\n"
                   "Cache-Control: no-cache\r\n"
                   "Connection: close\r\n\r\n",
                   binfile, updatehost);

  return true;
}

/**
 * @brief Wait for response from update server.
 *
 * @param timeout_ms Timeout in milliseconds
 * @return true if data available, false on timeout
 */
static bool waitForResponse(uint32_t timeout_ms) {
  uint32_t timeout = millis();
  while (otaclient.available() == 0) {
    if (millis() - timeout > timeout_ms) {
      dspOtaDisplayFailed("Connect timeout");
      dbgprint("Connect to Update host Timeout!");
      delay(3000);
      otaclient.stop();
      return false;
    }
  }
  return true;
}

/**
 * @brief Perform the actual update (software or NEXTION image).
 *
 * @param binfile Path to the binary file
 * @param newlstmod New Last-Modified timestamp
 * @param lstmodkey NVS key for storing timestamp
 * @return true if update was successful, false otherwise
 */
static bool performUpdate(const char *binfile, const String &newlstmod, const char *lstmodkey) {
  bool success = false;

  if (strstr(binfile, ".bin")) {
    success = do_software_update(clength);
  }

  if (strstr(binfile, ".tft")) {
    success = do_nextion_update(clength);
  }

  if (success) {
    nvsAddValue(lstmodkey, newlstmod);
  }

  return success;
}

/**
 * @brief Update software by download from remote host.
 *
 * @param lstmodkey NVS key for storing Last-Modified timestamp
 * @param updatehost Hostname of the update server
 * @param binfile Path to the binary file
 * @return true if update was started and completed successfully
 */
bool update_software(const char *lstmodkey, const char *updatehost, const char *binfile) {
  String line;
  String lstmod;
  String newlstmod;

  updatereq = false;
  otastart();
  stop_mp3client();

  otaShowStartScreen(0);
  dspOtaDisplayPhase("Preparing...");
  lstmod = nvsGetValue(lstmodkey);

  if (!connectToUpdateServer(updatehost, binfile)) {
    dspOtaDisplayRestoreDashboard();
    return false;
  }

  if (!waitForResponse(20000)) {
    dspOtaDisplayRestoreDashboard();
    return false;
  }

  dspOtaDisplayPhase("Reading headers...");

  while (otaclient.available()) {
    line = otaclient.readStringUntil('\n');
    line.trim();
    dbgprint(line.c_str());

    if (!line.length()) {
      break;
    }

    if (!parseOtaHeaders(line, newlstmod)) {
      dspOtaDisplayRestoreDashboard();
      return false;
    }
  }

  if (newlstmod == lstmod) {
    dspOtaDisplayInfo("No update required", "Firmware is current");
    dbgprint("No new version available");
    delay(3000);
    otaclient.flush();
    dspOtaDisplayRestoreDashboard();
    return false;
  }

  if (clength > 0) {
    return performUpdate(binfile, newlstmod, lstmodkey);
  }

  dspOtaDisplayFailed("Missing file data");
  dbgprint("There was no content in the response");
  delay(3000);
  otaclient.flush();
  dspOtaDisplayRestoreDashboard();
  return false;
}

/**
 * @brief Download and install firmware from an HTTPS or HTTP URL.
 *
 * @param url Full URL to the firmware binary
 * @return true if the update completed successfully
 */
bool updateFirmwareFromUrl(const String &url) {
  if (url.length() == 0 || WiFi.status() != WL_CONNECTED) {
    dspOtaDisplayFailed("No WiFi");
    delay(3000);
    dspOtaDisplayRestoreDashboard();
    return false;
  }

  if (!isOtaUpdateCapable()) {
    dbgprint("OTA failed: no OTA slot in partition table");
    dspOtaDisplayFailed("No OTA slot");
    delay(3000);
    dspOtaDisplayRestoreDashboard();
    return false;
  }

  updatereq = false;
  otastart();
  stop_mp3client();

  otaShowStartScreen(0);
  dspOtaDisplayPhase("Connecting...");

  HTTPClient http;
  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  http.setTimeout(60000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  bool httpReady = false;
  if (url.startsWith("https://")) {
    secureClient.setInsecure();
    httpReady = http.begin(secureClient, url);
  } else {
    httpReady = http.begin(plainClient, url);
  }

  if (!httpReady) {
    dbgprint("OTA URL invalid");
    dspOtaDisplayFailed("Invalid URL");
    delay(3000);
    dspOtaDisplayRestoreDashboard();
    return false;
  }

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    dbgprint("OTA download failed: HTTP %d", code);
    char err[20];
    snprintf(err, sizeof(err), "HTTP error %d", code);
    dspOtaDisplayFailed(err);
    http.end();
    delay(3000);
    dspOtaDisplayRestoreDashboard();
    return false;
  }

  int contentLength = http.getSize();
  const esp_partition_t *otaPart = esp_ota_get_next_update_partition(NULL);
  if (otaPart && contentLength > 0 && (size_t)contentLength > otaPart->size) {
    dbgprint("OTA image too large (%d > %u)", contentLength, (unsigned)otaPart->size);
    dspOtaDisplayFailed("Image too large");
    http.end();
    delay(3000);
    dspOtaDisplayRestoreDashboard();
    return false;
  }

  size_t totalBytes = (contentLength > 0) ? (size_t)contentLength : 0U;
  dspOtaDisplaySetTotalBytes(totalBytes);
  dspOtaDisplayPhase("Downloading...");

  if (!Update.begin(totalBytes > 0 ? totalBytes : UPDATE_SIZE_UNKNOWN)) {
    dbgprint("OTA begin failed: %s", Update.errorString());
    dspOtaDisplayFailed(Update.errorString());
    http.end();
    delay(3000);
    dspOtaDisplayRestoreDashboard();
    return false;
  }

  size_t written = otaWriteStreamWithProgress(*http.getStreamPtr(), totalBytes, "Installing...");
  http.end();

  if (written == 0 || !Update.end(true)) {
    dbgprint("OTA write failed: %s", Update.errorString());
    Update.abort();
    dspOtaDisplayFailed(Update.errorString());
    delay(3000);
    dspOtaDisplayRestoreDashboard();
    return false;
  }

  if (!Update.isFinished()) {
    dbgprint("OTA finished flag not set");
    dspOtaDisplayFailed("Verify failed");
    delay(3000);
    dspOtaDisplayRestoreDashboard();
    return false;
  }

  dbgprint("OTA success, %u bytes", (unsigned)written);
  dspOtaDisplayProgress(written, totalBytes > 0 ? totalBytes : written, "Complete");
  return true;
}
