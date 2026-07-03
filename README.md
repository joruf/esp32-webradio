# ESP32 Webradio – Advanced Internet Radio with VS1053, TFT Display & Web Interface

[![PlatformIO](https://img.shields.io/badge/platformio-ESP32-blue)](https://platformio.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A fully‑featured, high‑performance Internet radio based on the **ESP32** dual‑core microcontroller, the **VS1053** hardware MP3/Ogg decoder and a colour TFT display.
The firmware is built around **FreeRTOS multitasking** with a dedicated stream‑producer task for gap‑free playback, **fail‑safe (non‑destructive) startup validation**, a **JSON REST API**, **rotary encoder** control, an **alarm clock**, a **sleep timer**, **WiFi auto‑discovery**, a **Power Log** for boot/shutdown and runtime diagnostics, and **robust OTA updates** – all managed through a modern, responsive web interface.

**Current firmware:** `v2.40` · **Source code:** [github.com/joruf/esp32-webradio](https://github.com/joruf/esp32-webradio)

---

## Project Structure

The source is organised into **domain folders** so that audio, networking, configuration, UI and system code are cleanly separated. All modules are header‑based and included from `main.cpp`.

```text
esp32-webradio/
├─ src/
│  ├─ main.cpp                     # Entry point, loop(), OTA trigger, task glue
│  ├─ audio/                       # Audio pipeline & VS1053
│  │  ├─ mp3.h                     # mp3loop(), mp3StreamTask (producer), stop_mp3client()
│  │  ├─ audio_stream_parser.h     # AudioStreamParser: ICY/MP3 parsing → data queue
│  │  ├─ stream.h                  # Queue consumer (playStreamFromQueue), TFT rendering
│  │  ├─ stream_watchdog.h         # Stall detection & soft reconnect
│  │  ├─ volume.h                  # Mute/volume, 5-step rounding, fade, getChipVolume()
│  │  ├─ VolumeMapper.h            # Maps user 0–100 → audible VS1053 range (40–100)
│  │  ├─ vs1053.h                  # VS1053 driver
│  │  ├─ vu_meter.h                # VU meter data
│  │  ├─ title_history.h           # Last recognised stream titles
│  │  ├─ media_probe.h             # LAN folder scan for the MP3 Stream page
│  │  └─ sd.h                      # SD helpers (disabled in the active build)
│  ├─ boot/
│  │  ├─ app_setup.h               # radioSetup(): tasks, pins, mutexes, OTA callbacks
│  │  └─ boot_validation.h         # Non-destructive NVS config validation
│  ├─ config/
│  │  ├─ firmware_config.h         # FIRMWARE version, display type, audio queue size
│  │  ├─ features_config.h         # Feature flags, OTA check URL
│  │  ├─ radio_config.h            # Unified radio_config JSON (NVS)
│  │  ├─ radio_config_merge.h       # Pure JSON merge helper (partial patches)
│  │  ├─ alarm_config.h            # Alarm JSON parsing
│  │  ├─ nvs.h                     # NVS read/write + debounced settings save
│  │  ├─ writeDefaultConfig.h      # Write factory defaults to NVS
│  │  └─ defaultprefs.h            # Factory default values
│  ├─ net/
│  │  ├─ http.h / http_queue.h     # HTTP server, JSON API, SSE, firmware upload
│  │  ├─ ota_check.h               # OTA manifest, version compare, binUrlForVersion()
│  │  ├─ ota_version_embedded.h    # Embedded fallback manifest (recent releases)
│  │  ├─ upgrade.h                 # OTA flash writer, isOtaUpdateCapable()
│  │  ├─ live_status.h             # Live status JSON (thread-safe metadata)
│  │  ├─ wifi_signal.h / wifi_display.h
│  │  ├─ captive_portal.h          # AP-mode captive portal
│  │  ├─ web_theme_serve.h         # Static asset serving (ETag/304)
│  │  └─ xml.h
│  ├─ system/
│  │  ├─ setup.h                   # WiFi scan/connect/AP mode, default GPIO pins
│  │  ├─ loop.h                    # timer10sec (bitrate & stall measurement)
│  │  ├─ time.h                    # NTP synchronisation
│  │  ├─ event_log.h               # Power Log: boot/shutdown/error/warn (NVS ring buffer)
│  │  ├─ event_log_util.h           # Ring-buffer helpers (testable, ArduinoJson only)
│  │  ├─ isr.h                     # Encoder interrupt handlers
│  │  ├─ spi.h                     # Shared SPI bus claim/release
│  │  ├─ string.h / helper.h       # Utilities
│  ├─ util/
│  │  └─ value_steps.h             # snapValueToStep() (volume & alarm-fade quantization)
│  ├─ input/
│  │  ├─ rotary_encoder.h          # Encoder state machine (5-step volume)
│  │  └─ encoder_switch.h          # Debounced push button
│  ├─ ui/
│  │  ├─ interface.h               # Command parser (analyzeCmd)
│  │  ├─ alarm.h                   # Sleep timer & alarm scheduler
│  │  └─ display.h                 # TFT segment rendering API
│  ├─ display/                     # Display drivers & screens
│  │  ├─ BlueTFT.h, ILI9341.h, SSD1306.h, LCD1602.h, NEXTION.h, DummyTFT.h
│  │  ├─ dsp_common.h, dsp_dashboard.h, dsp_ota_display.h
│  │  └─ St7735Display.h, ILI9341Display.h
│  ├─ globals/                     # Shared types, globals, prototypes
│  │  ├─ globals.h / globals.cpp, types.h, app_prototypes.h
│  └─ web_assets/                  # Embedded web UI (PROGMEM)
│     ├─ pages/                    # HTML views (*_page.h → *_html), incl. eventlog_page.h
│     ├─ partials/                 # nav_menu.html → nav_menu.h
│     ├─ styles/ scripts/ vendor/  # app.css.h, app.js.h, bootstrap, jQuery
│     ├─ icons/                    # favicon.ico.h
│     └─ web_assets.h              # Umbrella include
├─ ota/                            # version.json & release binaries (GitHub OTA)
│  └─ releases/                    # esp32_webradio_vX.YZ.bin
├─ test/                           # Host-based unit tests (pio test -e native)
│  ├─ test_value_steps/            # Volume/fade step quantization
│  ├─ test_volume_mapper/          # External → chip volume mapping
│  ├─ test_config_merge/           # Partial config patch merge behaviour
│  └─ test_event_log/              # Event-log ring buffer helpers
├─ partitions/ota_app.csv          # Dual OTA partition table (~1.875 MB per slot)
├─ scripts/pio_embed_web_assets.py # Build step: HTML/CSS/JS → PROGMEM headers
├─ docs/                           # Reference material & Web UI screenshots
├─ fritzing/                       # Circuit diagram sources
├─ android-app/                    # Optional Android companion app (WebView)
├─ platformio.ini
└─ README.md
```

---

## Key Features

- **Hardware Audio Decoding** – VS1053 handles MP3 / Ogg / AAC / WAV streams, offloading the ESP32.
- **Dedicated stream‑producer task** – A FreeRTOS task (`mp3StreamTask`) keeps the audio queue filled independently of the UI/main loop, and a separate consumer task (`playStreamFromQueue`) feeds the VS1053. This decoupling is what keeps playback gap‑free even during WiFi hiccups and web requests.
- **Stream watchdog** – Detects a stalled decoder and performs a lightweight soft reconnect before falling back to a full reset, so short network dropouts recover automatically.
- **Auto‑Discovery WiFi** – Stores multiple SSID/password pairs; falls back to Access Point mode with a captive portal if no known network is found.
- **Rotary Encoder Control**
  - Turn → volume up/down (in **5‑step increments**)
  - Single click → toggle play/stop
  - Double click → change station (preset selection)
- **Volume mapping & 5‑step increments** – The user‑facing scale is **0–100** but is linearly mapped onto the audible VS1053 range (**40–100**), so the whole travel is useful. Volume always changes in **5‑step increments**, both from the encoder and from the web UI (enforced in the browser and on the device). Volume changes are persisted with a short debounce so the last level survives a power cycle.
- **Rich Web Interface** – Bootstrap + jQuery front‑end backed by a JSON API.
  - Real‑time metadata (station name, song title) via SSE (`/?events`) with polling fallback
  - Pages: Webradio, **MP3 Stream** (LAN playback from PC/phone), Scheduler, Configuration, Sound, Display, **OTA Update**, Maintenance, **Power Log**, Diagnostics, About
  - Volume, tone control (bass/treble gain & frequency)
  - Preset management (add, edit, delete, drag‑and‑drop reorder)
  - Snooze timer & alarm scheduler (weekdays, multiple alarms, fade‑in in **5‑second steps**)
- **Power Log** – The last **20** boot, shutdown, error and warning events are stored in NVS and shown on `eventlog.html`.
  - **Boot / shutdown** – Planned restarts (user, firmware update, NVS erase, stream stall recovery) and unexpected ones (power loss, brownout, crash, watchdog) inferred from `esp_reset_reason()`
  - **Errors & warnings** – Runtime issues such as WiFi loss, stream connect failures, watchdog stalls, OTA/upload failures, config save errors and VS1053 detection problems
  - Timestamps from NTP when available, otherwise “unknown time” (boot time is patched once the clock syncs)
  - Identical messages are **deduplicated** (60 s window) to limit flash wear
- **Partial configuration saves** – Saving a single setting (e.g. alarm fade on the Scheduler page) sends only that patch; volume, station and unrelated feature flags are not reverted from stale NVS data.
- **Robust OTA firmware updates**
  - Online check against a GitHub manifest, one‑click install of the latest release
  - **Install a specific release** – including **older ones (downgrade)** – picked from a list that is **auto‑detected online** from the repository `ota/releases/` folder, so only firmware images that really exist are offered
  - **Local `.bin` upload** with a clear brick‑risk warning and **validation** (minimum/maximum size, ESP32 `0xE9` image signature and chip type) in the browser **and** on the device before flashing
  - The OTA screen is drawn without artifacts and shows size and estimated time (dashboard rendering is paused during flashing)
- **Alarm & Sleep Timer** – Wake‑up alarms on the **Scheduler** page; snooze timer on the index page (`/?snooze=XX`).
- **Automatic TFT Backlight Control** – Turns on for any interaction, powers off after inactivity.
- **Playlist Support** – Handles `.m3u` playlists (remote streams).
- **Non‑Volatile Storage (NVS)** – Settings survive reboots via a unified `radio_config` JSON (plus legacy keys). Startup validation is **non‑destructive**: the configuration is no longer wiped on transient NVS read errors.
- **Thread‑safe metadata** – Shared stream metadata (station/title) is guarded by a mutex to avoid cross‑task heap corruption.
- **Host unit tests** – Hardware‑independent logic (config merge, volume mapping, step quantization, event‑log ring buffer) is covered by native tests runnable with `pio test -e native` (no ESP32 required).
- **Multiple Display Types** – ST7735, ST7789, ILI9341, SSD1306, LCD1602, NEXTION and a dummy (serial‑only) display – switchable in `config/firmware_config.h`.
- **Android Companion App** – Optional WebView app in `android-app/` (same web UI, no firmware changes).

> **Current hardware setup:** SD card playback and IR remote are **not** used in the active firmware. Legacy code paths remain in the repository for optional future use.

---

## Web UI Screenshots

Captured from a live device to document the current look and layout of each page.

### Main navigation pages

- **Webradio Dashboard** (`index.html`) – Live metadata, playback controls, stream input, preset list, and display toggles.
  ![Webradio Dashboard](docs/screenshots/webui/index.png)
- **MP3 Stream** (`mp3stream.html`) – Stream MP3 files from your PC or phone over the local network (see [MP3 Stream – play your own music](#mp3-stream--play-your-own-music) below).
  ![MP3 Stream](docs/screenshots/webui/mp3stream.png)
- **Scheduler** (`scheduler.html`) – Snooze timer, multi‑alarm scheduling with weekday selection, and alarm fade‑in (5–60 s, 5 s steps).
  ![Scheduler](docs/screenshots/webui/scheduler.png)
- **Configuration** (`config.html`) – WiFi priorities, clock settings, startup preset, watchdog option, presets, and GPIO mapping.
  ![Configuration](docs/screenshots/webui/config.png)
- **Sound Settings** (`sound.html`) – Master output level and VS1053 treble/bass equalizer controls.
  ![Sound Settings](docs/screenshots/webui/sound.png)
- **Display Settings** (`display.html`) – Dark mode schedule and TFT VU meter behavior toggles.
  ![Display Settings](docs/screenshots/webui/display.png)
- **Maintenance** (`maintenance.html`) – Restart, erase storage, and raw JSON configuration editor.
  ![Maintenance](docs/screenshots/webui/maintenance.png)
- **Diagnostics** (`diagnostics.html`) – Runtime/device JSON diagnostics and refresh control.
  ![Diagnostics](docs/screenshots/webui/diagnostics.png)
- **Power Log** (`eventlog.html`) – Boot, shutdown, error and warning history with colour‑coded badges (newest first).
- **OTA Update** (`ota.html`) – Online update check/install, install a specific (auto‑detected) release, validated local `.bin` upload, release history, and manifest URL settings.
  ![OTA Update](docs/screenshots/webui/ota.png)
- **About** (`about.html`) – Project overview, feature summary, and source links.
  ![About](docs/screenshots/webui/about.png)

### Additional UI pages

- **Firmware Upgrade (legacy)** (`firmware.html`) – Legacy one‑click firmware installer view.
  ![Firmware Upgrade](docs/screenshots/webui/firmware.png)

---

## MP3 Stream – play your own music

The **MP3 Stream** page (`mp3stream.html`) lets you play MP3 files that live on your **computer or smartphone** through the webradio speakers.
The webradio does **not** read files directly from your hard drive. Instead, your PC/phone briefly acts as a small **music server**, and the ESP32 streams the file over your home Wi‑Fi — just like an internet radio station, but entirely on your local network.

### What you need

| Requirement | Why |
|-------------|-----|
| ESP32 webradio on your Wi‑Fi | Receives the stream and plays it through the VS1053 |
| PC, laptop, or phone on the **same Wi‑Fi** | Must reach the media server URL |
| MP3 files in one folder | e.g. `~/Music` or `~/Downloads` |
| **Python 3** on the device that serves the files (most Linux/macOS PCs; Windows with Python installed) | Runs the built‑in mini web server |

> **Important:** Paths like `C:\Music\song.mp3` or files picked in the browser **cannot** be played directly. The file must be reachable as an **HTTP URL** (e.g. `http://192.168.178.35:8000/song.mp3`).

### Step‑by‑step (for beginners)

1. **Open the MP3 Stream page** in your browser: `http://<radio-ip>/mp3stream.html`
   (Replace `<radio-ip>` with the address shown on the webradio display or in your router.)

2. **Prepare your music folder on the PC/phone** that will serve the files.
   Example: put MP3s in `Downloads` or create a folder `Music`.

3. **Start a small web server** in that folder:
   - Open a terminal / command prompt.
   - Go to the music folder, e.g. `cd ~/Downloads`
   - Run: `python3 -m http.server 8000`
   - Leave this window open while you listen. The server stops when you close it.

   On the MP3 Stream page you can **click** the blue `python3 -m http.server 8000` hint to **copy the command** to the clipboard, then paste it into your terminal.

4. **Check the Base URL** (step 1 on the page).
   The field **Media server Base URL** is filled in automatically with your device’s IP, e.g.
   `http://192.168.178.35:8000/`
   If it is empty, enter `http://<your-pc-ip>:8000/` (same IP as the computer running Python, port **8000**).

5. **Choose music** (step 2):
   - **Single file** – pick one MP3; playback starts immediately and the **file name appears on the radio display**.
   - **Folder** – pick a folder; all MP3s are listed; **click a title** to start that stream immediately.

6. **Verify and start** (step 3): the page builds the full stream URL (e.g. `http://192.168.178.35:8000/MySong.mp3`). Press **START STREAM** if you entered the URL manually.

### Typical problems

| Symptom | Fix |
|---------|-----|
| No sound | Is `python3 -m http.server 8000` still running? Is the Base URL IP your **PC’s** IP (not the ESP32’s)? |
| Wrong IP in Base URL | Edit the field to `http://YOUR-PC-IP:8000/` |
| File not found (404) | Start the Python server **inside** the folder that contains the MP3 |
| Spaces/special characters in file names | Use the file/folder picker on the page — it encodes the URL correctly |

### Technical note

The webradio requests `GET /?station=<url>` internally. Special characters in URLs are decoded in firmware so encoded paths (e.g. `%20` for spaces, `%5B` for `[`) work reliably.

---

## Hardware Requirements

| Component                       | Recommended Model / Remarks                              |
|--------------------------------|----------------------------------------------------------|
| Microcontroller                | ESP32 Dev Module (any dual‑core version)                 |
| Audio Decoder                  | VS1053B breakout board (e.g. from Adafruit or generic)   |
| Display                        | ST7735 (160×128) / ST7789 / ILI9341 / SSD1306 / LCD1602  |
| Rotary Encoder                 | KY‑040 or similar with built‑in push button              |
| Amplifier Shutdown (optional)  | Use `pin_shutdown` or `pin_shutdownx` for external amp   |

---

## Wiring Guide (Default Pins)

The table below shows the **default pin assignments** as defined in the code.
You can change all pins in the web interface – the settings are saved in NVS.

| Function                | ESP32 Pin | Connected to                    |
|------------------------|-----------|---------------------------------|
| VS1053 CS              | 5         | CS                              |
| VS1053 DCS             | 32        | DCS (Data / Command)            |
| VS1053 DREQ            | 4         | DREQ                            |
| SPI SCK                | 18        | SCK (shared with TFT)           |
| SPI MISO               | 19        | MISO                            |
| SPI MOSI               | 23        | MOSI                            |
| TFT CS                 | 15        | TFT_CS                          |
| TFT DC                 | 2         | TFT_DC                          |
| TFT Backlight          | 33        | BL (active high)                |
| Rotary Encoder CLK     | 25        | CLK                             |
| Rotary Encoder DT      | 26        | DT                              |
| Rotary Encoder SW      | 27        | SW (push button)                |
| Amplifier Shutdown     | –         | (optional, set in preferences)  |

> The default pins are defined in `readIOprefs()` in `src/system/setup.h`.
> If you use a different display (e.g. I²C SSD1306) you need to adjust `pin_tft_scl` and `pin_tft_sda`.

---

## Installation & Configuration

This project is configured for **PlatformIO**.

- Clone the repository:

```bash
git clone https://github.com/joruf/esp32-webradio.git
cd esp32-webradio
```

- Open in VS Code with the PlatformIO extension installed.
- Build & Upload: PlatformIO automatically fetches the dependencies defined in `platformio.ini` (Adafruit GFX/ST7735, ArduinoJson, PubSubClient, and the button/task‑scheduler libraries).
- A pre‑build script (`scripts/pio_embed_web_assets.py`) converts the HTML/CSS/JS in `src/web_assets/` into PROGMEM headers, so the whole web UI is embedded in the firmware.

### Flash partitioning (important)

The device uses a **dual OTA partition table** (`partitions/ota_app.csv`) with **~1.875 MB per app slot** so the firmware has room to grow. The unused SPIFFS partition is minimised because all web assets are embedded in PROGMEM.

> Because changing the partition table cannot be done over OTA, a **one‑time USB flash** is required to move to this layout. The NVS offset/size are kept identical so your stored configuration survives that reflash. Afterwards, further updates can be installed over the air.

### Android companion app (optional)

Control the radio from your phone using the same web interface — no firmware changes required.

**Full install guide:** [android-app/README.md](android-app/README.md#install-on-your-android-device)

Short version:

1. Build the APK (`cd android-app && ./build-debug.sh`) or copy `app-debug.apk` to your phone.
2. Install via USB: `adb install -r app/build/outputs/apk/debug/app-debug.apk`
   Or open the APK on the phone and allow installation from unknown sources.
3. Open **ESP32 Webradio**, enter the radio’s IP address (e.g. `192.168.178.33`), tap **Save and open**.

Phone and ESP32 must be on the same Wi‑Fi.

### Running unit tests (optional)

Hardware‑independent helpers are tested on the host (no ESP32 connected):

```bash
pio test -e native
```

This runs **23** tests covering config merge (partial patches must not touch unrelated keys), volume mapping, 5‑step quantization and the event‑log ring buffer. The firmware build uses `default_envs = esp32dev` in `platformio.ini`; only `pio test -e native` builds the test environment.

---

## REST API & JSON Endpoint

The ESP32 responds to HTTP GET requests on port 80.
You can query the current radio status, change volume, select presets, etc.

- **JSON Live Metadata** – `GET /?getJSON` returns real‑time station and playback information.
  - Live updates on the index page via SSE (`GET /?events`) with polling fallback

- **Configuration (JSON)** – Unified settings document.
  - `GET /?getconfig` → full radio configuration
  - `POST /?saveconfig` → save JSON body to NVS (full document or **partial patch** – only keys present in the body are updated; unrelated parameters are left unchanged)
  - `GET /?getalarms` / `POST /?savealarms` → alarm scheduler JSON

- **Diagnostics** – `GET /?getdiagnostics` → chip, memory, flash, network and partition info (includes `ota_max_size`, the size of the target OTA partition).

- **Power Log** – `GET /?geteventlog` → JSON document `{"events":[…]}` with the last 20 boot/shutdown/error/warn entries (`ev`, `reason`, `time`, `known`, `fw`, `epoch`).

- **OTA**
  - `GET /?checkupdate=1` → version manifest comparison (also returns `ota_max_size`)
  - `GET /?update` → install the latest release over the air
  - `GET /?installversion=v2.34` → install a specific release (supports downgrade); the bin URL is derived from the manifest template
  - `POST /?uploadfirmware=1` → upload a local `.bin` (raw `application/octet-stream`); validated for size, `0xE9` signature and chip type before flashing

- **Preset Control**
  - `/?preset=5` → Play preset number 5
  - `/?uppreset=1` → Jump to next preset
  - `/?downpreset=1` → Jump to previous preset

- **Volume Control** (values are snapped to **5‑step increments**; the 0–100 scale is mapped to the audible VS1053 range)
  - `/?volume=75` → Set volume (rounded to the nearest 5)
  - `/?upvolume=5` / `/?downvolume=5` → Relative change

- **Playback Control**
  - `/?stop` → Stop playback
  - `/?station=http://example.com/stream.mp3` → Play a custom stream (not saved)

- **Tone Control (VS1053 DSP)**
  - `/?toneha=5` → Treble gain (0=off, 1…7=+1.5…+10.5 dB, 8…15=-1.5…-12 dB)
  - `/?tonehf=10` → Treble frequency limit (1…15 = 1…15 kHz)
  - `/?tonela=8` → Bass gain (0=off, 1…15 = +1…+15 dB)
  - `/?tonelf=11` → Bass frequency (2…15 = 10…140 Hz)

- **Timer Functions**
  - `/?snooze=30` or `/?sleeptime=30` → Stop playback after 30 minutes (`0` = disable)

- **System Control**
  - `/?reset` → Restart the ESP32

> **Note:** All commands return a short text confirmation. For full implementation details see `analyzeCmd()` in `src/ui/interface.h`.

---

## Alarm & Sleep Timer

- **Alarm Clock** – Configured on the **Scheduler** page (`scheduler.html`).
  - Multiple alarms with start/stop times and weekday selection, plus **alarm volume fade‑in** (5–60 s in **5‑second steps**)
  - At start time the radio plays the configured boot preset; at stop time it shuts off
  - Fade duration takes effect immediately after saving (no reboot required)
  - Stored as JSON in NVS (`alrm_config`, also part of `radio_config`)
  - Implemented in `config/alarm_config.h` and `ui/alarm.h` using NTP time (`system/time.h`)

- **Snooze / Sleep Timer** – Automatic shutoff for night‑time listening.
  - Use the drop‑down on the index page or send `/?snooze=XX` (also `/?sleeptime=XX`)
  - After the chosen minutes, the radio stops and the display turns off
  - Any interaction (encoder, button, web command) resets the timer
  - Implemented in `ui/alarm.h`

---

## Configuration Storage (NVS)

All user settings are stored in the ESP32’s **Non‑Volatile Storage (NVS)** partition.

**Primary:** `radio_config` – unified JSON document (WiFi, presets, GPIO, clock, volume, tone, alarms, feature flags). Load/save via `/?getconfig` and `/?saveconfig`. Partial `POST` bodies merge only the keys they carry (`config/radio_config_merge.h`); live volume/station/tone are preserved when saving unrelated settings.

**Event log:** `event_log` – JSON ring buffer of the last 20 boot/shutdown/error/warn events for the Power Log page.

**Legacy keys** (still read and migrated into `radio_config`):

- **Station Presets** – `preset_00`, `preset_01`, … · Format `"http://stream.url#Station Name"` (text after `#` is the display name)
- **WiFi Credentials** – `wifi_00`, `wifi_01`, … · Format `"SSID/Password"` (multiple networks with automatic fallback)
- **Audio Settings** – `volume`, `toneha`, `tonehf`, `tonela`, `tonelf` · Saved with a short debounce after a change and periodically as a fallback; restored on boot
- **Alarm Configuration** – `alrm_config` (JSON) or legacy `alrm_start` / `alrm_stop`
- **Time Configuration** – `clk_server`, `clk_offset`, `clk_dst`

> **Startup safety:** Boot validation (`boot/boot_validation.h`) no longer performs a factory reset on transient NVS read errors — the configuration is only rewritten when it is genuinely missing/unrecoverable, so overnight resets are avoided.

> **Editing:** Use the web UI (**Configuration**, **Scheduler**, **Sound**, **Display**) or the raw JSON editor under **Maintenance**. Review **Power Log** after incidents.

> **Implementation:** `config/radio_config.h`, `config/nvs.h`, `system/event_log.h`, `net/http.h`.

---

## Power Log

The **Power Log** page (`eventlog.html`) shows the last **20** persisted events from NVS. It helps trace reboots and software issues without a serial monitor.

| Event type | Badge | Examples |
|------------|-------|----------|
| `boot` | green | power‑on, planned restart, recovered from crash/watchdog |
| `shutdown` | red | user request, firmware update, power loss, brownout |
| `error` | dark red | stream connect failed, OTA write failed, VS1053 not detected, config save failed |
| `warn` | orange | WiFi lost, stream stalled (reconnecting), firmware upload rejected |

**When events are recorded**

- **Planned shutdowns** – Logged synchronously before `ESP.restart()` (user reset, OTA success, NVS erase, stream stall recovery, firmware upload).
- **Unexpected shutdowns** – Inferred on the next boot from `esp_reset_reason()` when no planned shutdown was recorded.
- **Every boot** – Logged during setup; timestamp patched once NTP is available.
- **Runtime errors/warnings** – Queued from any task (stream, HTTP, OTA) and flushed to NVS from the main loop; duplicate reasons within 60 s are dropped.

**API:** `GET /?geteventlog` → `{"events":[{"ev":"boot","reason":"…","time":"…","known":true,"fw":"v2.40 …","epoch":…}, …]}`

**Implementation:** `system/event_log.h`, `system/event_log_util.h`

---

## Display Support

The project includes ready‑to‑use drivers for several display types. Select the display in `config/firmware_config.h`.

- **ST7735 (160×128)** – Default display driver (`BlueTFT.h`, SPI). Modern dashboard layout (see below).
- **ST7789** – Same driver as ST7735 (`BlueTFT.h`, SPI, different initialisation)
- **ILI9341** – 240×320 resolution (`ILI9341.h`, SPI)
- **SSD1306 (OLED)** – 128×64 monochrome (`SSD1306.h`, I²C)
- **LCD1602 (I²C)** – 16×2 character display (`LCD1602.h`, I²C)
- **NEXTION** – HMI touch display (`NEXTION.h`, Serial)
- **Dummy** – No physical display, serial logging only (`DummyTFT.h`)

> **Unified API:** All displays share the same rendering interface (see `src/ui/display.h`), e.g. text segments, volume bar, time and optional battery level.

### BLUETFT dashboard (160×128)

On the default ST7735 display (`BLUETFT`), the firmware uses a compact dashboard layout without overflow:

```text
┌────────────────────────────────────────────┐
│ ● │ Mo 15.06.25          20:30 │  Header   │
├───┼────────────────────────────┼───────────┤
│ V │ SWR3 (Sender)              │     VU    │
│ O │────────────────────────────│     │     │
│ L │ Songtitel mehrzeilig       │     █     │
│   │ (ICy-Metadaten)            │     │     │
│ % │────────────────────────────│     │     │
│   │ Radio on / Mute …          │           │
│   │ MeinWLAN        ▓▓▓░ 87%   │           │
└───┴────────────────────────────┴───────────┘
 12px        136px Inhalt              12px
```

| Zone | Content |
|------|---------|
| Header (15 px) | Date, time, play indicator (green dot when streaming) |
| Left gutter (12 px) | Vertical volume bar and percentage |
| Right gutter (12 px) | Vertical VU meter |
| Content (136 px) | Station name, wrapped song title (ICY metadata), status line |
| Bottom row | WiFi SSID, signal bar, quality in % (same curve as the Web UI) |

Implementation: `src/display/dsp_dashboard.h`, wired from `BlueTFT.h`.

---

## OTA Updates

Firmware updates are distributed via the GitHub repository and managed on the **OTA Update** page (`ota.html`).

- Repository: [github.com/joruf/esp32-webradio](https://github.com/joruf/esp32-webradio)
- Version manifest: `https://raw.githubusercontent.com/joruf/esp32-webradio/main/ota/version.json`
- Release binaries: `ota/releases/esp32_webradio_vX.YZ.bin`
- Details: [ota/README.md](ota/README.md)

The manifest URL is stored in `features.ota_check_url` (editable on the **OTA Update** page). If GitHub is unreachable, the device falls back to a small **embedded manifest** (`net/ota_version_embedded.h`) containing the most recent releases; the full changelog lives in `ota/version.json`.

### Ways to update

1. **Install latest** – Checks the manifest and installs the newest release over the air.
2. **Install a specific release (incl. downgrade)** – The Web UI queries the repository online (via the GitHub Contents API) to list the `.bin` files that actually exist under `ota/releases/`, and offers only those versions. Selecting one calls `/?installversion=<version>`; the firmware derives the matching bin URL from the manifest template.
3. **Local `.bin` upload** – Upload a firmware file directly from your PC/phone. Before flashing, the file is validated in the browser and on the device:
   - minimum and maximum size (must fit the OTA partition),
   - ESP32 application image signature (first byte `0xE9`),
   - chip type (`chip_id == 0` = ESP32).
   A prominent warning explains that flashing a wrong or unsuitable image can make the device unbootable and may require a USB reflash.

> **Safety:** The ESP32 `Update` library additionally validates the image on completion and only activates the new slot for a valid image. A failed or rejected upload keeps the currently running firmware intact.
