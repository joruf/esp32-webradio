/**
 * @file ota_version_embedded.h
 * @brief Built-in OTA manifest (from ota/version.json).
 *
 * This is only the offline fallback used when the remote manifest cannot be
 * reached. To keep the firmware small it contains just the most recent releases;
 * the full changelog lives in ota/version.json on the repository.
 */

#ifndef OTA_VERSION_EMBEDDED_H
#define OTA_VERSION_EMBEDDED_H

const char ota_version_embedded_json[] PROGMEM = R"=====(
{
  "version": "v2.71",
  "released": "2026-10-05",
  "repository": "https://github.com/joruf/esp32-webradio",
  "bin_url": "https://raw.githubusercontent.com/joruf/esp32-webradio/main/ota/releases/esp32_webradio_v2.71.bin",
  "notes": "Fix alarm save: keep stable NVS namespace separate from hostname radio; gzip CSS/JS for OTA size; alarm UI version fixed.",
  "history": [
    {
      "version": "v2.71",
      "released": "2026-10-05",
      "notes": "Fix alarm save: keep stable NVS namespace separate from hostname radio; gzip CSS/JS for OTA size; alarm UI version fixed."
    },
    {
      "version": "v2.68",
      "released": "2026-09-22",
      "notes": "Reduce stream stutter: SPI-safe VU render, skip idle volume SPI claims, soft-reconnect keeps audio queue."
    },
    {
      "version": "v2.67",
      "released": "2026-09-22",
      "notes": "Fix Offline wake sound/interval NVS persistence (ArduinoJson |nullptr, feat_* boot load, feat_fb_int key length)."
    },
    {
      "version": "v2.66",
      "released": "2026-09-22",
      "notes": "Fix Offline wake sound persistence: ArduinoJson |nullptr read, boot loads feat_* NVS keys."
    },
    {
      "version": "v2.65",
      "released": "2026-09-22",
      "notes": "Fix Offline wake sound and OTA URL not persisting (ArduinoJson | nullptr string read)."
    },
    {
      "version": "v2.64",
      "released": "2026-09-22",
      "notes": "Replace Mario/Tetris with original Arcade wake jingle; rename Play every label."
    },
    {
      "version": "v2.63",
      "released": "2026-09-22",
      "notes": "Offline wake melodies: Mario and Tetris sine motifs (cascade and other sounds unchanged)."
    },
    {
      "version": "v2.60",
      "released": "2026-09-22",
      "notes": "Offline wake sound: test button and selectable repeat interval."
    }
  ]
}
)=====";

#endif
