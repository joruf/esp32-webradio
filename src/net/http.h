
/**
 * @brief Restore factory default configuration in NVS.
 */
void restoreDefaultConfig();

/**
 * @brief Write factory default configuration to NVS without clearing it first.
 */
void saveDefaultConfig();

#include "http_queue.h"
#include "captive_portal.h"
#include "audio/media_probe.h"
#include "net/web_ui_activity.h"

static String http_host_header = "";
static String http_if_none_match = "";
static String http_if_modified_since = "";
static String http_post_body_cache = "";
static uint8_t httpReadBuf[1024]; // HTTP input buffer shared with body reader
static uint16_t httpReadPos = 0;    // Read position in httpReadBuf
static uint16_t httpReadLen = 0;  // Number of valid bytes in httpReadBuf

static void httpReadResetBuffer() {
  httpReadPos = 0;
  httpReadLen = 0;
}

static void resetHttpRequestState() {
  http_getcmd = "";
  http_rqfile = "";
  http_post_content_length = 0;
  http_post_body_cache = "";
  http_response_flag = false;
  http_host_header = "";
  http_if_none_match = "";
  http_if_modified_since = "";
  httpReadResetBuffer();
}

/**
 * @brief Recover HTTP server state after a dropped client connection.
 */
void recoverHttpServerState() {
  if (http_response_flag && !cmdclient.connected()) {
    resetHttpRequestState();
  }
}

/**
 * @brief Close the active HTTP client connection and clear request state.
 */
static void closeHttpClient() {
  if (cmdclient.connected()) {
    cmdclient.flush();
    cmdclient.stop();
  }
  resetHttpRequestState();
}

/**
 * @brief Send an HTTP/1.1 response with a JSON body and close the connection.
 *
 * @param body JSON response body
 */
static void sendHttpJsonResponse(const String &body) {
  cmdclient.print("HTTP/1.1 200 OK\r\n");
  cmdclient.print("Content-Type: application/json; charset=utf-8\r\n");
  cmdclient.print("Content-Length: ");
  cmdclient.print(body.length());
  cmdclient.print("\r\n");
  cmdclient.print("Connection: close\r\n");
  cmdclient.print("Cache-Control: no-cache\r\n\r\n");
  cmdclient.print(body);
  cmdclient.flush();
  closeHttpClient();
}

/**
 * @brief Fill the list of all relevant keys in NVS.
 *
 * Reads all pages of the NVS partition, extracts active entries with the
 * correct namespace, stores them in nvskeys array, and sorts the keys.
 */
void fillkeylist() {
  esp_err_t result = ESP_OK; // Result of reading partition
  uint32_t offset = 0;       // Offset in nvs partition
  uint16_t i;                // Index in Entry 0..125.
  uint8_t bm;                // Bitmap for an entry
  uint16_t nvsKeyIndex = 0;
  bool keyListFull = false;

  keynames.clear(); // Clear the list
  while (offset < nvs->size && !keyListFull) {
    result = esp_partition_read(nvs, offset, &nvsbuf,
                                sizeof(nvsbuf)); // Read 1 page in nvs partition
    if (result != ESP_OK) {
      dbgprint("Error reading NVS!");
      break;
    }
    i = 0;
    while (i < 126 && !keyListFull) {
      bm = (nvsbuf.Bitmap[i / 4] >> ((i % 4) * 2)); // Get bitmap for this entry,
      bm &= 0x03;                                   // 2 bits for one entry
      if (bm == 2)                                  // Entry is active?
      {
        if (nvsbuf.Entry[i].Ns == namespace_ID) // Namespace right?
        {
          if (nvsKeyIndex >= MAXKEYS - 1) {
            dbgprint("NVS key list full (%d keys)", MAXKEYS);
            keyListFull = true;
            break;
          }
          strcpy(nvskeys[nvsKeyIndex], nvsbuf.Entry[i].Key); // save in table
          nvsKeyIndex++;
        }
        i += nvsbuf.Entry[i].Span; // Next entry
      } else {
        i++;
      }
    }
    offset += sizeof(nvs_page); // Prepare to read next page in nvs
  }
  nvskeys[nvsKeyIndex][0] = '\0'; // Empty key at the end
  dbgprint("Read %d keys from NVS", nvsKeyIndex);
  bubbleSortKeys(nvsKeyIndex); // Sort the keys
}

/**
 * @brief Make a list of acceptable networks from preferences.
 *
 * Will be called only once by setup(). The result will be stored in wifilist.
 * The last found SSID and password are kept in common data. If only one SSID is
 * defined, the connection is made without using wifiMulti. In this case a
 * connection will be made even if the SSID is hidden.
 */
