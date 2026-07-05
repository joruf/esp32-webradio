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
  "version": "v2.45",
  "released": "2026-07-05",
  "repository": "https://github.com/joruf/esp32-webradio",
  "bin_url": "https://raw.githubusercontent.com/joruf/esp32-webradio/main/ota/releases/esp32_webradio_v2.45.bin",
  "notes": "Hardened config persistence: partial saves skip alrm_config writes, empty alarm patches ignored, no boot auto-reset on invalid alrm_config, legacy alrm_start/stop keys removed, Web UI no longer injects default alarms.",
  "history": [
    {
      "version": "v2.45",
      "released": "2026-07-05",
      "notes": "Hardened config persistence: partial saves skip alrm_config writes, empty alarm patches ignored, no boot auto-reset on invalid alrm_config, legacy alrm_start/stop keys removed, Web UI no longer injects default alarms."
    },
    {
      "version": "v2.44",
      "released": "2026-07-05",
      "notes": "Fixed alarm schedules silently reset to factory default (6:30-7:30) when saving unrelated settings. Partial saves seed alarms from authoritative alrm_config; Web UI no longer substitutes defaults on load errors; alarm reductions logged as warnings."
    },
    {
      "version": "v2.43",
      "released": "2026-07-03",
      "notes": "Fixed alarm schedule inconsistency: the Web UI could show a stale default alarm while the firmware scheduled a different one (alarms live in the alrm_config NVS key, the UI read a cached copy in radio_config). The UI now always shows the real active schedule, the cache is healed on boot, and alarm saves keep both in sync."
    },
    {
      "version": "v2.42",
      "released": "2026-07-03",
      "notes": "Fixed a rare Web UI hang (device online but unreachable for minutes): the live-status SSE push could block the main loop on a dead browser connection. SSE now only writes when the send buffer has room, drops a stuck client, and no longer calls the blocking flush."
    },
    {
      "version": "v2.41",
      "released": "2026-07-03",
      "notes": "The scheduler setting 'Alarm volume fade' now works both ways: alarms fade the volume in from silence, and the snooze/sleep timer fades the volume out gently over the same duration before stopping."
    },
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
    }
  ]
}
)=====";

#endif
