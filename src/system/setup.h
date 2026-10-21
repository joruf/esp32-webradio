#include "config/gpio_valid.h"
#include "boot/st7701_boot_wdt.h"

/**
 * @brief Set I/O pin to "reserved". The pin is then not available for a
 * programmable function.
 *
 * @param rpinnr GPIO pin number to reserve
 */
void reservepin(int8_t rpinnr) {
  if (!gpioIsValid(rpinnr)) {
    return;
  }
  uint8_t i = 0;
  int8_t pin;

  // Reserve in progpin array
  while ((pin = progpin[i].gpio) >= 0) {
    if (pin == rpinnr) {
      if (progpin[i].reserved) {
        dbgprint("Pin %d is already reserved!", rpinnr);
      }
      progpin[i].reserved = true;
      break;
    }
    i++;
  }

  // Also reserve touch pin numbers
  i = 0;
  while ((pin = touchpin[i].gpio) >= 0) {
    if (pin == rpinnr) {
      touchpin[i].reserved = true;
      break;
    }
    i++;
  }
}

/**
 * @brief Read the encryption type of the network and return as a 4 byte name.
 *
 * @param thisType The WiFi authentication mode
 * @return const char* String representation of the encryption type
 */
const char *getEncryptionType(wifi_auth_mode_t thisType) {
  switch (thisType) {
    case WIFI_AUTH_OPEN:
      return "OPEN";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA_WPA2_PSK";
    case WIFI_AUTH_MAX:
      return "MAX";
    default:
      break;
  }
  return "????";
}

/**
 * @brief Check if a network SSID is acceptable (exists in wifilist).
 *
 * @param ssid The SSID to check
 * @return const char* "Acceptable" if found, empty string otherwise
 */
static const char *isAcceptableNetwork(const String &ssid) {
  for (int j = 0; j < wifilist.size(); j++) {
    WifiInfo_t winfo = wifilist[j];
    if (ssid.indexOf(winfo.ssid) == 0) {
      return "Acceptable";
    }
  }
  return "";
}

/**
 * @brief Display a single network on the TFT.
 *
 * @param index Network index (1-based)
 * @param ssid Network SSID
 * @param rssi Signal strength in dBm
 * @param displayCount Reference to counter for displayed networks
 */
static void renderNetworkOnTft(int index, const String &ssid, int rssi, int &displayCount) {
  if (displayCount >= MAX_DISPLAY_NETWORKS) {
    return;
  }

  char displayLine[40];
  String ssidShort = ssid;

  if (ssidShort.length() > 16) {
    ssidShort = ssidShort.substring(0, 16);
  }

  sprintf(displayLine, "%d-%s %ddBm", index, ssidShort.c_str(), rssi);
  tftlog(displayLine);
  delay(400);
  displayCount++;
}

/**
 * @brief List the available networks.
 *        Acceptable networks are those who have an entry in the preferences.
 *        SSIDs of available networks will be saved for use in web interface.
 *        Optimized: Shows only the strongest networks on the display.
 */