void mk_lsan() {
  uint8_t i;         // Loop control
  char key[10];      // For example: "wifi_03"
  String buf;        // "SSID/password"
  String lssid, lpw; // Last read SSID and password from nvs
  int inx;           // Place of "/"
  WifiInfo_t winfo;  // Element to store in list

  dbgprint("Create list with acceptable WiFi networks");
  for (i = 0; i < 100; i++) // Examine wifi_00 .. wifi_99
  {
    sprintf(key, "wifi_%02d", i); // Form key in preferences
    if (nvsKeyExists(key))        // Does it exists?
    {
      buf = nvsGetValue(key); // Get the contents
      inx = buf.indexOf("/"); // Find separator between ssid and password
      if (inx > 0)            // Separator found?
      {
        lpw = buf.substring(inx + 1);  // Isolate password
        lssid = buf.substring(0, inx); // Holds SSID now
        // Masked placeholders must never be used for association. Empty
        // passwords are valid (open / OWE networks) and stay in the list.
        if (lpw == "********") {
          dbgprint("Skip WiFi SSID %s with invalid/masked password (wifi_%02d)", lssid.c_str(), i);
          continue;
        }
        // Skip duplicate SSIDs so WiFiMulti does not try the same network twice
        // with possibly stale/wrong passwords from a bad config merge.
        bool duplicate = false;
        for (size_t di = 0; di < wifilist.size(); di++) {
          if (wifilist[di].ssid && lssid.equals(wifilist[di].ssid)) {
            duplicate = true;
            break;
          }
        }
        if (duplicate) {
          dbgprint("Skip duplicate WiFi SSID %s (wifi_%02d)", lssid.c_str(), i);
          continue;
        }
        dbgprint("Added %s to list of networks", lssid.c_str());
        winfo.inx = i;                      // Create new element for wifilist ;
        winfo.ssid = strdup(lssid.c_str()); // Set ssid of element
        winfo.passphrase = strdup(lpw.c_str());
        wifilist.push_back(winfo);  // Add to list
        wifiMulti.addAP(winfo.ssid, // Add to wifi acceptable network list
                        winfo.passphrase);
      }
    }
  }
  dbgprint("End adding networks");
}

/**
 * @brief Return preset, tone and volume status as a formatted string.
 *
 * Included are the preset number, the volume and the four tone settings (HA,
 * HF, LA, LF).
 *
 * @return String Formatted status string
 */
String getradiostatus() {
  char pnr[3]; // Preset as 2 character, i.e. "03"

  sprintf(pnr, "%02d", ini_block.newpreset);                      // Current preset
  return String("preset=") +                                      // Add preset setting
         String(pnr) + String("\nvolume=") +                      // Add volume setting
         String(String(ini_block.reqvol)) + String("\ntoneha=") + // Add tone setting HA
         String(ini_block.rtone[0]) + String("\ntonehf=") +       // Add tone setting HF
         String(ini_block.rtone[1]) + String("\ntonela=") +       // Add tone setting LA
         String(ini_block.rtone[2]) + String("\ntonelf=") +       // Add tone setting LF
         String(ini_block.rtone[3]);
}

/**
 * @brief Read next byte from HTTP input buffer.
 *
 * Uses a static buffer for speed reasons. The buffer is refilled when empty or
 * when forcestart is true.
 *
 * @param forcestart If true, force a buffer refill regardless of current
 * position
 * @param maxIdleRetries Max idle polls before returning an error
 * @return uint8_t The next byte read, or '\n' on error
 */
static bool httpReadTakeBuffered(uint8_t *out) {
  if (httpReadPos < httpReadLen) {
    *out = httpReadBuf[httpReadPos++];
    return true;
  }
  return false;
}

uint8_t httpReadByte(bool forcestart, uint16_t maxIdleRetries = 40) {
  uint8_t *buf = httpReadBuf;
  uint16_t &i = httpReadPos;
  uint16_t &len = httpReadLen;
  uint16_t tlen;            // Number of available bytes
  uint16_t trycount = 0;    // Limit max. time to read

  if (!cmdclient.connected()) {
    i = 0;
    len = 0;
    return '\n';
  }

  if (forcestart) {
    i = 0;
    len = 0;
  }

  if (forcestart || (i == len)) // Time to read new buffer
  {
    trycount = 0;
    while (cmdclient.connected()) // Loop while the client's connected
    {
      tlen = cmdclient.available(); // Number of bytes to read from the client
      len = tlen;                   // Try to read whole input
      if (len == 0)                 // Any input available?
      {
        if (++trycount > maxIdleRetries) // Not for a long time?
        {
          dbgprint("HTTP input shorter than expected");
          i = 0;
          len = 0;
          return '\n'; // Error! No input
        }
        delay(1); // Give communication some time
        continue; // Next loop of no input yet
      }
      if (len > sizeof(httpReadBuf)) // Limit number of bytes (buf is a pointer, so
      {                              // sizeof(buf) would wrongly cap reads to 4 bytes)
        len = sizeof(httpReadBuf);
      }
      len = cmdclient.read(buf, len); // Read a number of bytes from the stream
      i = 0;                          // Pointer to begin of buffer
      break;
    }
    if (!cmdclient.connected() || len == 0) {
      i = 0;
      len = 0;
      return '\n';
    }
  }
  if (i >= len) {
    i = 0;
    len = 0;
    return '\n';
  }
  return buf[i++];
}

/**
 * @brief Read the HTTP POST request body.
 *
 * Uses Content-Length when available, otherwise reads until an idle timeout.
 *
 * @param maxLen Maximum accepted body size
 * @return String Request body
 */
static int httpReadPostByte(uint32_t deadline) {
  uint8_t b;

  if (httpReadTakeBuffered(&b)) {
    return (int)b;
  }
  while (cmdclient.connected() && millis() < deadline) {
    if (cmdclient.available() > 0) {
      return cmdclient.read();
    }
    delay(1);
  }
  return -1;
}

