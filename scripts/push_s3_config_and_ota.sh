#!/usr/bin/env bash
# Push WiFi + volume to ESP32-S3 Webradio (STA), OTA esp32s3_touch28b, verify after reboot.
# Usage: ./scripts/push_s3_config_and_ota.sh [IP]
# WiFi NAME/PASSWORD from ~/Dokumente/GitHub/credentials.env.age (wifi{} block).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PIO="${HOME}/.platformio/penv/bin/pio"
CRED="${HOME}/Dokumente/GitHub/credentials.env.age"
AGE_KEY="${HOME}/.config/age/key.txt"
TARGET_MAC="fc:01:2c:d9:4a:60"
FORBIDDEN_IP="192.168.178.33"

resolve_ip() {
  if [[ -n "${1:-}" ]]; then
    echo "$1"
    return
  fi
  for host in radio-2.local radio.local; do
    local ip
    ip="$(avahi-resolve -n "$host" 2>/dev/null | awk '{print $2}' || true)"
    if [[ -n "$ip" && "$ip" != "$FORBIDDEN_IP" ]]; then
      echo "$ip"
      return
    fi
  done
  echo "192.168.178.29"
}

IP="$(resolve_ip "${1:-}")"
if [[ "$IP" == "$FORBIDDEN_IP" ]]; then
  echo "Refusing target $FORBIDDEN_IP" >&2
  exit 1
fi

echo "Waiting for HTTP on $IP (MAC $TARGET_MAC)..."
deadline=$((SECONDS + 120))
while (( SECONDS < deadline )); do
  if curl -sf -m 3 "http://${IP}/?getconfig" >/tmp/s3_getconfig.json 2>/dev/null; then
    mac="$(python3 -c "import json; d=json.load(open('/tmp/s3_getconfig.json')); print(d.get('wifi_mac',''))" 2>/dev/null || true)"
    if [[ -n "$mac" && "$mac" != "${TARGET_MAC//:/:}" && "$mac" != "$(echo "$TARGET_MAC" | tr '[:lower:]' '[:upper:]')" ]]; then
      echo "Warning: wifi_mac on device is $mac (expected $TARGET_MAC)" >&2
    fi
    break
  fi
  sleep 2
done
if ! [[ -s /tmp/s3_getconfig.json ]]; then
  echo "Device not reachable at http://${IP}/" >&2
  exit 1
fi

echo "Building esp32s3_touch28b..."
cd "$ROOT"
"$PIO" run -e esp32s3_touch28b

echo "Pushing WiFi + volume 80 via saveconfig..."
export S3_IP="$IP"
python3 <<'PY'
import json, subprocess, os, re, sys, urllib.request

cred = os.path.expanduser("~/Dokumente/GitHub/credentials.env.age")
key = os.path.expanduser("~/.config/age/key.txt")
text = subprocess.run(["age", "-d", "-i", key, cred], capture_output=True, text=True).stdout
m = re.search(r"wifi\s*\{([^}]*)\}", text, re.S | re.I)
if not m:
    sys.exit("wifi{} block missing in credentials")
fields = {}
for ln in m.group(1).splitlines():
    ln = ln.strip()
    if "=" in ln:
        k, v = ln.split("=", 1)
        fields[k.strip().upper()] = v.strip()
ssid = fields.get("NAME") or fields.get("SSID")
password = fields.get("PASSWORD") or fields.get("PASS")
if not ssid or not password:
    sys.exit("wifi NAME/PASSWORD missing in credentials")

ip = os.environ["S3_IP"]
patch = {"volume": 80, "wifi": [{"ssid": ssid, "password": password}]}
body = json.dumps(patch).encode()
req = urllib.request.Request(
    f"http://{ip}/?saveconfig",
    data=body,
    method="POST",
    headers={"Content-Type": "application/json"},
)
with urllib.request.urlopen(req, timeout=15) as resp:
    print(resp.read(200).decode(errors="replace"))

# Reboot so STA uses new WiFi
urllib.request.urlopen(f"http://{ip}/?reset", timeout=10)
print("Reset sent; waiting for STA...")
PY

sleep 15
IP="$(resolve_ip "$IP")"
export S3_IP="$IP"
deadline=$((SECONDS + 90))
while (( SECONDS < deadline )); do
  if curl -sf -m 3 "http://${IP}/?getconfig" | python3 -c "import sys,json; d=json.load(sys.stdin); print('volume',d.get('volume'), 'wifi', len(d.get('wifi',[])))"; then
    echo "Post-reboot OK at http://${IP}/"
    break
  fi
  sleep 3
done

echo "OTA upload to ${IP}..."
"$PIO" run -e esp32s3_touch28b -t upload --upload-port "$IP" --upload-protocol espota

echo "Done. Verify: curl -s http://${IP}/?getdiagnostics | head"
