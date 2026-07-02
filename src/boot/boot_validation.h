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
 * @brief Check whether at least one valid WiFi entry exists in NVS.
 *
 * @return true if a wifi_XX entry with SSID/password format was found
 */
static bool hasAnyValidWifiInNvs() {
  char key[10];
  uint8_t i;

  for (i = 0; i < 100; i++) {
    sprintf(key, "wifi_%02d", i);
    if (nvsKeyExists(key) && isValidWifiEntry(nvsGetValue(key))) {
      return true;
    }
  }

  return false;
}

/**
 * @brief Determine whether NVS is too broken to boot the radio.
 *
 * Factory defaults are written only when there is no usable WiFi configuration.
 *
 * @param reason Output text describing why boot is impossible
 * @return true if configuration cannot be recovered without a factory reset
 */
static bool isConfigUnrecoverable(String &reason) {
  if (countNvsKeys() == 0) {
    reason = "empty NVS";
    return true;
  }

  if (!hasAnyValidWifiInNvs()) {
    reason = "no valid WiFi credentials";
    return true;
  }

  return false;
}

/**
 * @brief Repair missing or invalid non-critical NVS keys without erasing user data.
 *
 * @param actions Output text listing repaired items (for logging)
 */
static void repairNvsConfig(String &actions) {
  bool repaired = false;

  if (!nvsKeyExists("volume")) {
    nvsAddValue("volume", "72");
    repaired = true;
    actions += " volume";
  }

  if (!nvsKeyExists("clk_server")) {
    nvsAddValue("clk_server", "pool.ntp.org");
    nvsAddValue("clk_offset", "1");
    nvsAddValue("clk_dst", "1");
    repaired = true;
    actions += " clock";
  }

  if (!nvsKeyExists("preset")) {
    nvsAddValue("preset", "0");
    repaired = true;
    actions += " preset";
  }

  if (!hasAnyPresetInNvs()) {
    nvsAddValue("preset_00", "icecast.omroep.nl:80/radio1-bb-mp3 # 0 - Default");
    repaired = true;
    actions += " presets";
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // DISABLED (config-reset investigation): automatic erasing of alrm_config /
  // radio_config keys on boot. A transient NVS read error could make a VALID
  // config look invalid and delete it, which then rebuilds from defaults.
  // Re-enable this block once the overnight config-reset cause is confirmed.
  // ───────────────────────────────────────────────────────────────────────────
  // if (nvsKeyExists(ALRM_CONFIG_NVS_KEY)) {
  //   String alarmJson = nvsGetAlarmConfig();
  //   if (!isValidAlarmConfigJson(alarmJson)) {
  //     nvsOpen();
  //     if (nvs_erase_key(nvshandle, ALRM_CONFIG_NVS_KEY) == ESP_OK) {
  //       repaired = true;
  //       actions += " alrm_config";
  //     }
  //   }
  // }
  //
  // if (nvsKeyExists(RADIO_CONFIG_NVS_KEY)) {
  //   String radioJson = nvsGetRadioConfig();
  //   if (!isValidRadioConfigJson(radioJson)) {
  //     nvsOpen();
  //     if (nvs_erase_key(nvshandle, RADIO_CONFIG_NVS_KEY) == ESP_OK) {
  //       repaired = true;
  //       actions += " radio_config";
  //     }
  //   }
  // }
  // ═══════════════════════════════════════════════════════════════════════════

  if (repaired && nvsCommit() == ESP_OK) {
    fillkeylist();
  }
}

/**
 * @brief Validate NVS on boot.
 *
 * Writes factory defaults only when the device cannot boot (empty NVS or no valid
 * WiFi credentials). Individual invalid keys are repaired in place instead of
 * wiping the full configuration.
 */
void checkAndInitConfig() {
  String reason;

  if (isConfigUnrecoverable(reason)) {
    // ═════════════════════════════════════════════════════════════════════════
    // DISABLED (config-reset investigation): automatic factory reset on boot.
    // This is the ONLY code path that wipes the entire NVS and rewrites all
    // defaults automatically. A transient NVS/partition read error at boot
    // (e.g. after a brownout or watchdog restart overnight) could make a valid
    // configuration look "unrecoverable" and trigger a full reset.
    //
    // While disabled, the radio keeps whatever is stored and just logs a
    // warning. If NVS is truly empty (first flash), uncomment saveDefaultConfig()
    // in app_setup.h once, or use the web UI "Restore factory defaults".
    //
    // To restore the previous automatic behaviour, re-enable the block below.
    // ─────────────────────────────────────────────────────────────────────────
    dbgprint("BOOT: NVS reported not bootable (%s) - AUTO-RESET DISABLED, keeping stored config", reason.c_str());
    // restoreDefaultConfig();
    // dbgprint("BOOT: Default configuration written, restarting...");
    // delay(1000);
    // ESP.restart();
    // return;
    // ═════════════════════════════════════════════════════════════════════════
  }

  repairNvsConfig(reason);
  if (reason.length() > 0) {
    dbgprint("BOOT: Repaired NVS without factory reset:%s", reason.c_str());
  }

  dbgprint("BOOT: NVS configuration validation passed");
}

#endif