static String readHttpPostBody(size_t maxLen) {
  if (http_post_body_cache.length() > 0) {
    String body = http_post_body_cache;
    http_post_body_cache = "";
    return body;
  }

  String body = "";
  uint32_t targetLen = http_post_content_length;
  uint32_t deadline = millis() + 5000;

  timerStop(timer);
  http_post_content_length = 0;

  if (targetLen > 0 && targetLen <= maxLen) {
    body.reserve(targetLen + 1);
    while (body.length() < targetLen) {
      int b = httpReadPostByte(deadline);
      if (b < 0) {
        break;
      }
      body += (char)b;
    }
  } else if (targetLen == 0) {
    while (body.length() < maxLen - 1) {
      int b = httpReadPostByte(deadline);
      if (b < 0) {
        break;
      }
      if (b != '\r') {
        body += (char)b;
      }
    }
    body.trim();
  } else {
    while (body.length() < maxLen - 1) {
      int b = httpReadPostByte(deadline);
      if (b < 0) {
        break;
      }
      if (b != '\r') {
        body += (char)b;
      }
    }
    body.trim();
  }

  timerStart(timer);
  return body;
}

/**
 * @brief Read a binary chunk from buffered/post stream data.
 *
 * @param buf Target buffer
 * @param maxLen Maximum bytes to read
 * @param deadline Absolute deadline in millis()
 * @return size_t Number of bytes read
 */
static size_t httpReadPostChunk(uint8_t *buf, size_t maxLen, uint32_t deadline) {
  size_t n = 0;
  uint8_t b;

  while (n < maxLen && httpReadTakeBuffered(&b)) {
    buf[n++] = b;
  }

  while (n < maxLen && cmdclient.connected() && millis() < deadline) {
    int avail = cmdclient.available();
    if (avail <= 0) {
      delay(1);
      continue;
    }
    size_t toRead = (size_t)avail;
    if (toRead > (maxLen - n)) {
      toRead = maxLen - n;
    }
    int got = cmdclient.read(buf + n, toRead);
    if (got > 0) {
      n += (size_t)got;
      break;
    }
    delay(1);
  }

  return n;
}

/**
 * @brief Install firmware uploaded through HTTP POST body.
 *
 * Expects raw binary data (Content-Type: application/octet-stream) with
 * Content-Length set by the client.
 *
 * @param contentLength Firmware length from Content-Length
 * @param message Output message for JSON response
 * @return true on successful write/verify, false otherwise
 */
static bool handleFirmwareUploadPost(uint32_t contentLength, String &message) {
  uint8_t buf[1024];
  size_t written = 0;
  uint32_t noDataDeadline;

  if (contentLength == 0) {
    message = "Missing Content-Length";
    return false;
  }

  // A real firmware image for this device is ~1.6 MB. Anything tiny is certainly
  // not a valid application image (e.g. a bootloader, partition table, or a
  // completely unrelated file) and must be rejected before touching flash.
  if (contentLength < 262144UL) {
    message = "File too small to be a valid firmware image";
    eventLogRecordWarning("Firmware upload rejected: file too small");
    return false;
  }

  if (!isOtaUpdateCapable((size_t)contentLength)) {
    message = "Image does not fit OTA partition";
    eventLogRecordWarning("Firmware upload rejected: does not fit partition");
    return false;
  }

  updatereq = false;
  otastart();
  stop_mp3client();
  otaShowStartScreen((size_t)contentLength);
  dspOtaDisplayPhase("Uploading...");
  dspOtaDisplaySetTotalBytes((size_t)contentLength);

  if (!Update.begin((size_t)contentLength)) {
    message = String("Update.begin failed: ") + String(Update.errorString());
    eventLogRecordError("Firmware upload: Update.begin failed");
    dspOtaDisplayFailed(Update.errorString());
    return false;
  }

  timerStop(timer);
  noDataDeadline = millis() + 60000UL;

  bool headerChecked = false; // Validate the ESP32 image header on the first chunk

  while (written < (size_t)contentLength && cmdclient.connected()) {
    size_t remaining = (size_t)contentLength - written;
    size_t chunk = remaining > sizeof(buf) ? sizeof(buf) : remaining;
    size_t got = httpReadPostChunk(buf, chunk, noDataDeadline);
    if (got == 0) {
      if (millis() >= noDataDeadline) {
        break;
      }
      continue;
    }

    // Verify the firmware header before writing anything to flash. The ESP32
    // application image starts with magic byte 0xE9 and carries a chip_id at
    // offset 12-13 (0x0000 = ESP32). This catches non-ESP32 files and images
    // built for a different chip family before they can brick the device.
    if (!headerChecked) {
      if (buf[0] != 0xE9) {
        timerStart(timer);
        Update.abort();
        message = "Not a valid ESP32 firmware image (bad magic byte)";
        eventLogRecordWarning("Firmware upload rejected: bad magic byte");
        dspOtaDisplayFailed("Invalid image");
        return false;
      }
      if (got >= 14) {
        uint16_t chipId = (uint16_t)buf[12] | ((uint16_t)buf[13] << 8);
        if (chipId != 0x0000) {
          timerStart(timer);
          Update.abort();
          message = "Firmware built for a different chip (not ESP32)";
          eventLogRecordWarning("Firmware upload rejected: wrong chip type");
          dspOtaDisplayFailed("Wrong chip");
          return false;
        }
      }
      headerChecked = true;
    }

    noDataDeadline = millis() + 60000UL;
    size_t nw = Update.write(buf, got);
    if (nw != got) {
      timerStart(timer);
      Update.abort();
      message = String("Write failed: ") + String(Update.errorString());
      eventLogRecordError("Firmware upload write to flash failed");
      dspOtaDisplayFailed(Update.errorString());
      return false;
    }

    written += nw;
    dspOtaDisplayProgress(written, (size_t)contentLength, "Uploading...");
  }

  timerStart(timer);
  http_post_content_length = 0;
  http_post_body_cache = "";

  if (written != (size_t)contentLength) {
    Update.abort();
    message = String("Upload incomplete: ") + String((unsigned)written) + "/" + String((unsigned)contentLength);
    eventLogRecordError("Firmware upload incomplete (connection dropped)");
    dspOtaDisplayFailed("Upload incomplete");
    return false;
  }

  if (!Update.end(true) || !Update.isFinished()) {
    message = String("Finalize failed: ") + String(Update.errorString());
    eventLogRecordError("Firmware upload finalize/verify failed");
    dspOtaDisplayFailed(Update.errorString());
    return false;
  }

  dspOtaDisplayProgress(written, (size_t)contentLength, "Complete");
  message = "Firmware uploaded successfully. Restarting...";
  eventLogSetShutdownReason("firmware upload (.bin)");
  resetreq = true;
  return true;
}

