#ifndef BOOT_VALIDATION_H
#define BOOT_VALIDATION_H

/**
 * @brief Count active NVS keys in the global key table.
 *
 * @return uint16_t Number of stored keys
 */
static uint16_t countNvsKeys() {
  uint16_t i = 0;

  while (nvskeys[i][0] != '\0' && i < MAXKEYS) {
    i++;
  }
  return i;
}

/**
 * @brief Validate WiFi credentials format SSID/password.
 *
 * @param wifi Raw NVS value
 * @return true if format is valid
 */
static bool isValidWifiEntry(const String &wifi) {
  int sep = wifi.indexOf('/');
  if (sep <= 0) {
    return false;
  }

  String ssid = wifi.substring(0, sep);
  String pass = wifi.substring(sep + 1);
  ssid.trim();
  pass.trim();
  return (ssid.length() > 0 && pass.length() > 0);
}

/**
 * @brief Check whether at least one radio preset exists in NVS.
 *
 * @return true if a preset entry was found
 */
static bool hasAnyPresetInNvs() {
  char key[12];
  uint16_t i;

  for (i = 0; i < MAXPRESETS; i++) {
    sprintf(key, "preset_%02d", i);
    if (nvsKeyExists(key) && nvsGetValue(key).length() > 0) {
      return true;
    }
  }

  return false;
}

/**
 * @brief Validate the current NVS configuration.
 *
 * @param reason Output text describing the validation failure
 * @return true if configuration is usable
 */
static bool validateNvsConfig(String &reason) {
  if (!nvsKeyExists("wifi_00")) {
    reason = "missing wifi_00";
    return false;
  }

  if (!isValidWifiEntry(nvsGetValue("wifi_00"))) {
    reason = "invalid wifi_00 format (expected SSID/password)";
    return false;
  }

  if (!nvsKeyExists("preset")) {
    reason = "missing active preset key";
    return false;
  }

  if (!hasAnyPresetInNvs()) {
    reason = "no radio presets found";
    return false;
  }

  if (!nvsKeyExists("volume")) {
    reason = "missing volume key";
    return false;
  }

  if (!nvsKeyExists("clk_server")) {
    reason = "missing time server key";
    return false;
  }

  if (nvsKeyExists("alrm_config")) {
    String alarmJson = nvsGetAlarmConfig();
    if (!isValidAlarmConfigJson(alarmJson)) {
      reason = "invalid alrm_config JSON";
      return false;
    }
  } else if (nvsKeyExists(RADIO_CONFIG_NVS_KEY)) {
    String radioJson = nvsGetRadioConfig();
    if (!isValidRadioConfigJson(radioJson)) {
      reason = "invalid radio_config JSON";
      return false;
    }
  }

  return true;
}

/**
 * @brief Validate NVS on boot and restore defaults if configuration is missing
 *        or corrupted.
 */
void checkAndInitConfig() {
  String reason;

  if (!nvsKeyExists("wifi_00")) {
    dbgprint("BOOT: No NVS configuration found (first start or reset)");
    restoreDefaultConfig();
    dbgprint("BOOT: Default configuration written, restarting...");
    delay(1000);
    ESP.restart();
    return;
  }

  if (!validateNvsConfig(reason)) {
    dbgprint("BOOT: Invalid NVS configuration detected: %s", reason.c_str());
    restoreDefaultConfig();
    dbgprint("BOOT: Default configuration restored, restarting...");
    delay(1000);
    ESP.restart();
    return;
  }

  dbgprint("BOOT: NVS configuration validation passed");
}

#endif
