
/**
 * @brief Open NVS with my-app namespace.
 *
 * Each application module, library, etc. has to use a namespace name to prevent
 * key name collisions. Storage is opened in RW mode.
 */
void nvsOpen() {
  if (!nvshandle) {

    nvserr = nvs_open(NAME, NVS_READWRITE, &nvshandle); // open nvs
    if (nvserr) {
      dbgprint("nvs_open failed!");
    }
  }
}

/**
 * @brief Clear all preferences from NVS.
 *
 * @return esp_err_t ESP_OK on success, otherwise error code
 */
esp_err_t nvsClear() {
  nvsOpen();                       // Be sure to open nvs
  return nvs_erase_all(nvshandle); // Clear all keys
}

/**
 * @brief Read a string value from NVS.
 *
 * @param key The key to look up
 * @return String The value associated with the key, or empty string on error
 */
String nvsGetValue(const char *key) {
  static char nvs_buf[NVSBUFSIZE]; // Buffer for contents
  size_t len = NVSBUFSIZE;         // Max length of the string, later real length

  nvsOpen();         // Be sure to open nvs
  nvs_buf[0] = '\0'; // Return empty string on error
  nvserr = nvs_get_str(nvshandle, key, nvs_buf, &len);
  if (nvserr) {
    dbgprint("nvs_get_str failed %X for key %s, keylen is %d, len is %d!", nvserr, key, strlen(key), len);
    dbgprint("Contents: %s", nvs_buf);
  }
  return String(nvs_buf);
}

/**
 * @brief Store a key/value pair in NVS.
 *
 * Length is limited to allow easy read-back. No write is performed if the value
 * hasn't changed.
 *
 * @param key The key to store
 * @param val The value to associate with the key
 * @return esp_err_t ESP_OK on success, otherwise error code
 */
esp_err_t nvsAddValue(const char *key, String val) {
  String curcont;    // Current contents
  bool wflag = true; // Assume update or new key

  // dbgprint("Setstring for %s: %s", key, val.c_str());
  if (val.length() >= NVSBUFSIZE) { // Limit length of string to store
    dbgprint("nvsAddValue length failed!");
    return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
  }

  if (nvsKeyExists(key)) {      // Already in nvs?
    curcont = nvsGetValue(key); // Read current value
    wflag = (curcont != val);   // Value change?
  }

  if (wflag) { // Update or new?
    dbgprint("nvsAddValue update value");
    nvserr = nvs_set_str(nvshandle, key, val.c_str()); // Store key and value
    if (nvserr) {
      dbgprint("nvsAddValue failed!");
    }
  }
  return nvserr;
}

/**
 * @brief Check if a key exists in NVS.
 *
 * @param key The key to check
 * @return true if the key exists, false otherwise
 */
bool nvsKeyExists(const char *key) {
  size_t len = NVSBUFSIZE; // Length of the string

  nvsOpen();                                        // Be sure to open nvs
  nvserr = nvs_get_str(nvshandle, key, NULL, &len); // Get length of contents
  return (nvserr == ESP_OK);                        // Return true if found
}

/**
 * @brief Auto-save volume, preset, and tone settings.
 *
 * This saves current settings to NVS every 10 minutes. On the next restart,
 * these values will be loaded. Saving only occurs if contents have changed.
 */
void nvsAutoSave() {
  static uint32_t savetime = 0; // Limit save to once per 10 minutes

  if ((millis() - savetime) < (1000 * 600)) { // 600 sec (10 min)
    return;
  }
  savetime = millis(); // Set time of last save

  nvsAddValue("preset", String(currentpreset));      // Save current preset
  nvsAddValue("volume", String(ini_block.reqvol));   // Save current volue
  nvsAddValue("toneha", String(ini_block.rtone[0])); // Save current toneha
  nvsAddValue("tonehf", String(ini_block.rtone[1])); // Save current tonehf
  nvsAddValue("tonela", String(ini_block.rtone[2])); // Save current tonela
  nvsAddValue("tonelf", String(ini_block.rtone[3])); // Save current tonelf
}

/**
 * @brief Read the MP3 host from preferences for a specific preset number.
 *
 * Searches for keys in the format "preset_x", "preset_xx", or "preset_xxx".
 *
 * @param preset The preset number to look up
 * @return String The host URL, or empty string if not found
 */
String nvsGetHostFromPref(int16_t preset) {
  char tkey[12]; // Key as an array of char

  sprintf(tkey, "preset_%d", preset); // Form the search key
  if (!nvsKeyExists(tkey))            // Does _x[x[x]] exists?
  {
    sprintf(tkey, "preset_%03d", preset); // Form new search key
    if (!nvsKeyExists(tkey))              // Does _xxx exists?
    {
      sprintf(tkey, "preset_%02d", preset); // Form new search key
    }
    if (!nvsKeyExists(tkey)) // Does _xx exists?
    {
      return String(""); // Not found
    }
  }
  // Get the contents
  return nvsGetValue(tkey); // Get the station (or empty sring)
}

/**
 * @brief Search for the next available MP3 host in preferences.
 *
 * Uses ini_block.newpreset as starting point and updates it when a valid host
 * is found. Wraps around to 0 if MAXPRESETS is reached.
 *
 * @return String The found host URL, or empty string if none found
 */
String nvsGetHostFromPref() {
  String contents = ""; // Result of search
  int maxtry = 0;       // Limit number of tries

  while ((contents = nvsGetHostFromPref(ini_block.newpreset)) == "") {
    if (++maxtry >= MAXPRESETS) {
      return "";
    }
    if (++ini_block.newpreset >= MAXPRESETS) // Next or wrap to 0
    {
      ini_block.newpreset = 0;
    }
  }
  // Get the contents
  return contents; // Return the station
}