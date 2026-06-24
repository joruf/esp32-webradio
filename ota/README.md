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
  "released": "2026-06-23",
  "bin_url": "https://example.com/path/firmware.bin",
  "notes": "Release notes"
}
```

The firmware compares the numeric part of `version` with the running `FIRMWARE` string.
If remote is newer, the web UI diagnostics page shows **Update available**.

## Building a release binary

```bash
pio run
```

Copy `.pio/build/esp32dev/firmware.bin` to your web server (e.g. `BINFILE` in `main.cpp`).

Update `ota/version.json` and commit when publishing a new release.

## On-device update

- **Diagnostics** page → **Check for updates**
- Or send `GET /?checkupdate=1`
- Or **Maintenance** / `/?update` for immediate flash from `UPDATEHOST` + `BINFILE`
