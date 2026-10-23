#!/usr/bin/env bash
# Launch the USB build/flash helper UI for ESP32-Webradio.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec python3 "$ROOT/scripts/usb_flash_ui.py"
