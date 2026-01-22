/*
 * ESP32 Webradio - Config Reset Function
 * 
 * Diese Funktion schreibt alle Default-Einstellungen ins NVS
 * Einfach vor mk_lsan() aufrufen um die Config zu erneuern
 */

//**************************************************************************************************
//                              S A V E D E F A U L T C O N F I G                                  *
//**************************************************************************************************
// Speichert die komplette Default-Konfiguration ins NVS                                           *
// Überschreibt alle vorhandenen Einstellungen!                                                    *
//**************************************************************************************************
void saveDefaultConfig()
{
  dbgprint("╔═════════════════════════════════════════╗");
  dbgprint("║  Speichere Default-Konfiguration...     ║");
  dbgprint("╚═════════════════════════════════════════╝");
  
  // ═══════════════════════════════════════════════════════════════
  // WIFI KONFIGURATION
  // ═══════════════════════════════════════════════════════════════
  dbgprint("\n[1/7] WiFi-Einstellungen...");
  nvsAddValue("wifi_00", "LS/8541-7444-5177-6109-2070");
  // Optional: Weitere WiFi-Netzwerke (falls gewünscht)
  // nvsAddValue("wifi_01", "SSID2/PASSWD2");
  dbgprint("  ✅ WiFi konfiguriert");
  
  // ═══════════════════════════════════════════════════════════════
  // ALARM KONFIGURATION
  // ═══════════════════════════════════════════════════════════════
  dbgprint("\n[2/7] Alarm-Einstellungen...");
  nvsAddValue("alrm_start", "6:30");
  nvsAddValue("alrm_stop", "7:30");
  dbgprint("  ✅ Alarm konfiguriert");
  
  // ═══════════════════════════════════════════════════════════════
  // GPIO KONFIGURATION (Programmable Input Pins)
  // ═══════════════════════════════════════════════════════════════
  dbgprint("\n[3/7] GPIO-Einstellungen...");
  nvsAddValue("gpio_00", "uppreset = 1");
  nvsAddValue("gpio_12", "upvolume = 2");
  nvsAddValue("gpio_13", "downvolume = 2");
  nvsAddValue("gpio_14", "stop");
  nvsAddValue("gpio_17", "resume");
  nvsAddValue("gpio_34", "station = icecast.omroep.nl:80/radio1-bb-mp3");
  dbgprint("  ✅ GPIO konfiguriert");
  
  // ═══════════════════════════════════════════════════════════════
  // AUDIO EINSTELLUNGEN
  // ═══════════════════════════════════════════════════════════════
  dbgprint("\n[4/7] Audio-Einstellungen...");
  nvsAddValue("volume", "72");
  nvsAddValue("toneha", "0");     // Treble Amplitude
  nvsAddValue("tonehf", "0");     // Treble Frequency
  nvsAddValue("tonela", "0");     // Bass Amplitude
  nvsAddValue("tonelf", "0");     // Bass Frequency
  dbgprint("  ✅ Audio konfiguriert");
  
  // ═══════════════════════════════════════════════════════════════
  // PRESET (AKTIVER SENDER)
  // ═══════════════════════════════════════════════════════════════
  dbgprint("\n[5/7] Aktiver Preset...");
  nvsAddValue("preset", "6");     // Startet mit Preset 6 (Dance FM)
  dbgprint("  ✅ Preset konfiguriert");
  
  // ═══════════════════════════════════════════════════════════════
  // RADIO STATIONEN (PRESETS)
  // ═══════════════════════════════════════════════════════════════
  dbgprint("\n[6/7] Radio-Stationen...");
  nvsAddValue("preset_00", "charthits-high.rautemusik.fm # 0 - Rautemusik");
  nvsAddValue("preset_01", "stream.sunshine-live.de/live/mp3-192 # 1 - Sunshine Live");
  nvsAddValue("preset_02", "listen.technobase.fm/tunein-mp3 # 2 - Technobase.FM (HandsUp & Dance)");
  nvsAddValue("preset_03", "stream.slam.nl/slam_mp3 # 3 - SLAM Live");
  nvsAddValue("preset_04", "ic.radiomonster.fm/dance.ultra # 4 - RadioMonster Dance");
  nvsAddValue("preset_05", "ice1.somafm.com/groovesalad-128-mp3 # 5 - SomaFM");
  nvsAddValue("preset_06", "dancefm.net/listen.m3u # 6 - Dance FM (Global/US-Style)");
  nvsAddValue("preset_07", "stream.antenne1.de/a1stg/livestream2.mp3 # 7 - Antenne 1");
  nvsAddValue("preset_08", "topradio-de-hz-fal-stream03-cluster01.radiohost.de/kissfm_128 # 8 - Kiss FM");
  nvsAddValue("preset_09", "streamhq.top100station.com/top100station-high.mp3 # 9 - TOP 100 Station");
  nvsAddValue("preset_10", "stream.antenne1.de/80er/livestream2.mp3 # 10 - Antenne 1 80ger");
  nvsAddValue("preset_11", "play.antenne.de/dance-xxl.m3u # 11 - Antenne 1 Dance");
  nvsAddValue("preset_12", "liveradio.swr.de/sw331ch/swr3/play.aac # 12 - SWR3");
  dbgprint("  ✅ 13 Radio-Stationen konfiguriert");
  
  // ═══════════════════════════════════════════════════════════════
  // UHR / ZEITSERVER KONFIGURATION
  // ═══════════════════════════════════════════════════════════════
  dbgprint("\n[7/7] Zeitserver-Einstellungen...");
  nvsAddValue("clk_server", "pool.ntp.org");
  nvsAddValue("clk_offset", "1");      // UTC+1 (Deutschland Winter)
  nvsAddValue("clk_dst", "1");         // +1 Stunde Sommerzeit
  dbgprint("  ✅ Zeitserver konfiguriert");
  
  // ═══════════════════════════════════════════════════════════════
  // IR FERNBEDIENUNG (Optional)
  // ═══════════════════════════════════════════════════════════════
  // Optional: IR-Codes nur wenn IR-Fernbedienung vorhanden
  // nvsAddValue("ir_40BF", "upvolume = 2");
  // nvsAddValue("ir_C03F", "downvolume = 2");
  
  dbgprint("\n╔═════════════════════════════════════════╗");
  dbgprint("║  ✅ Default-Konfiguration gespeichert!  ║");
  dbgprint("╚═════════════════════════════════════════╝\n");
}
