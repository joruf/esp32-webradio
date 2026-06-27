/**
 * @file ota_version_embedded.h
 * @brief Built-in OTA manifest (from ota/version.json).
 */

#ifndef OTA_VERSION_EMBEDDED_H
#define OTA_VERSION_EMBEDDED_H

const char ota_version_embedded_json[] PROGMEM = R"=====(
{
  "version": "v2.29",
  "released": "2026-06-27",
  "repository": "https://github.com/joruf/esp32-webradio",
  "bin_url": "https://raw.githubusercontent.com/joruf/esp32-webradio/main/ota/releases/esp32_webradio_v2.29.bin",
  "notes": "Audio startup noise reduction release: improved anti-pop stream start sequence with delayed amplifier enable and decoder warmup buffering, including an extended cold-boot warmup window for the first automatic preset start after power-on.",
  "history": [
    {
      "version": "v2.29",
      "released": "2026-06-27",
      "notes": "Reduced startup pop/pfffftt artifacts by decoupling amplifier enable from immediate volume writes and moving unmute/fade-in to decoder-confirmed start timing. Added stream warmup gating before enabling audio output and an extra one-time cold-boot warmup window for the very first automatic stream start after device boot."
    },
    {
      "version": "v2.28",
      "released": "2026-06-26",
      "notes": "Optimized live status updates to save resources: SSE remains primary, polling fallback is now adaptive (active/idle/background intervals) with retry backoff and visibility-aware scheduling. Added stable preset state synchronization from live payloads and settings load so the active preset button is always highlighted correctly after refresh. Simplified index preset UI by removing redundant current-label text and keeping visual active-state only. Display alarm fade setting now supports 0-60 seconds."
    },
    {
      "version": "v2.27",
      "released": "2026-06-26",
      "notes": "Fixed stream URL parsing for manually entered URLs (including scheme handling), added Enter key submission for Play Stream input, and stabilized stream bitrate reporting by preventing measurement spikes during counter resets. Index live status now reports stream_kbps via measured bitrate with fallback to header bitrate. Play button blink is now JS-driven in 1-second toggles while playback is active. Live stream meta panel is shown only when playback is active and stream kbps is greater than zero."
    },
    {
      "version": "v2.26",
      "released": "2026-06-26",
      "notes": "Diagnostics endpoint now includes chip, CPU, reset, memory, flash, network, and partition details for better incident analysis. OTA start screen now hard-clears the full display to black to prevent stale UI artifacts. Display settings page was modernized (time selects, fade slider, immediate save, start/end validation). Stream watchdog controls were moved from display to configuration with persistent autosave. Multiple pages were aligned to the newer sound-style UI layout."
    },
    {
      "version": "v2.25",
      "released": "2026-06-25",
      "notes": "Webserver stability release: robust HTTP request parsing to avoid partial-header timeouts, getconfig now prefers stored radio_config blob (NVS key scan fallback only on invalid/missing blob), favicon served as 204 No Content with caching, SSE live updates restored with automatic polling fallback, static pages/assets cached for 10 minutes, and Raw Configuration moved from config page to maintenance page."
    },
    {
      "version": "v2.24",
      "released": "2026-06-25",
      "notes": "BLUETFT dashboard: song title wraps across multiple lines (22 characters per line, up to 8 lines below station name). Web UI: faster page load — WiFi signal API uses scan cache (optional scan=1), OTA check returns embedded manifest without blocking HTTP, theme and preset list load after static assets, improved yield during large CSS/JS transfers."
    },
    {
      "version": "v2.23",
      "released": "2026-06-24",
      "notes": "BLUETFT song title on TFT using the same direct-refresh path as the station row (icystreamtitle, single line below station, content-column X offset). Bitrate label normalized to kbps on station change. Rotary encoder preset switch while streaming fixed."
    },
    {
      "version": "v2.22",
      "released": "2026-06-24",
      "notes": "Modern TFT dashboard for BLUETFT (160x128): vertical volume bar with % on the left, vertical VU meter on the right, header with date and time, WiFi SSID with signal bar and quality % (same curve as Web UI), station name, wrapped song metadata, status line, play indicator, optional battery line in header. No layout overflow."
    },
    {
      "version": "v2.21",
      "released": "2026-06-24",
      "notes": "OTA install fix: dual OTA partition table (ota_app.csv), HTTPS firmware download, restart only after successful update. Dark mode on all Web UI pages including preset select. Navigation from nav_menu.html (build-time embed). VU meter: 2px bar, improved activity display."
    },
    {
      "version": "v2.20",
      "released": "2026-06-24",
      "notes": "VU meter fix: visible yellow activity bar below the top line on BLUETFT/ILI9341 (displaytype set at boot). HTTP/SSE no longer block normal requests. Web UI GET queue, SSE polling fallback, config save race fixes. SPI claim timeout prevents display/audio deadlock."
    },
    {
      "version": "v2.19",
      "released": "2026-06-24",
      "notes": "Multi-client HTTP, SSE live updates, stream watchdog (auto-reconnect). Alarm fade-in, snooze rename. Auto dark mode (configurable hours). VU meter and WiFi/IP on TFT. Diagnostics page, captive portal, OTA version check. Android companion app. Display/behaviour and OTA subpages in Web UI."
    },
    {
      "version": "v2.18",
      "released": "2026-06-22",
      "notes": "Bugfix radio scheduler."
    },
    {
      "version": "v2.17",
      "released": "2026-06-19",
      "notes": "Alarm scheduler fix: NTP time via getLocalTime each second, weekday from SNTP, reliable minute matching, validated alarm times, auto-save on scheduler changes."
    },
    {
      "version": "v2.16",
      "released": "2026-06-19",
      "notes": "Stream metadata fixes: keep last title on empty blocks, robust StreamTitle parse, no permanent metadata disable on large blocks, JSON escaping in getJSON."
    },
    {
      "version": "v2.15",
      "released": "2026-06-19",
      "notes": "Song title history: last 10 recognized stream titles with date/time in Web UI JSON."
    },
    {
      "version": "v2.14",
      "released": "2026-06-18",
      "notes": "Web UI overhaul: responsive nav (Unicode icons), Scheduler alarm row layout, Maintenance page. Config: preset drag-and-drop reorder, collapsible raw JSON editor. Index: volume slider, separate play/stop, display on/off panel."
    },
    {
      "version": "v2.13",
      "released": "2026-06-17",
      "notes": "Alarm scheduler fixed: weekday check, reliable start/stop, per-second task handling. Boot preset select lists only configured presets. Scheduler alarm times as hh:mm masked inputs. Clock/NTP labels and tooltips. WEBUI_CACHE_MAX_AGE. Soft volume fade on play start/stop."
    },
    {
      "version": "v2.12",
      "released": "2026-06-16",
      "notes": "Configuration auto-saves on field change. UTC offset and DST as select boxes. Sound settings moved to Sound Settings page. HTTP POST config save fixed. Scheduler alarm remove persists to NVS."
    },
    {
      "version": "v2.11",
      "released": "2026-06-15",
      "notes": "Web config as JSON/NVS with form UI. Scheduler page (sleep timer, alarms). Preset reorder. Bugfixes playlist redirect, EEPROM/config load, alarm save."
    },
    {
      "version": "v2.10",
      "released": "2026-06-06",
      "notes": "Code switch from Arduino IDE to VSCode PlatformIO. VS1053 semaphore bugfixes. Time format expanded with date."
    },
    {
      "version": "v2.09",
      "released": "2026-01-22",
      "notes": "IR remote control recognition removed. defaultprefs.h structured."
    },
    {
      "version": "v2.08",
      "released": "2026-01-22",
      "notes": "writeDefaultConfig.h for default props in NVS on problems. SD card disabled. Display WiFi network list on TFT."
    },
    {
      "version": "v2.07",
      "released": "2026-01-21",
      "notes": "MQTT removed, mute button removed from index.html. Hardcoded GPIO pins for rotary encoder, TFT, and VS1053. Web interface layout adapted."
    },
    {
      "version": "v2.06",
      "released": "2026-01-17",
      "notes": "Bugfix alarm start/stop not working after ESP32 Espressif-Systems update to 3.3.0."
    },
    {
      "version": "v2.05",
      "released": "2026-01-16",
      "notes": "Rotary encoder click and double-click logic: click = radio on/off, double-click = select radio station."
    },
    {
      "version": "v2.04",
      "released": "2026-01-16",
      "notes": "Update ESP32 Espressif-Systems platform from v2.0.17 to 3.3.0."
    },
    {
      "version": "v2.03",
      "released": "2026-01-15",
      "notes": "Song title API for web interface (icystreamtitle and icyname). Sleep timer to stop MP3 stream after a defined time. Const VERSION renamed to FIRMWARE."
    },
    {
      "version": "v2.02",
      "released": "2025-05-07",
      "notes": "Bugfix HTTP server for root directory \"/\". HTML tags and stylesheets adapted for menu."
    },
    {
      "version": "v2.01",
      "released": "2025-03-27",
      "notes": "Responsive Design meta tag added to adapt content on different screen sizes."
    },
    {
      "version": "v2.00",
      "released": "2025-03-06",
      "notes": "Reload time changes on problems with the data connection."
    }
  ]
}
)=====";

#endif