/**
 * @brief Update the preferences.
 *
 * Called from the web interface. Reads the values from the HTTP input,
 * clears all existing preferences, and writes the new values to NVS.
 */
void writeprefs() {
  int inx;              // Position in inputstr
  uint8_t winx;         // Index in wifilist
  char c;               // Input character
  String inputstr = ""; // Input regel
  String key, contents; // Pair for Preferences entry
  String dstr;          // Contents for debug

  /*
  timerAlarmDisable ( timer ) ;                               // Disable the
  timer
  */
  timerStop(timer); // Disable the timer

  nvsClear(); // Remove all preferences
  while (true) {
    c = httpReadByte(false); // Get next inputcharacter
    if (c == '\n')     // Newline?
    {
      if (inputstr.length() == 0)

      {
        dbgprint("End of writing preferences");
        break; // End of contents
      }
      if (!inputstr.startsWith("#")) // Skip pure comment lines
      {
        inx = inputstr.indexOf("=");
        if (inx >= 0) // Line with "="?
        {
          key = inputstr.substring(0, inx); // isolate the key
          key.trim();
          contents = inputstr.substring(inx + 1); // and contents
          contents.trim();
          dstr = contents;                 // Copy for debug
          if ((key.indexOf("wifi_") == 0)) // Sensitive info?
          {
            winx = key.substring(5).toInt(); // Get index in wifilist
            if ((winx < wifilist.size()) &&  // Existing wifi spec in wifilist?
                (contents.indexOf(wifilist[winx].ssid) == 0) && (contents.indexOf("/****") > 0)) // Hidden password?
            {
              contents = String(wifilist[winx].ssid) + // Retrieve ssid and password
                         String("/") + String(wifilist[winx].passphrase);
              dstr = String(wifilist[winx].ssid) + String("/*******"); // Hide in debug line
            }
          }

          dbgprint("writeprefs setstr %s = %s", key.c_str(), dstr.c_str());
          nvsAddValue(key.c_str(), contents); // Save new pair (needs a restart
                                              // to use it in the program)

          // Save this values directly in the ini_block to make it available
          // without restart!
          if (key == "alrm_config") {
            ini_block.alrm_config = contents;
            if (applyAlarmConfig(contents)) {
              syncAlarmsToRadioConfig(); // Keep the radio_config blob in sync
            }
            dbgprint("Alarm configuration saved from preferences");
          }
        }
      }
      inputstr = "";
    } else {
      if (c != '\r') // Not newline.  Is is a CR?
      {
        inputstr += String(c); // No, normal char, add to string
      }
    }
  }

  /*
  timerAlarmEnable ( timer ) ;                                // Enable the
  timer
  */
  timerStart(timer); // Enable the timer

  fillkeylist(); // Update list with keys
}

/**
 * @brief Read alarm configuration JSON from the HTTP POST body and save it.
 */
bool writeAlarmConfig() {
  String body = readHttpPostBody(ALRM_CONFIG_MAX_LEN);

  if (body.length() > 0) {
    if (applyAlarmConfig(body)) {
      syncAlarmsToRadioConfig();
      dbgprint("Alarm configuration saved (%d bytes)", body.length());
      fillkeylist();
      return true;
    }
    dbgprint("Alarm configuration save rejected");
    return false;
  }

  dbgprint("Alarm configuration save ignored (empty body)");
  return false;
}

/**
 * @brief Read unified radio configuration JSON from the HTTP POST body and save it.
 */
String gLastConfigSaveDetail = "";

