// OTA-Update: https://github.com/esp8266/Arduino/issues/4400

#include <HTTPClient.h>
#include <Update.h>

/**
 * @brief Scan for content-length information in header line.
 *        If the line contains content-length information, set the clength
 * counter.
 *
 * @param metalinebf Header line to scan
 */
void scan_content_length(const char *metalinebf) {
  if (strstr(metalinebf, "Content-Length")) {
    char *p;
    clength = atoi(metalinebf + 15);
    p = dbgprint("Content-Length is %d", clength);
    dsp_println(p);
    delay(2000);
  }
}

/**
 * @brief Update via WiFi has been started by Arduino IDE or update request.
 */
void otastart() {
  char *p;
  p = dbgprint("OTA update Started");
  dsp_println(p);
  delay(2000);
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

  if (!nxtserial) {
    return false;
  }

  vTaskDelete(xspftask);
  delay(1000);

  nxtserial->printf("\xFF\xFF\xFF"); // Empty command

  // Read garbage input
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

  while (clength) {
    k = (clength > 4096) ? 4096 : clength;
    k = otaclient.read(tmpbuff, k);
    dbgprint("TFT file, read %d bytes", k);
    nxtserial->write(tmpbuff, k);

    c = waitForNextionAck(2000);
    if (c != 0x05) {
      break;
    }
    clength -= k;
  }

  otaclient.flush();

  if (clength == 0) {
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

  if (!Update.begin(clength)) {
    dbgprint("Not enough space to begin OTA");
    otaclient.flush();
    return false;
  }

  dbgprint("Begin OTA update, length is %d", clength);

  if (Update.writeStream(otaclient) == clength) {
    dbgprint("Written %d bytes successfully", clength);
  } else {
    dbgprint("Write failed!");
  }

  if (Update.end()) {
    dbgprint("OTA done");
    if (Update.isFinished()) {
      dbgprint("Update successfully completed");
      res = true;
    } else {
      dbgprint("Update not finished!");
    }
  } else {
    dbgprint("Error Occurred. Error %s", Update.getError());
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
      dsp_erase();
      dsp_println("Invalid HTTP Status");
      dsp_println("Exit OTA Update");
      dbgprint("Got a non 200 status code from server!");
      delay(2000);
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
  dsp_erase();
  dsp_println("Connecting to host");
  dsp_println(updatehost);
  delay(2000);

  dbgprint("Connecting to %s for %s", updatehost, binfile);

  if (!otaclient.connect(updatehost, 80)) {
    dsp_erase();
    dsp_println("Connecting failed");
    dbgprint("Connect to updatehost failed!");
    delay(2000);
    return false;
  }

  dsp_erase();
  dsp_println("Loading bin file");
  delay(2000);

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
      dsp_erase();
      dsp_println("Connect Timeout");
      dbgprint("Connect to Update host Timeout!");
      delay(2000);
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
  dsp_erase();
  dsp_println("Updating sketch");

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
 */
void update_software(const char *lstmodkey, const char *updatehost, const char *binfile) {
  String line;
  String lstmod;
  String newlstmod;

  dsp_erase();
  dsp_println("OTA Update");
  delay(2000);

  updatereq = false;
  otastart();
  stop_mp3client();
  lstmod = nvsGetValue(lstmodkey);

  if (!connectToUpdateServer(updatehost, binfile)) {
    return;
  }

  if (!waitForResponse(20000)) {
    return;
  }

  // Parse HTTP response headers
  dsp_erase();
  while (otaclient.available()) {
    line = otaclient.readStringUntil('\n');
    line.trim();

    dsp_println(line.c_str());
    dbgprint(line.c_str());

    if (!line.length()) {
      break;
    }

    if (!parseOtaHeaders(line, newlstmod)) {
      return;
    }
  }

  // Check if update is needed
  if (newlstmod == lstmod) {
    dsp_erase();
    dsp_println("No update required");
    dsp_println("Exit OTA Update");
    dbgprint("No new version available");
    delay(2000);
    otaclient.flush();
    return;
  }

  // Perform the update
  if (clength > 0) {
    performUpdate(binfile, newlstmod, lstmodkey);
  } else {
    dsp_erase();
    dsp_println("Missing content in response");
    dbgprint("There was no content in the response");
    delay(2000);
    otaclient.flush();
  }
}

/**
 * @brief Download and install firmware from an HTTPS or HTTP URL.
 *
 * @param url Full URL to the firmware binary
 * @return true if the update completed successfully
 */
bool updateFirmwareFromUrl(const String &url) {
  if (url.length() == 0 || WiFi.status() != WL_CONNECTED) {
    return false;
  }

  updatereq = false;
  otastart();
  stop_mp3client();

  HTTPClient http;
  http.setTimeout(60000);
  if (!http.begin(url)) {
    dbgprint("OTA URL invalid");
    return false;
  }

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    dbgprint("OTA download failed: HTTP %d", code);
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  if (!Update.begin(contentLength > 0 ? (size_t)contentLength : UPDATE_SIZE_UNKNOWN)) {
    dbgprint("OTA begin failed: %s", Update.errorString());
    http.end();
    return false;
  }

  size_t written = Update.writeStream(*http.getStreamPtr());
  http.end();

  if (written == 0 || !Update.end(true)) {
    dbgprint("OTA write failed: %s", Update.errorString());
    Update.abort();
    return false;
  }

  dbgprint("OTA success, %u bytes", (unsigned)written);
  resetreq = true;
  return true;
}