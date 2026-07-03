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
  "version": "v2.40",
  "released": "2026-07-03",
  "repository": "https://github.com/joruf/esp32-webradio",
  "bin_url": "https://raw.githubusercontent.com/joruf/esp32-webradio/main/ota/releases/esp32_webradio_v2.40.bin",
  "notes": "Power Log now records runtime errors and warnings (WiFi, stream, OTA, config, VS1053) with deduplication and Error/Warning badges on the Power Log page.",
  "history": [
    {
      "version": "v2.40",
      "released": "2026-07-03",
      "notes": "Power Log now records runtime errors and warnings (WiFi, stream, OTA, config, VS1053) with deduplication and Error/Warning badges on the Power Log page."
    },
    {
      "version": "v2.39",
      "released": "2026-07-02",
      "notes": "New Power Log page: the last 20 shutdown and boot events are stored in NVS and shown in the Web UI. Planned shutdowns store their reason; unexpected ones (power loss, brownout, crash, watchdog) are inferred on the next boot. Boots are logged with date/time when NTP is available, otherwise as unknown time."
    },
    {
      "version": "v2.38",
      "released": "2026-07-02",
      "notes": "Saving a setting now only changes that one parameter: partial config saves no longer revert the current volume, station or tone and no longer reset unrelated feature flags to their defaults. Alarm volume fade-in is fixed (no volume jump, effective immediately) and is set in 5-second increments. Added host-based unit tests (pio test -e native)."
    },
    {
      "version": "v2.37",
      "released": "2026-07-02",
      "notes": "OTA page can now install a specific release (including older ones = downgrade) picked from an online-detected list of firmware images that really exist. Manual .bin uploads now show a brick-risk warning and are validated (size, ESP32 0xE9 signature and chip type) before flashing. Online volume now changes only in 5-step increments."
    },
    {
      "version": "v2.36",
      "released": "2026-07-02",
      "notes": "Stability and structure release: user volume 0-100 now maps onto the audible VS1053 range with 5-step encoder increments and immediate persistent save; fixed leftover third digit in the TFT volume readout; OTA update screen no longer shows artifacts and reliably shows size and estimated time (dashboard rendering is paused during flashing); thread-safe stream metadata, non-blocking client stop, HTTP read fix; enlarged app partitions and reorganized source into domain folders."
    },
    {
      "version": "v2.35",
      "released": "2026-06-28",
      "notes": "MP3 Stream page responsive layout: viewport meta tag and index.html-style panel structure for smartphones; mobile CSS for full-width action buttons, volume slider, and help text wrapping."
    },
    {
      "version": "v2.34",
      "released": "2026-06-28",
      "notes": "MP3 Stream page redesign: scan LAN folder via ESP32 proxy (?mediadir=), track list with Play all, sessionStorage keeps playback across Web UI navigation, and firmware reconnects interrupted LAN streams before Content-Length is complete."
    },
    {
      "version": "v2.33",
      "released": "2026-06-28",
      "notes": "New MP3 Stream page (mp3stream.html): play MP3 files from your PC or phone over the local network with a guided UI, auto-detected media server Base URL, copy-to-clipboard for the python3 HTTP server command, immediate playback on file or folder selection, and fixed URL decoding for encoded stream paths."
    },
    {
      "version": "v2.32",
      "released": "2026-06-27",
      "notes": "Improved OTA reliability and UX: static WebUI assets now use ETag/Last-Modified with conditional 304 responses, local OTA upload shows progress modal from upload start through install, and OTA TFT screen performs a full black clear to prevent leftover graphics."
    },
    {
      "version": "v2.31",
      "released": "2026-06-27",
      "notes": "Introduced local firmware upload via ota.html with direct installation, plus a unified modal workflow that blocks interactions during update and reports completion after the device service is reachable again. Applied the same wait/recovery UX to online OTA updates. Moved Alarm volume fade control from display page to scheduler page for better feature ownership."
    }
  ]
}
)=====";

#endif