bool writeRadioConfig() {
  String body = readHttpPostBody(RADIO_CONFIG_MAX_LEN);
  gLastConfigSaveDetail = "";

  if (body.length() > 0) {
    const bool hasWifiKey = body.indexOf("\"wifi\"") >= 0;
    gLastConfigSaveDetail = String("blen=") + String(body.length()) + String(" wkey=") +
                            String(hasWifiKey ? 1 : 0);
    dbgprint("writeRadioConfig: body %d bytes, wifi_key=%d", body.length(), hasWifiKey ? 1 : 0);
    if (applyRadioConfigMerge(body, true)) {
      readprogbuttons();
      applyDisplayOrientation(true);
      dbgprint("Radio configuration saved (%d bytes)", body.length());
      fillkeylist();
      return true;
    }
    dbgprint("Radio configuration save rejected (%d bytes)", body.length());
    return false;
  }

  dbgprint("Radio configuration save ignored (empty body)");
  gLastConfigSaveDetail = "empty_body";
  return false;
}

/**
 * @brief Read feature settings JSON from the HTTP POST body and save them.
 */
bool writeFeaturesConfig() {
  String body = readHttpPostBody(RADIO_CONFIG_MAX_LEN);

  if (body.length() > 0) {
    if (applyFeaturesConfig(body)) {
      applyDisplayOrientation(true);
      dbgprint("Feature configuration saved (%d bytes)", body.length());
      fillkeylist();
      return true;
    }
    dbgprint("Feature configuration save rejected (%d bytes)", body.length());
    return false;
  }

  dbgprint("Feature configuration save ignored (empty body)");
  return false;
}

/**
 * @brief Send settings to the webserver.
 *
 * Includes all presets, the current station, the volume and the tone settings.
 */
void getsettings() {
  String val;     // Result to send
  String statstr; // Station string
  int inx;        // Position of search char in line
  int16_t i;      // Loop control, preset number

  for (i = 0; i < MAXPRESETS; i++) // Max number of presets
  {
    statstr = nvsGetHostFromPref(i); // Get the preset from NVS
    if (statstr != "")               // Preset available?
    {
      // Show just comment if available.  Otherwise the preset itself.
      inx = statstr.indexOf("#"); // Get position of "#"
      if (inx > 0)                // Hash sign present?
      {
        statstr.remove(0, inx + 1); // remove non-comment part
      }
      chomp(statstr); // Remove garbage from description
      dbgprint("statstr is %s", statstr.c_str());
      val += String("preset_") + String(i) + String("=") + statstr + String("\n"); // Add delimeter
      if (val.length() > 1000)                                                     // Time to flush?
      {
        cmdclient.print(val); // send
        val = "";             // Start new string
      }
    }
  }
  val += getradiostatus() + // Add radio setting
         String("\n\n");    // End of reply
  cmdclient.print(val);     // And send
}

/**
 * @brief Handle the output after an HTTP request.
 *
 * Processes the response based on the requested command or file.
 */
