# ESP32 Webradio – Advanced Internet Radio with VS1053, TFT Display & Webinterface

[![PlatformIO](https://img.shields.io/badge/platformio-ESP32-blue)](https://platformio.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A fully‑featured, high‑performance Internet radio based on the **ESP32** dual‑core microcontroller, the **VS1053** hardware MP3/Ogg decoder and a colour TFT display.  
The firmware supports **multi‑tasking** (FreeRTOS), **fail‑safe startup**, **JSON REST API**, **rotary encoder** control, **alarm clock**, **sleep timer**, **WiFi scanning** and **OTA updates** – all managed through a modern, responsive web interface.

---


## Project Structure

```text
ESP32_Webradio/
├─ src/
│  ├─ main.cpp                     # Entry point, tasks, initialisation
│  ├─ handler/                     # Core modules
│  │  ├─ alarm.h                   # Sleep timer & alarm clock
│  │  ├─ display.h                 # TFT text/battery/volume rendering
│  │  ├─ http.h                    # HTTP server, JSON API
│  │  ├─ interface.h               # Command parser (analyzeCmd)
│  │  ├─ isr.h                     # Encoder & IR interrupt handlers
│  │  ├─ mp3.h                     # Streaming loop, data parsing
│  │  ├─ nvs.h                     # NVS read/write functions
│  │  ├─ rotary_encoder.h          # Encoder state machine
│  │  ├─ setup.h                   # WiFi scanning, connection, AP mode
│  │  ├─ stream.h                  # Audio queue & playback task
│  │  ├─ time.h                    # NTP synchronisation
│  │  ├─ volume.h                  # Mute / volume management
│  │  └─ vs1053.h                  # VS1053 driver
│  ├─ webinterface/                # Embedded web files (PROGMEM)
│  │  ├─ *.html.h, *.css.h, *.js.h
│  └─ display/                     # Display drivers
│     ├─ BlueTFT.h, ILI9341.h, ...
├─ platformio.ini
└─ README.md
```

---

## Key Features

- **Hardware Audio Decoding** – VS1053 handles MP3 / Ogg / AAC / WAV streams, offloading the ESP32.
- **Multi‑Core Tasking** – Audio streaming runs on Core 0, network/UI tasks on Core 1 for smooth playback.
- **Auto‑Discovery WiFi** – Stores multiple SSID/password pairs; falls back to Access Point mode if no known network is found.
- **Rotary Encoder Control** –  
  - Turn → volume up/down  
  - Single click → toggle play/stop  
  - Double click → change station (preset selection)  
  - Long press / triple click (configurable)
- **Rich Web Interface** – Built with Bootstrap 3, jQuery and a JSON API.  
  - Real‑time metadata (station name, song title)  
  - Volume, tone control (bass/treble gain & frequency)  
  - Preset management (add, edit, delete)  
  - Sleep timer & alarm clock configuration
- **Alarm & Sleep Timer** – Set a wake‑up time (radio starts automatically) and an auto‑shutoff after a chosen number of minutes.
- **Automatic TFT Backlight Control** – Turns on when any interaction occurs, dims/powers off after 5 minutes of inactivity.
- **SD Card MP3 Player** – Play MP3 files directly from a FAT‑formatted SD card (optional).
- **Playlist Support** – Handles `.m3u` playlists (local and remote).
- **Non‑Volatile Storage (NVS)** – All settings (presets, WiFi credentials, tone parameters, alarm times) survive reboots.
- **OTA Firmware Update** – Upgrade directly from the web interface.
- **Multiple Display Types** – Supports ST7735, ST7789, ILI9341, SSD1306, LCD1602, NEXTION and a dummy (serial only) display – easily switchable in the code.

---

## Hardware Requirements

| Component                       | Recommended Model / Remarks                              |
|--------------------------------|----------------------------------------------------------|
| Microcontroller                | ESP32 Dev Module (any dual‑core version)                 |
| Audio Decoder                  | VS1053B breakout board (e.g. from Adafruit or generic)   |
| Display                        | ST7735 (160×128) / ST7789 / ILI9341 / SSD1306 / LCD1602  |
| Rotary Encoder                 | KY‑040 or similar with built‑in push button              |
| SD Card Module (optional)      | SPI interface (e.g. microSD breakout)                    |
| IR Receiver (optional)         | VS1838B (or any 38 kHz receiver)                         |
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
| SPI SCK                | 18        | SCK (shared with SD and TFT)    |
| SPI MISO               | 19        | MISO                            |
| SPI MOSI               | 23        | MOSI                            |
| SD Card CS             | 21        | SD_CS                           |
| TFT CS                 | 15        | TFT_CS                          |
| TFT DC                 | 2         | TFT_DC                          |
| TFT Backlight          | 33        | BL (active high)                |
| Rotary Encoder CLK     | 25        | CLK                             |
| Rotary Encoder DT      | 26        | DT                              |
| Rotary Encoder SW      | 27        | SW (push button)                |
| IR Receiver            | 35        | IR out                          |
| Amplifier Shutdown     | –         | (optional, set in preferences)  |

> The default pins are defined in `ini_block` inside `readIOprefs()`.  
> If you use a different display (e.g. I²C SSD1306) you need to adjust `pin_tft_scl` and `pin_tft_sda`.

---

## Installation & Configuration

### This project is configured for PlatformIO.

- Clone the repository:

```Bash
cd ESP32_Webradio
git clone https://github.com/joruf/ESP32_Webradio.git
```

- Open in VS Code: Ensure the PlatformIO extension is installed.
- Build & Upload: Open the project folder in VS Code. PlatformIO will automatically fetch the required dependencies (Adafruit GFX/ST7735, PubSubClient, etc.) defined in your platformio.ini.
- Flash: Upload the firmware to your ESP32 via USB.

### Generating Doxygen Documentation

This repository includes a `Doxyfile` for generating API documentation from the `src` and `include` folders.

- Install Doxygen on Linux:

```bash
sudo apt install doxygen
```

- Generate the docs:

```bash
doxygen Doxyfile
```

- Open the generated HTML docs:

```bash
xdg-open docs/doxygen/html/index.html
```

The generated documentation is written to `docs/doxygen/html`.

---

## REST API & JSON Endpoint

The ESP32 responds to HTTP GET requests on port 80.  
You can query the current radio status, change volume, select presets, etc.

- **JSON Live Metadata** – `GET /?getJSON` returns real-time station and playback information.
  - Returns: `{"station":"Radio Paradise","title":"Bob Marley - Could You Be Loved","firmware":"v2.0","time":"14:35","date":"Sa 12.04.25"}`
  - Used by the web interface for automatic UI updates

- **Preset Control** – Change or navigate between saved radio stations.
  - `/?preset=5` → Play preset number 5
  - `/?uppreset=1` → Jump to next preset
  - `/?downpreset=1` → Jump to previous preset

- **Volume Control** – Adjust audio output level in real-time.
  - `/?volume=75` → Set volume to 75%
  - `/?upvolume=5` → Increase volume by 5%
  - `/?downvolume=5` → Decrease volume by 5%

- **Playback Control** – Start, stop or mute the audio stream.
  - `/?stop` → Stop playback
  - `/?station=http://example.com/stream.mp3` → Play a custom stream (not saved)

- **Tone Control (VS1053 DSP)** – Fine-tune bass and treble characteristics.
  - `/?toneha=5` → Treble gain (0=off, 1…7=+1.5…+10.5dB, 8…15=-1.5…-12dB)
  - `/?tonehf=10` → Treble frequency limit (1…15 = 1…15 kHz)
  - `/?tonela=8` → Bass gain (0=off, 1…15 = +1…+15 dB)
  - `/?tonelf=11` → Bass frequency (2…15 = 10…140 Hz)

- **Timer Functions** – Schedule automatic playback or shutdown.
  - `/?sleeptime=30` → Enable sleep timer for 30 minutes (0 = disable)

- **System Control** – Manage the device remotely.
  - `/?reset` → Restart the ESP32
  - `/?update` → Start OTA firmware update (from pre-defined URL)

> **Note:** All commands return a short text confirmation. For full implementation details see `analyzeCmd()` in `interface.h`.

---

## Alarm & Sleep Timer

- **Alarm Clock** – Fully automatic wake‑up functionality.
  - Set start and stop times in the **Configuration** page
  - At the specified start time, the radio automatically plays the `newpreset` station (the one selected in the web UI)
  - At the stop time it shuts off completely
  - The alarm is active every day (weekday selection can be added easily)
  - Implemented in `alarm.h` using NTP‑synchronised time (see `time.h`)

- **Sleep Timer** – Automatic shutoff for night-time listening.
  - Use the drop‑down menu in the web interface or send `/?sleeptime=XX`
  - After the chosen minutes, the radio stops and the display turns off
  - Any interaction (encoder, button, web command) resets the timer
  - Implemented in `alarm.h` with millisecond precision

---

## Configuration Storage (NVS)

All user settings are stored in the ESP32’s **Non‑Volatile Storage (NVS)** partition, ensuring all preferences survive reboots and power cycles.

- **Station Presets** – `preset_00`, `preset_01`, …
  - Format: `"http://stream.url#Station Name"`
  - Text after `#` is shown in the web interface station list

- **WiFi Credentials** – `wifi_00`, `wifi_01`, …
  - Format: `"SSID/Password"`
  - Supports multiple networks with automatic fallback

- **Audio Settings** – `volume`, `toneha`, `tonehf`, `tonela`, `tonelf`
  - Saved automatically every 10 minutes
  - Restored on boot

- **Alarm Configuration** – `alrm_start`, `alrm_stop`
  - Format: `"HH:MM"` (24-hour format)

- **Pin Assignments** – `pin_enc_clk`, `pin_enc_dt`, `pin_enc_sw`, `pin_ir`, etc.
  - Customisable without recompiling the firmware

- **Time Configuration** – `clk_server`, `clk_offset`, `clk_dst`
  - NTP server selection and timezone settings

> **Editing:** You can edit the raw preferences directly in the web interface (**Configuration → Configuration File**). After clicking **Save Configuration**, the changes are written to NVS and become active immediately – no restart needed for most settings.

> **Implementation:** NVS handling is found in `nvs.h` and `http.h` (functions `writeprefs()`, `readprefs()`).

---

## Display Support

The project includes ready‑to‑use drivers for several display types. Switch by including the appropriate header in `main.cpp` (or via conditional compilation).

- **ST7735 (160×128)** – Default display driver (`BlueTFT.h`, SPI interface)
- **ST7789** – Same driver as ST7735 (`BlueTFT.h`, SPI, different initialisation)
- **ILI9341** – 240×320 resolution (`ILI9341.h`, SPI, needs larger buffers)
- **SSD1306 (OLED)** – 128×64 monochrome (`SSD1306.h`, I²C interface)
- **LCD1602 (I²C)** – 16×2 character display (`LCD1602.h`, I²C interface)
- **NEXTION** – HMI touch display (`NEXTION.h`, Serial interface)
- **Dummy** – No physical display, serial logging only (`DummyTFT.h`)

> **Unified API:** All displays share the same programming interface:
> - `showTextOnTFT(segment, string)` – Display text on a specific screen segment
> - `displayVolumeOnTFT()` – Show volume level as a visual bar
> - `showTimeOnTFT()` – Display current time (right‑aligned)
> - `displayBatteryChargeOnTFT()` – Show battery level (if configured)

> **Layout:** See `display.h` for the segment layout configuration (top line, middle text, bottom area, and rotary encoder status line).
