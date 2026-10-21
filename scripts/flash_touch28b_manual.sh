#!/usr/bin/env bash
# Flash esp32s3_touch28b when USB-JTAG auto-reset fails (RTS protocol error).
# 1) Hold BOOT, tap RESET, release RESET, then release BOOT
# 2) Run this script within a few seconds
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PORT="${1:-/dev/ttyACM0}"
PIO="${HOME}/.platformio/penv/bin/pio"
ESPTOOL="${HOME}/.platformio/penv/bin/python -m esptool"
FW="$ROOT/.pio/build/esp32s3_touch28b"

cd "$ROOT"
"$PIO" run -e esp32s3_touch28b

echo "Waiting for download mode on $PORT (hold BOOT, tap RESET)..."
deadline=$((SECONDS + 60))
while (( SECONDS < deadline )); do
  if $ESPTOOL --chip esp32s3 -p "$PORT" -b 460800 --before no-reset --after no-reset chip-id >/tmp/touch28b_chipid.txt 2>&1; then
    echo "Connected."
    break
  fi
  sleep 1
done
if ! grep -q "Chip type" /tmp/touch28b_chipid.txt 2>/dev/null; then
  echo "Timeout: chip not in download mode." >&2
  cat /tmp/touch28b_chipid.txt >&2 || true
  exit 1
fi

BOOT_APP0="${HOME}/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
$ESPTOOL --chip esp32s3 -p "$PORT" -b 460800 --before no-reset --after watchdog-reset \
  write-flash --flash-mode dio --flash-size 16MB --flash-freq 80m \
  0x0 "$FW/bootloader.bin" \
  0x8000 "$FW/partitions.bin" \
  0xe000 "$BOOT_APP0" \
  0x10000 "$FW/firmware.bin"

echo "Flash done. Press RESET if the app stays in download mode."
