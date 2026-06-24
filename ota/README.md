# OTA firmware releases

This folder holds OTA metadata and release binaries for the ESP32 Webradio on GitHub.

## Files

| File | Purpose |
|------|---------|
| `version.json` | Version manifest (changelog + download URLs) |
| `releases/*.bin` | Published firmware binaries |

## Repository

[https://github.com/joruf/esp32-webradio](https://github.com/joruf/esp32-webradio)

## Default URLs (public GitHub repo)

| Resource | URL |
|----------|-----|
| Version manifest | `https://raw.githubusercontent.com/joruf/esp32-webradio/main/ota/version.json` |
| Firmware binary | `https://raw.githubusercontent.com/joruf/esp32-webradio/main/ota/releases/esp32_webradio_v2.19.bin` |

The radio stores the manifest URL in `features.ota_check_url` (OTA Update page).

## `version.json` format

```json
{
  "version": "v2.19",
  "released": "2026-06-24",
  "bin_url": "https://raw.githubusercontent.com/joruf/esp32-webradio/main/ota/releases/esp32_webradio_v2.19.bin",
  "notes": "Release notes for the latest build",
  "history": [
    {
      "version": "v2.19",
      "released": "2026-06-24",
      "notes": "Release notes"
    }
  ]
}
```

| Field | Purpose |
|-------|---------|
| `version` | Latest firmware label (compared with on-device `FIRMWARE`) |
| `released` | Release date (`YYYY-MM-DD`) |
| `bin_url` | HTTPS URL of the firmware binary |
| `notes` | Short summary for the latest release |
| `history` | Changelog array (newest first); shown on the OTA Update page |

## Publishing a new release

1. Build: `pio run`
2. Copy firmware: `cp .pio/build/esp32dev/firmware.bin ota/releases/esp32_webradio_vX.XX.bin`
3. Update `ota/version.json` (`version`, `released`, `bin_url`, `notes`, new `history` entry)
5. Commit and **push to GitHub** (repository must be **public** for `raw.githubusercontent.com`)

After changing `ota/version.json`, regenerate `src/handler/ota_version_embedded.h` from the same file before the next firmware build.

Until GitHub is reachable, the device uses the **embedded manifest** in firmware (also at `http://<radio-ip>/ota/version.json`).

## On-device update

- **OTA Update** page → **Check for updates** → **Install update**
- Or `GET /?checkupdate=1`
- Legacy fallback: `UPDATEHOST` + `BINFILE` in `main.cpp` (loresoft.de)