void handlehttpreply() {
  const char *p;      // Pointer to reply if command
  String sndstr = ""; // String to send
  int n;              // Number of files on SD card

  if (http_response_flag) {
    http_response_flag = false;
    if (!cmdclient.connected()) {
      resetHttpRequestState();
      return;
    }
    if (http_rqfile.length() == 0 && // An empty "GET"?
        http_getcmd.length() == 0) {
        if (captivePortalShouldRedirect(http_host_header, "/")) {
          handleWebpage(String("config.html"));
        } else if (NetworkFound) // check network
        {
          handleWebpage(String("index.html")); // Okay, send the startpage
        } else {
          handleWebpage(String("config.html")); // Or the configuration page if in AP mode
        }
      } else {
        if (captivePortalShouldRedirect(http_host_header, http_rqfile)) {
          handleWebpage(String("config.html"));
        } else if (http_rqfile.length()) // Static file has priority over query parameters
        {
          dbgprint("Start file reply for %s", http_rqfile.c_str());
          handleWebpage(http_rqfile);
        } else if (http_getcmd.length()) // Command to analyze?
        {
          dbgprint("Send reply for %s", http_getcmd.c_str());
          sndstr = httpheader(String("text/html")); // Set header

          if (http_getcmd.startsWith("getprefs")) // Is it a "Get preferences"?
          {
            if (datamode != STOPPED) // Still playing?
            {
              //              datamode = STOPREQD ;                         //
              //              Stop playing
            }
            sndstr += readprefs(true);                  // Read and send
          } else if (http_getcmd.startsWith("getdefs")) // Is it a "Get default preferences"?
          {
            sndstr += String(defprefs_txt + 1);           // read initial values
          } else if (http_getcmd.startsWith("getconfig")) // Get unified JSON config
          {
            String json = getRadioConfigJsonForWeb();
            if (json.length() == 0 || !isValidRadioConfigJson(json)) {
              dbgprint("getconfig: rebuilt JSON invalid, using defaults");
              json = getDefaultRadioConfigJson();
            }
            json = appendWifiSignalsToConfigJson(json, false);
            sendHttpJsonResponse(json);
            return;
          } else if (http_getcmd.startsWith("getwifisignals")) {
            sendHttpJsonResponse(buildWifiSignalsJson(http_getcmd.indexOf("scan=1") >= 0));
            return;
          } else if (http_getcmd.startsWith("saveconfig")) // Save unified JSON config
          {
            if (writeRadioConfig()) {
              uint8_t wifiCount = 0;
              char wkey[10];
              for (uint8_t wi = 0; wi < 100; wi++) {
                sprintf(wkey, "wifi_%02d", wi);
                if (!nvsKeyExists(wkey)) {
                  break;
                }
                wifiCount++;
              }
              sndstr += String("Configuration saved");
              if (gLastConfigSaveDetail.indexOf("pWifi=1") >= 0) {
                sndstr += String(" (") + String(wifiCount) + String(" WiFi network") +
                          String(wifiCount == 1 ? "" : "s") + String(")");
              }
            } else {
              eventLogRecordError("Web UI: configuration save failed");
              sndstr += String("Configuration save failed");
            }
          } else if (http_getcmd.startsWith("savefeatures")) // Save display/feature settings only
          {
            if (writeFeaturesConfig()) {
              sndstr += String("Display settings saved");
            } else {
              sndstr += String("Display settings save failed");
            }
          } else if (http_getcmd.startsWith("restoreconfig")) // Restore factory defaults
          {
            restoreDefaultConfig();
            migrateToRadioConfig();
            readprogbuttons();
            sndstr += String("Default configuration restored");
          } else if (http_getcmd.startsWith("eraseeeprom")) // Erase all NVS data
          {
            timerStop(timer);
            if (nvsEraseAllCommitted() == ESP_OK) {
              saveDefaultConfig();
              fillkeylist();
              dbgprint("NVS storage erased via web interface");
              eventLogSetShutdownReason("NVS erase (factory reset)");
              resetreq = true;
              sndstr += String("NVS erased. Defaults restored. ESP32 is restarting...");
            } else {
              dbgprint("NVS erase failed!");
              eventLogRecordError("NVS erase failed");
              sndstr += String("NVS erase failed");
            }
            timerStart(timer);
          } else if (http_getcmd.startsWith("getalarms")) // Get alarm JSON config
          {
            String json = nvsGetAlarmConfig();
            if (json.length() == 0 || !isValidAlarmConfigJson(json)) {
              json = ini_block.alrm_config.length() > 0 ? ini_block.alrm_config : serializeAlarmConfig();
            } else {
              ini_block.alrm_config = json;
              parseAlarmConfig(json);
            }
            sendHttpJsonResponse(json);
            return;
          } else if (http_getcmd.startsWith("savealarms")) // Save alarm JSON config
          {
            if (writeAlarmConfig()) {
              sndstr += String("Alarm configuration saved");
            } else {
              eventLogRecordError("Web UI: alarm configuration save failed");
              sndstr += String("Alarm configuration save failed");
            }
          } else if (http_getcmd.startsWith("saveprefs")) // Is is a "Save preferences"
          {
            writeprefs();                               // handle it
            sndstr += String("Config saved");           // Give reply
          } else if (http_getcmd.startsWith("mp3list")) // Is is a "Get SD MP3 tracklist"?
          {
            if (datamode != STOPPED) // Still playing?
            {
              //              datamode = STOPREQD ;                         //
              //              Stop playing
            }
            cmdclient.print(sndstr); // send header
            n = listsdtracks("/");   // Handle it
            dbgprint("%d tracks found on SD card", n);
            closeHttpClient();
            return;                                      // Do not send empty line
          } else if (http_getcmd.startsWith("settings")) // Is is a "Get settings" (like presets
                                                         // and tone)?
          {
            cmdclient.print(sndstr); // send header
            getsettings();           // Handle settings request
            closeHttpClient();
            return;                  // Do not send empty line
          } else {
            p = analyzeCmd(http_getcmd.c_str()); // do so
            sndstr += String(p);                 // Content of HTTP response follows the header
          }
          sndstr += String("\n"); // The HTTP response ends with a blank line
          cmdclient.print(sndstr);
        } else {
          httpheader(String("text/html")); // Send header
          // the content of the HTTP response follows the header:
          cmdclient.println("Dummy response\n"); // Text ending with double newline
          dbgprint("Dummy response sent");
        }
    }
    closeHttpClient();
  }
}

//**************************************************************************************************
//                                        H A N D L E H T T P *
//**************************************************************************************************
// Handle the input of an http request. *
//**************************************************************************************************

/**
 * @brief Handle the input of an HTTP request.
 *
 * Reads the HTTP request from the client, parses GET/POST commands and
 * parameters, and extracts the command or filename for further processing.
 */