void listNetworks() {
  wifi_auth_mode_t encryption;
  const char *acceptable;
  int i;
  int displayCount = 0;
  char displayLine[40];

  dbgprint("Scan Networks");
  tftlog("Scan Networks");

  yield();
  st7701FeedTaskWdt();
  WiFi.scanDelete();
  (void)WiFi.scanNetworks(true);
  numSsid = WiFi.scanComplete();
  while (numSsid == WIFI_SCAN_RUNNING) {
    delay(50);
    yield();
    st7701FeedTaskWdt();
    numSsid = WiFi.scanComplete();
  }
  if (numSsid == WIFI_SCAN_FAILED) {
    numSsid = 0;
  }
  st7701FeedTaskWdt();
  dbgprint("Scan completed");
  tftlog("Scan Complete");

  if (numSsid <= 0) {
    dbgprint("Couldn't get a wifi connection");
    tftlog("Missing wifi connection");
    return;
  }

  dbgprint("Number of available networks: %d", numSsid);

  // Show filtering info on display
  if (numSsid > MAX_DISPLAY_NETWORKS) {
    sprintf(displayLine, "Found %d (show top %d)", numSsid, MAX_DISPLAY_NETWORKS);
  } else {
    sprintf(displayLine, "Found %d networks", numSsid);
  }
  tftlog(displayLine);
  delay(800);

  // Output all networks to serial monitor, but only strongest to display
  for (i = 0; i < numSsid; i++) {
#if defined(ST7701_28B)
    if ((i & 3) == 0) {
      st7701FeedTaskWdt();
    }
#endif
    acceptable = isAcceptableNetwork(WiFi.SSID(i));
    encryption = WiFi.encryptionType(i);

    // Always print to serial monitor
    dbgprint("%2d - %-25s Signal: %3d dBm, Encryption %4s, %s", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i),
             getEncryptionType(encryption), acceptable);

    // Display only strongest networks (scanNetworks sorts by signal strength)
    renderNetworkOnTft(i + 1, WiFi.SSID(i), WiFi.RSSI(i), displayCount);

    // Remember this network for later use
    networks += WiFi.SSID(i) + String("|");
  }

  wifiSignalStoreScanResults(numSsid);

  dbgprint("End of list");
  delay(1000);
}

/**
 * @brief Log the current network mode and assigned IP address.
 */
void logNetworkBootStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    ipaddress = WiFi.localIP().toString();
    if (ipaddress == "0.0.0.0") {
      dbgprint("BOOT: ERROR - WiFi connected but IP assignment failed");
      return;
    }

    dbgprint("BOOT: WiFi station connected");
    dbgprint("BOOT: SSID: %s", WiFi.SSID().c_str());
    dbgprint("BOOT: IP address: %s", ipaddress.c_str());
    dbgprint("BOOT: Gateway: %s", WiFi.gatewayIP().toString().c_str());
    dbgprint("BOOT: Signal: %d dBm", WiFi.RSSI());
    return;
  }

  wifi_mode_t mode = WiFi.getMode();
  if (mode == WIFI_AP || mode == WIFI_AP_STA) {
    ipaddress = WiFi.softAPIP().toString();
    if (ipaddress == "0.0.0.0") {
      dbgprint("BOOT: ERROR - AP mode active but IP assignment failed");
      return;
    }

    dbgprint("BOOT: WiFi AP mode active");
    dbgprint("BOOT: AP SSID: %s", NAME);
    dbgprint("BOOT: IP address: %s", ipaddress.c_str());
    return;
  }

  dbgprint("BOOT: ERROR - IP assignment not possible (WiFi not connected)");
}

/**
 * @brief Setup Access Point mode when WiFi connection fails.
 *
 * @return true Always returns true for AP mode
 */
static bool setupAccessPoint() {
  char *pfs;
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  dbgprint("BOOT: WiFi connection failed, starting access point: %s", NAME);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(NAME, NAME);
  delay(1000);

  ipaddress = WiFi.softAPIP().toString();
  if (ipaddress == "0.0.0.0") {
    dbgprint("BOOT: ERROR - AP started but no IP address assigned");
  } else {
    dbgprint("BOOT: AP IP address: %s", ipaddress.c_str());
  }
  captivePortalBegin();
  pfs = dbgprint("IP = %s", ipaddress.c_str());
  NetworkFound = true;
  wifiConfigApActive = true; // Block STA reconnect so the soft-AP stays usable
  tftlog(pfs);

  return false;
}

/**
 * @brief Connect to WiFi using the SSID's available in wifiMulti.
 *        If only one AP is found in preferences (i.e. wifi_00) the connection
 * is made without using wifiMulti. If connection fails, an AP is created and
 * the function returns false.
 *
 * @return true if connected to WiFi, false if in AP mode
 */
