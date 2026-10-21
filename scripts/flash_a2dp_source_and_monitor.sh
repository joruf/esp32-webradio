#!/usr/bin/env bash
# Flash ESP32-WROOM A2DP Source and open serial monitor.
# Expects WROOM on /dev/ttyUSB0 (CP210x/CH340). Does not touch S3 (/dev/ttyACM0).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PORT="${1:-/dev/ttyUSB0}"
ENV=esp32_a2dp_source

if [[ ! -e "$PORT" ]]; then
  echo "ERROR: $PORT not found. Plug the WROOM USB-serial adapter."
  exit 1
fi

# Keep Linux discoverable as A2DP Sink (Bluetooth Classic / BR/EDR, not BLE).
# ESP32 A2DP Source will not show up in BLE scanners.
bluetoothctl power on >/dev/null
bluetoothctl pairable on >/dev/null
bluetoothctl discoverable on >/dev/null
bluetoothctl discoverable-timeout 0 >/dev/null || true
echo "Linux sink Alias: $(bluetoothctl show | awk -F': ' '/^\tAlias:/{print $2; exit}')"
echo "Linux sink MAC:   $(bluetoothctl show | awk '/Controller/{print $2; exit}')"
echo "Firmware env: $ENV  port: $PORT"
echo "Expect serial: discovered sink '<Alias>' -> CONNECTED -> beat: bt=up"
echo "Audio on Linux: PipeWire sink-input from bluez_input.* (pactl list short sink-inputs)"
echo "Tip: if Blueman steals the agent, close blueman-manager during first pair."

fuser -k "$PORT" 2>/dev/null || true
sleep 1
cd "$ROOT"
/home/joruf/.platformio/penv/bin/pio run -e "$ENV" -t upload --upload-port "$PORT"
exec /home/joruf/.platformio/penv/bin/pio device monitor -e "$ENV" --port "$PORT" --baud 115200
