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
  "version": "v2.88",
  "released": "2026-10-23",
  "repository": "https://github.com/joruf/esp32-webradio",
  "bin_url": "https://raw.githubusercontent.com/joruf/esp32-webradio/main/ota/releases/esp32_webradio_v2.88.bin",
  "notes": "Display colstart/rowstart offsets; WiFi credentials always-save; password visibility toggle; WebUI app.js fix.",
  "history": [
    {
      "version": "v2.88",
      "released": "2026-10-23",
      "notes": "Display colstart/rowstart offsets; WiFi credentials always-save; password visibility toggle; WebUI app.js fix."
    },
    {
      "version": "v2.80",
      "released": "2026-10-21",
      "notes": "mDNS http + webradio services for LAN discovery (Android app 1.1.0 multi-radio)."
    },
    {
      "version": "v2.79",
      "released": "2026-10-21",
      "notes": "About page: author Joachim Ruf / Loresoft first; remove duplicate footer (v2.78 credits reorder)."
    },
    {
      "version": "v2.77",
      "released": "2026-10-21",
      "notes": "Display 180° rotation setting in Display Settings; Playback panel above Webradio Control."
    },
    {
      "version": "v2.75",
      "released": "2026-10-10",
      "notes": "YouTube links for all titles; ST7701 Touch-LCD and I2S/A2DP audio paths; WROOM TFT wiring (covers v2.72–v2.76 session work)."
    },
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
    }
  ]
}
)=====";

#endif