bool connectwifi() {
  char *pfs;
  bool localAP = false;
  WifiInfo_t winfo;

  wifiConfigApActive = false;
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);

  if (wifilist.size() > 0) {
    // Prefer a single WiFi.begin when every stored entry shares the same SSID
    // (duplicate LS rows break WiFiMulti auth and are common after config merges).
    bool sameSsid = true;
    for (size_t i = 1; i < wifilist.size(); i++) {
      if (!wifilist[i].ssid || !wifilist[0].ssid ||
          strcmp(wifilist[i].ssid, wifilist[0].ssid) != 0) {
        sameSsid = false;
        break;
      }
    }

    if (wifilist.size() == 1 || sameSsid) {
      winfo = wifilist[0];
      WiFi.mode(WIFI_STA);
      WiFi.begin(winfo.ssid, winfo.passphrase);
      dbgprint("BOOT: Connecting to WiFi network '%s'", winfo.ssid);
    } else {
      dbgprint("BOOT: Connecting to one of %d configured WiFi networks", wifilist.size());
      wifiMulti.run();
    }

    // Wait max 15 seconds for connection (poll so task WDT stays fed on ST7701)
    {
      const uint32_t connectDeadline = millis() + 15000UL;
      while (WiFi.status() != WL_CONNECTED && (int32_t)(millis() - connectDeadline) < 0) {
        delay(100);
        yield();
        st7701FeedTaskWdt();
      }
      if (WiFi.status() != WL_CONNECTED) {
        dbgprint("BOOT: WiFi connection timed out after 15 seconds (status %d)", (int)WiFi.status());
        eventLogRecordWarning("WiFi connect timed out, started config AP");
        localAP = true;
      }
    }
  } else {
    dbgprint("BOOT: No WiFi credentials found in configuration");
    eventLogRecordWarning("No WiFi credentials, started config AP");
    localAP = true;
  }

  if (localAP) {
    setupAccessPoint();
    return false;
  }

  ipaddress = WiFi.localIP().toString();
  captivePortalStop();
  wifiConfigApActive = false;
  tftlog((String("WiFi Name: ") + WiFi.SSID()).c_str());
  pfs = dbgprint("IP = %s", ipaddress.c_str());
  NetworkFound = true;
  tftlog(pfs);

  delay(500);
  tftlog("\f");
  return true;
}

/**
 * @brief Read the preferences for the programmable input pins.
 *
 * @param i Index in pin array
 * @param pinnr GPIO pin number
 * @param mykey Buffer for key name
 */
static void readProgrammablePin(int i, int8_t pinnr, char *mykey) {
  String val;

  if (!gpioIsValid(pinnr)) {
    return;
  }
  if (nvsKeyExists(mykey)) {
    val = nvsGetValue(mykey);
    if (val.length()) {
      if (!progpin[i].reserved) {
        progpin[i].avail = true;
        progpin[i].command = val;
        dbgprint("gpio_%02d will execute %s", pinnr, val.c_str());
      }
    }
  }
}

/**
 * @brief Read the preferences for the programmable touch pins.
 *
 * @param i Index in touch pin array
 * @param pinnr GPIO pin number
 * @param mykey Buffer for key name
 */
static void readTouchPin(int i, int8_t pinnr, char *mykey) {
  String val;

#if defined(CONFIG_IDF_TARGET_ESP32S3)
  (void)i;
  (void)pinnr;
  (void)mykey;
  return;
#else
  if (nvsKeyExists(mykey)) {
    val = nvsGetValue(mykey);
    if (val.length()) {
      if (!touchpin[i].reserved) {
        touchpin[i].avail = true;
        touchpin[i].command = val;
        dbgprint("touch_%02d will execute %s", i, val.c_str());
        dbgprint("Level is now %d", touchRead(pinnr));
      } else {
        dbgprint("touch_%02d pin (GPIO%02d) is reserved for I/O!", i, pinnr);
      }
    }
  }
#endif
}