void handlehttp() {
  bool first = true;       // First call to httpReadByte()
  char c;                  // Next character from http input
  int inx0, inx;           // Pos. of search string in currenLine
  String currentLine = ""; // Build up to complete line
  bool reqseen = false;    // No GET seen yet
  uint32_t deadline = millis() + 5000;

  http_post_content_length = 0;
  http_getcmd = "";
  http_rqfile = "";
  http_if_none_match = "";
  http_if_modified_since = "";

  if (!cmdclient.connected()) // Action if client is connected
  {
    return; // No client active
  }
  webUiActivityMark();
  // dbgprint ( "handlehttp started" ) ;
  while (millis() < deadline) // Loop till command/file seen
  {
    c = httpReadByte(first, 120); // Get a byte (tolerate short network gaps)
    first = false;     // No more first call
    if (!cmdclient.connected()) {
      break;
    }
    if (c == '\n') {
      // If the current line is blank, you got two newline characters in a row.
      // that's the end of the client HTTP request, so send a response:
      if (currentLine.length() == 0) {
        http_response_flag = reqseen; // Response required or not
        if (reqseen && !http_getcmd.startsWith("uploadfirmware") &&
            http_post_content_length > 0 && http_post_content_length <= RADIO_CONFIG_MAX_LEN) {
          timerStop(timer);
          http_post_body_cache.reserve(http_post_content_length + 1);
          uint32_t targetLen = http_post_content_length;
          uint32_t bodyDeadline = millis() + 5000;
          while (http_post_body_cache.length() < targetLen) {
            int b = httpReadPostByte(bodyDeadline);
            if (b < 0) {
              break;
            }
            http_post_body_cache += (char)b;
          }
          timerStart(timer);
          http_post_content_length = 0;
        }
        break;
      } else {
        // Newline seen, remember if it is like "GET /xxx?y=2&b=9 HTTP/1.1"
        if (currentLine.startsWith("GET /")) // GET request?
        {
          inx0 = 5;                                  // Start search at pos 5
        } else if (currentLine.startsWith("POST /")) // POST request?
        {
          inx0 = 6;
        } else {
          inx0 = 0; // Not GET nor POST
        }
        if (currentLine.startsWith("Content-Length:") || currentLine.startsWith("content-length:")) {
          http_post_content_length = (uint32_t)currentLine.substring(15).toInt();
        }
        if (currentLine.startsWith("Host:") || currentLine.startsWith("host:")) {
          http_host_header = currentLine.substring(5);
          http_host_header.trim();
        }
        if (currentLine.startsWith("If-None-Match:") || currentLine.startsWith("if-none-match:")) {
          http_if_none_match = currentLine.substring(14);
          http_if_none_match.trim();
        }
        if (currentLine.startsWith("If-Modified-Since:") || currentLine.startsWith("if-modified-since:")) {
          http_if_modified_since = currentLine.substring(18);
          http_if_modified_since.trim();
        }
        if (inx0) // GET or POST request?
        {
          reqseen = true;                 // Request seen
          inx = currentLine.indexOf("&"); // Search for 2nd parameter
          if (inx < 0) {
            inx = currentLine.indexOf(" HTTP"); // Search for end of GET command
          }
          // Isolate the command
          http_getcmd = currentLine.substring(inx0, inx);
          inx = http_getcmd.indexOf("?"); // Search for command
          if (inx == 0)                   // Arguments only?
          {
            http_getcmd = http_getcmd.substring(1); // get rid of question mark
            http_rqfile = "";                       // No file
          } else if (inx > 0)                       // Filename present?
          {
            http_rqfile = http_getcmd.substring(0, inx);  // Remember filename
            http_getcmd = http_getcmd.substring(inx + 1); // Remove filename from GET command
          } else {
            http_rqfile = http_getcmd; // No parameters, set filename
            http_getcmd = "";
          }
          if (http_getcmd.length()) {
            dbgprint("Get command is: %s", // Show result
                     http_getcmd.c_str());
          }
          if (http_rqfile.length()) {
            dbgprint("Filename is: %s", // Show requested file
                     http_rqfile.c_str());
          }
        }
        currentLine = "";
      }
    } else if (c != '\r') // No LINFEED.  Is it a CR?
    {
      currentLine += c; // No, add normal char to currentLine
    }
  }

  // Server-Sent Events stream for live UI updates
  if (http_getcmd.startsWith("events") || http_rqfile == "events") {
    httpAttachSseClient(cmdclient);
    http_getcmd = "";
    http_rqfile = "";
    http_response_flag = false;
    return;
  }

  // Sends: sender name, song title, time and date as JSON
  if (http_getcmd.startsWith("getJSON")) {
    sendHttpJsonResponse(buildLiveStatusJson());
    return;
  }

  // Sends the IP address of the requesting client (used to prefill the media server base URL)
  if (http_getcmd.startsWith("clientip")) {
    String clientIp = cmdclient.remoteIP().toString();
    sendHttpJsonResponse(String("{\"ip\":\"") + jsonEscapeString(clientIp.c_str()) + "\"}");
    return;
  }

  // Proxy-scan a LAN http.server directory listing for MP3 files
  if (http_getcmd.startsWith("mediadir")) {
    String baseUrl = mediaProbeQueryParam(http_getcmd, "mediadir");
    baseUrl = mediaProbeUrlDecode(baseUrl);
    sendHttpJsonResponse(buildMediaDirectoryProbeJson(baseUrl));
    return;
  }

  if (http_getcmd.startsWith("getdiagnostics")) {
    sendHttpJsonResponse(buildDiagnosticsJson());
    return;
  }

#if defined(ST7701_28B)
  if (http_getcmd.startsWith("audioreinit")) {
    st7701RequestVs1053Reinit();
    sendHttpJsonResponse("{\"ok\":true,\"message\":\"VS1053 reinit scheduled\"}");
    return;
  }
#endif

  if (http_getcmd.startsWith("gethwprofiles")) {
    sendHttpJsonResponse(buildHardwareProfilesJson());
    return;
  }

  if (http_getcmd.startsWith("applyhwprofile=")) {
    String id = http_getcmd.substring(String("applyhwprofile=").length());
    int amp = id.indexOf('&');
    if (amp >= 0) {
      id = id.substring(0, amp);
    }
    id.trim();
    bool ok = applyHardwareProfile(id.c_str());
    String json = String("{\"ok\":") + (ok ? "true" : "false") +
                  ",\"hardware_profile\":\"" + id +
                  "\",\"message\":\"" +
                  (ok ? "Profile saved. Rebooting..." : "Unknown hardware profile") + "\"}";
    sendHttpJsonResponse(json);
    if (ok) {
      delay(400);
      ESP.restart();
    }
    return;
  }

  if (http_getcmd.startsWith("geteventlog")) {
    sendHttpJsonResponse(eventLogGetJson());
    return;
  }

  if (http_getcmd.startsWith("uploadfirmware")) {
    String uploadMessage;
    bool ok = handleFirmwareUploadPost(http_post_content_length, uploadMessage);
    otaActive = false; // Resume stream producer (on success the device restarts)
    String json = String("{\"ok\":") + (ok ? "true" : "false") + ",\"message\":\"" +
                  jsonEscapeString(uploadMessage.c_str()) + "\"}";
    sendHttpJsonResponse(json);
    return;
  }

  if (http_getcmd.startsWith("checkupdate")) {
    const bool forceRemote = http_getcmd.startsWith("checkupdate=1");
    otaChecker.check(forceRemote);
    sendHttpJsonResponse(otaChecker.buildCheckJson());
    return;
  }

  if (!reqseen) {
    dbgprint("HTTP request incomplete or timed out");
    closeHttpClient();
    return;
  }

  // cmdclient.stop() ;
}

