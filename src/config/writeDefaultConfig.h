/*
 * ESP32 Webradio - Config Reset Function
 * This function writes all default settings to the NVS.
 * Simply call it before mk_lsan() to refresh/reset the configuration.
 */

//**************************************************************************************************
//                              S A V E D E F A U L T C O N F I G *
//**************************************************************************************************
// Saves the complete default configuration to the NVS.
// Call restoreDefaultConfig() to erase broken settings first.
//**************************************************************************************************
void saveDefaultConfig() {
  dbgprint("BOOT: Writing default configuration to NVS...");

  // ═══════════════════════════════════════════════════════════════
  // WIFI KONFIGURATION
  // ═══════════════════════════════════════════════════════════════
  dbgprint("BOOT: [1/7] WiFi settings...");
  nvsAddValue("wifi_00", "LS/8541-7444-5177-6109-2070");
  // Optional: Weitere WiFi-Netzwerke (falls gewünscht)
  // nvsAddValue("wifi_01", "SSID2/PASSWD2");
  dbgprint("BOOT:   WiFi entry wifi_00 saved");

  // ═══════════════════════════════════════════════════════════════
  // ALARM CONFIGURATION
  // ═══════════════════════════════════════════════════════════════
  dbgprint("BOOT: [2/7] Alarm settings...");
  nvsAddValue("alrm_config",
              "{\"version\":1,\"alarms\":[{\"id\":1,\"enabled\":true,\"start\":\"6:30\","
              "\"stop\":\"7:30\",\"days\":[\"mon\",\"tue\",\"wed\",\"thu\",\"fri\"]}]}");
  dbgprint("BOOT:   Alarm configuration saved");

  // ═══════════════════════════════════════════════════════════════
  // GPIO CONFIGURATION (Programmable Input Pins)
  // ═══════════════════════════════════════════════════════════════
  dbgprint("BOOT: [3/7] GPIO settings...");
  nvsAddValue("gpio_00", "uppreset = 1");
  nvsAddValue("gpio_12", "upvolume = 2");
  nvsAddValue("gpio_13", "downvolume = 2");
  nvsAddValue("gpio_14", "stop");
  nvsAddValue("gpio_17", "resume");
  nvsAddValue("gpio_34", "station = icecast.omroep.nl:80/radio1-bb-mp3");
  dbgprint("BOOT:   GPIO pins saved");

  // ═══════════════════════════════════════════════════════════════
  // AUDIO SETTINGS
  // ═══════════════════════════════════════════════════════════════
  dbgprint("BOOT: [4/7] Audio settings...");
  nvsAddValue("volume", "72");
  nvsAddValue("toneha", "0"); // Treble Amplitude
  nvsAddValue("tonehf", "0"); // Treble Frequency
  nvsAddValue("tonela", "0"); // Bass Amplitude
  nvsAddValue("tonelf", "0"); // Bass Frequency
  dbgprint("BOOT:   Audio settings saved");

  // ═══════════════════════════════════════════════════════════════
  // ACTIVE PRESET
  // ═══════════════════════════════════════════════════════════════
  dbgprint("BOOT: [5/7] Active preset...");
  nvsAddValue("preset", "6"); // Startet mit Preset 6 (Antenne Bayern Dance XXL)
  dbgprint("BOOT:   Preset saved");

  // ═══════════════════════════════════════════════════════════════
  // RADIO STATIONS (PRESETS)
  // ═══════════════════════════════════════════════════════════════
  dbgprint("BOOT: [6/7] Radio stations...");
  nvsAddValue("preset_00", "charthits-high.rautemusik.fm # 0 - Rautemusik");
  nvsAddValue("preset_01",
              "stream.sunshine-live.de/live/mp3-192 # 1 - Sunshine Live");
  nvsAddValue(
      "preset_02",
      "listen.technobase.fm/tunein-mp3 # 2 - Technobase.FM (HandsUp & Dance)");
  nvsAddValue("preset_03", "stream.slam.nl/slam_mp3 # 3 - SLAM Live");
  nvsAddValue("preset_04",
              "ic.radiomonster.fm/dance.ultra # 4 - RadioMonster Dance");
  nvsAddValue("preset_05", "ice1.somafm.com/groovesalad-128-mp3 # 5 - SomaFM");
  nvsAddValue("preset_06",
              "s2-webradio.antenne.de/dance-xxl # 6 - Antenne Bayern Dance XXL");
  nvsAddValue("preset_07",
              "stream.antenne1.de/a1stg/livestream2.mp3 # 7 - Antenne 1");
  nvsAddValue("preset_08", "topradio-de-hz-fal-stream03-cluster01.radiohost.de/"
                           "kissfm_128 # 8 - Kiss FM");
  nvsAddValue("preset_09", "streamhq.top100station.com/top100station-high.mp3 "
                           "# 9 - TOP 100 Station");
  nvsAddValue("preset_10",
              "stream.antenne1.de/80er/livestream2.mp3 # 10 - Antenne 1 80ger");
  nvsAddValue("preset_11",
              "play.antenne.de/dance-xxl.m3u # 11 - Antenne 1 Dance");
  nvsAddValue("preset_12",
              "liveradio.swr.de/sw331ch/swr3/play.aac # 12 - SWR3");
  dbgprint("BOOT:   13 radio stations saved");

  // ═══════════════════════════════════════════════════════════════
  // TIME SERVER CONFIGURATION
  // ═══════════════════════════════════════════════════════════════
  dbgprint("BOOT: [7/7] Time server settings...");
  nvsAddValue("clk_server", "pool.ntp.org");
  nvsAddValue("clk_offset", "1"); // UTC+1 (Deutschland Winter)
  nvsAddValue("clk_dst", "1");    // +1 Stunde Sommerzeit
  dbgprint("BOOT:   Time server saved");
  migrateToRadioConfig();
  if (nvsCommit() != ESP_OK) {
    dbgprint("BOOT: WARNING - NVS commit after default config failed");
  }
  dbgprint("BOOT: Default configuration stored successfully");
}

/**
 * @brief Erase NVS and write a fresh default configuration.
 */
void restoreDefaultConfig() {
  dbgprint("BOOT: Clearing invalid NVS configuration...");
  nvsClear();
  fillkeylist();
  saveDefaultConfig();
  fillkeylist();
}