/**
 * @brief Read the preferences for the programmable input pins and the touch
 * pins.
 */
void readprogbuttons() {
  char mykey[20];
  int8_t pinnr;
  int i;

  // Scan all programmable pins
  for (i = 0; (pinnr = progpin[i].gpio) >= 0; i++) {
    sprintf(mykey, "gpio_%02d", pinnr);
    readProgrammablePin(i, pinnr, mykey);
  }

  // Scan all touch pins (0..9, identified by their GPIO pin number)
  for (i = 0; (pinnr = touchpin[i].gpio) >= 0; i++) {
    sprintf(mykey, "touch_%02d", i);
    readTouchPin(i, pinnr, mykey);
  }
}

/**
 * @brief Structure for I/O pin configuration mapping.
 */
struct iosetting {
  const char *gname; // Name in preferences
  int8_t *gnr;       // GPIO pin number
  int8_t pdefault;   // Default pin
};

/**
 * @brief List of I/O related keys and their default pin mappings.
 */
static const struct iosetting klist[] = {
    {"pin_enc_clk", &ini_block.enc_clk_pin, 25},      {"pin_enc_dt", &ini_block.enc_dt_pin, 26},
    {"pin_enc_sw", &ini_block.enc_sw_pin, 27},        {"pin_ir", &ini_block.ir_pin, 35},
    {"pin_sd_cs", &ini_block.sd_cs_pin, 21},          {"pin_tft_cs", &ini_block.tft_cs_pin, 15},
    {"pin_tft_dc", &ini_block.tft_dc_pin, 2},         {"pin_vs_cs", &ini_block.vs_cs_pin, 5},
    {"pin_vs_dcs", &ini_block.vs_dcs_pin, 32},        {"pin_vs_dreq", &ini_block.vs_dreq_pin, 4},
    {"pin_tft_scl", &ini_block.tft_scl_pin, -1},      {"pin_tft_sda", &ini_block.tft_sda_pin, -1},
    {"pin_tft_bl", &ini_block.tft_bl_pin, 33},        {"pin_tft_blx", &ini_block.tft_blx_pin, -1},
    {"pin_shutdown", &ini_block.vs_shutdown_pin, -1}, {"pin_shutdownx", &ini_block.vs_shutdownx_pin, -1},
    {"pin_spi_sck", &ini_block.spi_sck_pin, 18},      {"pin_spi_miso", &ini_block.spi_miso_pin, 19},
    {"pin_spi_mosi", &ini_block.spi_mosi_pin, 23},    {NULL, NULL, 0}};

/**
 * @brief Process a single I/O preference entry.
 *
 * @param i Index in the I/O list
 * @return true if key was found and processed
 */
static bool processIoPrefEntry(int i) {
  int8_t *p = klist[i].gnr;
  const int8_t profileDefault = hardwareProfileDefaultPin(klist[i].gname, klist[i].pdefault);
  int8_t ival = profileDefault;
  String val;

  if (nvsKeyExists(klist[i].gname)) {
    val = nvsGetValue(klist[i].gname);
    if (val.length()) {
      ival = (int8_t)val.toInt();
    }
  }

  if (ival >= 0 && !gpioIsValid(ival)) {
    dbgprint("BOOT: %s GPIO%d invalid on this MCU, using profile default %d", klist[i].gname, ival,
             profileDefault);
    ival = profileDefault;
  }

  *p = ival;
  // Always reserve active pins (NVS or profile default). Fresh devices have no
  // pin_* keys yet; without this, boot pinMode(INPUT_PULLUP) steals PSRAM/SPI/SD.
  if (ival >= 0) {
    reservepin(ival);
  }
  dbgprint("%s set to %d", klist[i].gname, ival);

  return (val.length() > 0);
}