//**************************************************************************************************
//                                     H T T P H E A D E R *
//**************************************************************************************************
// Set http headers to a string. *
//**************************************************************************************************

/**
 * @brief Generate HTTP response header string.
 *
 * Set @p cacheMaxAgeSec to a value >= 1 for cacheable static responses.
 *
 * @param contentstype The Content-Type header value (e.g., "text/html")
 * @param cacheMaxAgeSec Cache lifetime in seconds, 0 disables caching
 * @param contentLength Body length in bytes, or -1 to omit Content-Length
 * @return String Complete HTTP header
 */
String httpheader(String contentstype, uint16_t cacheMaxAgeSec,
                  const String &etag,
                  const String &lastModified,
                  int contentLength,
                  const String &contentEncoding) {
  String header = String("HTTP/1.1 200 OK\nContent-type:") + contentstype +
                  String("\n"
                         "Server: " NAME "\n"
                         "Connection: close\n");
  if (contentEncoding.length() > 0) {
    header += String("Content-Encoding: ") + contentEncoding + String("\n");
  }
  if (contentLength >= 0) {
    header += String("Content-Length: ") + String(contentLength) + String("\n");
  }
  header += String("Cache-Control: ");
  if (cacheMaxAgeSec > 0) {
    header += String("max-age=") + String(cacheMaxAgeSec) + "\n";
    if (etag.length() > 0) {
      header += String("ETag: ") + etag + String("\n");
    }
    if (lastModified.length() > 0) {
      header += String("Last-Modified: ") + lastModified + String("\n");
    }
    header += String("\n");
  } else {
    header += String("no-cache, no-store, must-revalidate\n"
                     "Pragma: no-cache\n\n");
  }
  return header;
}

/**
 * @brief Build a weak ETag for static web assets.
 *
 * @param resourceName Requested asset name
 * @param contentLength Asset content length in bytes
 * @return String Weak ETag value wrapped in quotes
 */
static String httpBuildWeakEtag(const String &resourceName, int contentLength) {
  String etag = String("W/\"") + FIRMWARE + String("-") + resourceName + String("-") +
                String(contentLength) + String("\"");
  etag.replace(" ", "_");
  return etag;
}

/**
 * @brief Check whether request cache validators match the current asset.
 *
 * @param etag Current asset ETag
 * @param lastModified Current asset Last-Modified value
 * @return true if cached response can be reused, false otherwise
 */
static bool httpIsAssetNotModified(const String &etag, const String &lastModified) {
  if (http_if_none_match.length() > 0) {
    if (http_if_none_match.indexOf(etag) >= 0 || http_if_none_match == "*") {
      return true;
    }
  }
  if (http_if_modified_since.length() > 0 && lastModified.length() > 0) {
    if (http_if_modified_since == lastModified) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Send a 304 response when browser cache is still valid.
 *
 * @param etag Current asset ETag
 * @param lastModified Current asset Last-Modified value
 * @param cacheMaxAgeSec Cache lifetime in seconds
 */
static void httpSendNotModified(const String &etag,
                                const String &lastModified,
                                uint16_t cacheMaxAgeSec) {
  cmdclient.print("HTTP/1.1 304 Not Modified\r\n");
  cmdclient.print("Connection: close\r\n");
  cmdclient.print("Cache-Control: max-age=");
  cmdclient.print(cacheMaxAgeSec);
  cmdclient.print("\r\n");
  if (etag.length() > 0) {
    cmdclient.print("ETag: ");
    cmdclient.print(etag);
    cmdclient.print("\r\n");
  }
  if (lastModified.length() > 0) {
    cmdclient.print("Last-Modified: ");
    cmdclient.print(lastModified);
    cmdclient.print("\r\n");
  }
  cmdclient.print("\r\n");
}
