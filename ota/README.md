# OTA firmware releases

This folder documents over-the-air (OTA) update metadata for the ESP32 Webradio.

## Files

| File | Purpose |
|------|---------|
| `version.json` | Version manifest checked by the radio (`features.ota_check_url` in config) |

## `version.json` format

```json
{
  "version": "v2.19",
  "released": "2026-06-24",
  "bin_url": "https://example.com/path/firmware.bin",
  "notes": "Release notes for the latest build",
  "history": [
    {
      "version": "v2.19",
      "released": "2026-06-24",
      "notes": "Release notes"
    },
    {
      "version": "v2.18",
      "released": "2026-06-22",
      "notes": "Previous release notes"
    }
  ]
}
```

| Field | Purpose |
|-------|---------|
| `version` | Latest firmware label (compared with on-device `FIRMWARE`) |
| `released` | Release date (`YYYY-MM-DD`) |
| `bin_url` | HTTPS/HTTP URL of the firmware binary |
| `notes` | Short summary for the latest release |
| `history` | Optional changelog array (newest first); shown on the OTA Update page |

Each `history` entry may contain `version`, `released`, and `notes`.

The firmware compares the numeric part of `version` with the running `FIRMWARE` string.
If remote is newer, the web UI diagnostics page shows **Update available**.

## Building a release binary

```bash
pio run
```

Copy `.pio/build/esp32dev/firmware.bin` to your web server (e.g. `BINFILE` in `main.cpp`).

Update `ota/version.json` and publish it next to the firmware binary on your web server (default: `https://www.loresoft.de/downloads/version.json`).

### Publishing `version.json`

1. Copy `ota/version.json` to your download server, e.g. `www.loresoft.de/downloads/version.json`.
2. Ensure the file is served with `Content-Type: application/json` and is reachable over HTTPS.
3. Set **OTA version check URL** on the radio to that URL (or restore factory defaults after a firmware flash).

GitHub `raw.githubusercontent.com` URLs only work for **public** repositories. The repo `joruf/ESP32-Webradio` is not publicly available, so the previous default URL returned HTTP 404.

Alternatively, push this repository to GitHub as public and use:

`https://raw.githubusercontent.com/<user>/ESP32-Webradio/main/ota/version.json`

## On-device update

- **Diagnostics** page → **Check for updates**
- Or send `GET /?checkupdate=1`
- Or **Maintenance** / `/?update` for immediate flash from `UPDATEHOST` + `BINFILE`