/**
 * @brief Force I/O pins to match the firmware build profile on dedicated board images.
 *
 * OTA or NVS from classic ESP32 (GPIO 22/23/25 encoder/SPI) must not leak into S3 builds.
 */
static void enforceBuildProfilePinMap(void) {
  const HardwareProfile *buildProfile = findHardwareProfile(HW_PROFILE_DEFAULT);
  if (buildProfile == NULL) {
    return;
  }

  String storedProfileId;
  if (nvsKeyExists("hw_profile")) {
    storedProfileId = nvsGetValue("hw_profile");
  }

  const bool profileMismatch =
      (storedProfileId.length() == 0) || (storedProfileId != String(HW_PROFILE_DEFAULT));
  bool repaired = false;

  for (size_t j = 0; j < buildProfile->pin_count; j++) {
    const char *key = buildProfile->pins[j].key;
    const int8_t expected = buildProfile->pins[j].value;

    int8_t current = expected;
    for (int k = 0; klist[k].gname != NULL; k++) {
      if (strcmp(klist[k].gname, key) == 0) {
        current = *klist[k].gnr;
        break;
      }
    }

    const bool invalidStored = (current >= 0 && !gpioIsValid(current));
    const bool pinDrift = (current != expected);
    if (!profileMismatch && !invalidStored && !pinDrift) {
      continue;
    }

    for (int k = 0; klist[k].gname != NULL; k++) {
      if (strcmp(klist[k].gname, key) == 0) {
        *klist[k].gnr = expected;
        if (expected >= 0) {
          reservepin(expected);
        }
        nvsAddValue(key, String((int)expected));
        repaired = true;
        dbgprint("BOOT: %s reconciled to %d (profile %s)", key, expected, HW_PROFILE_DEFAULT);
        break;
      }
    }
  }

  if (profileMismatch) {
    nvsAddValue("hw_profile", String(HW_PROFILE_DEFAULT));
    repaired = true;
    dbgprint("BOOT: hw_profile set to build default %s", HW_PROFILE_DEFAULT);
  }

  if (repaired) {
    nvsCommit();
  }
}

/**
 * @brief Reserve board-fixed pins that are not listed as configurable I/O prefs.
 *
 * ESP32-CAM PSRAM uses GPIO16/17. SD_MMC 1-bit uses GPIO2/14/15. Touching those
 * with pinMode before WiFi starts corrupts heap and crashes SoftAP init.
 */
static void reserveBoardFixedPins(void) {
#if defined(BOARD_HAS_PSRAM) && !defined(CONFIG_IDF_TARGET_ESP32S3)
  // Classic ESP32 external PSRAM on GPIO16/17.
  reservepin(16);
  reservepin(17);
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S3) && defined(BOARD_HAS_PSRAM)
  // ESP32-S3 OPI PSRAM occupies GPIO33..37.
  for (int8_t p = 33; p <= 37; p++) {
    reservepin(p);
  }
#endif
#if defined(ST7701_28B)
  // Waveshare ESP32-S3-Touch-LCD-2.8B internal buses (do not steal for progpin).
  // s3_touch_28b: VS1053 on 0/4/43/44/19/20; GPIO19/20 are USB D-/D+ (OTA-only while wired).
  // GPIO26-32 flash, 33-37 PSRAM: not GPIO. Onboard RGB/I2C/touch (not progpin/touchRead).
  const int8_t kS3Fixed[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                             17, 18, 19, 20, 21, 38, 39, 40, 41, 43, 44, 45, 46, 47, 48};
  for (size_t i = 0; i < sizeof(kS3Fixed) / sizeof(kS3Fixed[0]); i++) {
    reservepin(kS3Fixed[i]);
  }
#endif
  if (strcmp(hardwareProfileSdBackend(), "mmc") == 0) {
    reservepin(2);  // SD_MMC D0
    reservepin(14); // SD_MMC CLK
    reservepin(15); // SD_MMC CMD
  }
}

/**
 * @brief Scan the preferences for IO-pin definitions.
 */
void readIOprefs() {
  int i;
  int count = 0;

  for (i = 0; klist[i].gname; i++) {
    if (processIoPrefEntry(i)) {
      count++;
    }
  }
  enforceBuildProfilePinMap();
  reserveBoardFixedPins();
  (void)count;
}

/**
 * @brief Format a preference line for output, hiding passwords if needed.
 *
 * @param key The preference key
 * @param val The preference value
 * @param output Whether to format for output or not
 * @return String Formatted line
 */
static String formatPrefLine(const char *key, String val, bool output) {
  uint8_t winx;

  if (strstr(key, "wifi_")) {
    winx = atoi(key + 5);
    if ((winx < wifilist.size()) && (val.indexOf(wifilist[winx].ssid) == 0)) {
      val = String(wifilist[winx].ssid) + String("/*******");
    }
    if (!output) {
      return "";
    }
  }

  if (output) {
    return String(key) + String(" = ") + val + String("\n");
  }

  return String(key) + String(" = ") + val;
}

/**
 * @brief Read the preferences and interpret the commands.
 *        If output == true, the key/value pairs are returned to the caller as a
 * String.
 *
 * @param output If true, returns formatted string; if false, executes
 * analyzeCmd
 * @return String Formatted preference pairs or status message
 */
String readprefs(bool output) {
  uint16_t i = 0;
  String val;
  String cmd;
  String outstr = "";
  char *key;
  uint16_t last2char = 0;

  while (*(key = nvskeys[i])) {
    if (strcmp(key, ALRM_CONFIG_NVS_KEY) == 0) {
      val = nvsGetAlarmConfig();
    } else {
      val = nvsGetValue(key);
    }
    cmd = formatPrefLine(key, val, output);

    if (output) {
      if ((i > 0) && (*(uint16_t *)key != last2char)) {
        outstr += String("#\n");
      }
      last2char = *(uint16_t *)key;
      outstr += cmd;
    } else if (cmd.length() > 0) {
      analyzeCmd(cmd.c_str());
    }

    i++;
  }

  if (i == 0) {
    outstr = String("No preferences found.\n"
                    "Use defaults or run Esp32_radio_init first.\n");
  }

  return outstr;
}

/**
 * @brief Do some filtering on the input string:
 *        - Remove comment part (starting with "#")
 *        - Strip trailing CR
 *        - Strip leading spaces
 *        - Strip trailing spaces
 *
 * @param str The string to chomp (modified in place)
 */
void chomp(String &str) {
  int inx;

  if ((inx = str.indexOf("#")) >= 0) {
    str.remove(inx);
  }
  str.trim();
}

/**
 * @brief Find the namespace ID for the namespace passed as parameter.
 *
 * @param ns Namespace name to search for
 * @return uint8_t Namespace ID, or 0xFF if not found
 */
uint8_t FindNamespaceId(const char *ns) {
  esp_err_t result = ESP_OK;
  uint32_t offset = 0;
  uint8_t i;
  uint8_t bm;
  uint8_t res = 0xFF;

  while (offset < nvs->size) {
    result = esp_partition_read(nvs, offset, &nvsbuf, sizeof(nvsbuf));
    if (result != ESP_OK) {
      dbgprint("Error reading NVS!");
      break;
    }

    i = 0;
    while (i < 126) {
      bm = (nvsbuf.Bitmap[i / 4] >> ((i % 4) * 2));
      bm &= 0x03;

      if ((bm == 2) && (nvsbuf.Entry[i].Ns == 0) && (strcmp(ns, nvsbuf.Entry[i].Key) == 0)) {
        res = nvsbuf.Entry[i].Data & 0xFF;
        offset = nvs->size;
        break;
      } else if (bm == 2) {
        i += nvsbuf.Entry[i].Span;
      } else {
        i++;
      }
    }
    offset += sizeof(nvs_page);
  }

  return res;
}