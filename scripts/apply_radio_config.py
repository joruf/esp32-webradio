#!/usr/bin/env python3
"""Apply a saved radio config snapshot to a target ESP32-Webradio.

Snapshots live in config/snapshots/<name>/radio_config.json.
WiFi passwords are placeholders filled from the environment (never stored).

Examples:
  WIFI_PASSWORD='secret' ./scripts/apply_radio_config.py --target 192.168.178.40
  WIFI_PASSWORD='secret' ./scripts/apply_radio_config.py --target 192.168.178.40 \\
      --snapshot joruf-home --skip-wifi
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SNAPSHOTS = ROOT / "config" / "snapshots"

PLACEHOLDER_WIFI = "{{WIFI_PASSWORD}}"
PLACEHOLDER_OTA_FAKE = "{{OTA_FAKE_WIFI_PASSWORD}}"


def http_get(url: str, timeout: float = 20) -> bytes:
    with urllib.request.urlopen(url, timeout=timeout) as resp:
        return resp.read()


def http_post_json(url: str, payload: dict, timeout: float = 60) -> str:
    raw = json.dumps(payload, separators=(",", ":")).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=raw,
        method="POST",
        headers={"Content-Type": "application/json; charset=UTF-8"},
    )
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return resp.read().decode("utf-8", errors="replace")


def load_snapshot(name: str) -> tuple[Path, dict]:
    snap_dir = SNAPSHOTS / name
    cfg_path = snap_dir / "radio_config.json"
    if not cfg_path.is_file():
        raise SystemExit(f"Snapshot not found: {cfg_path}")
    cfg = json.loads(cfg_path.read_text(encoding="utf-8"))
    cfg.pop("wifi_signals", None)
    return snap_dir, cfg


def resolve_wifi(cfg: dict, skip_wifi: bool) -> dict:
    if skip_wifi:
        cfg = dict(cfg)
        cfg.pop("wifi", None)
        return cfg

    wifi_password = os.environ.get("WIFI_PASSWORD", "").strip()
    ota_fake_password = os.environ.get("OTA_FAKE_WIFI_PASSWORD", "").strip()
    resolved = []
    for entry in cfg.get("wifi") or []:
        if not isinstance(entry, dict):
            continue
        ssid = (entry.get("ssid") or "").strip()
        password = entry.get("password") or ""
        if password == PLACEHOLDER_WIFI:
            if not wifi_password:
                raise SystemExit(
                    "WIFI_PASSWORD is required to apply WiFi (or pass --skip-wifi)."
                )
            password = wifi_password
        elif password == PLACEHOLDER_OTA_FAKE:
            if not ota_fake_password:
                # Optional test network: drop when secret is unset.
                continue
            password = ota_fake_password
        elif password in ("", "********"):
            raise SystemExit(
                f"WiFi entry '{ssid}' still has a masked/empty password. "
                "Use placeholders in the snapshot and set WIFI_PASSWORD."
            )
        resolved.append({"ssid": ssid, "password": password})

    out = dict(cfg)
    out["wifi"] = resolved
    return out


def apply_live_runtime(base: str, cfg: dict) -> None:
    volume = int(cfg.get("volume", 70))
    preset = int(cfg.get("preset", 0))
    tone = cfg.get("tone") or {}
    http_get(f"{base}/?volume={volume}")
    for key, cmd in (("ha", "toneha"), ("hf", "tonehf"), ("la", "tonela"), ("lf", "tonelf")):
        http_get(f"{base}/?{cmd}={int(tone.get(key, 0))}")
    http_get(f"{base}/?preset={preset}")


def summarize(cfg: dict) -> str:
    wifi = [w.get("ssid") for w in (cfg.get("wifi") or []) if isinstance(w, dict)]
    return (
        f"presets={len(cfg.get('presets') or [])} "
        f"alarms={len(cfg.get('alarms') or [])} "
        f"volume={cfg.get('volume')} preset={cfg.get('preset')} "
        f"wifi={wifi}"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Apply ESP32-Webradio config snapshot")
    parser.add_argument("--target", required=True, help="Radio IP or host, e.g. 192.168.178.40")
    parser.add_argument(
        "--snapshot",
        default="joruf-home",
        help="Snapshot name under config/snapshots/ (default: joruf-home)",
    )
    parser.add_argument(
        "--skip-wifi",
        action="store_true",
        help="Do not write WiFi networks (keep credentials already on the device)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be sent, do not contact the radio",
    )
    args = parser.parse_args()

    snap_dir, cfg = load_snapshot(args.snapshot)
    payload = resolve_wifi(cfg, skip_wifi=args.skip_wifi)
    base = f"http://{args.target.rstrip('/')}"
    if "://" in args.target:
        base = args.target.rstrip("/")

    print(f"Snapshot: {snap_dir}")
    print(f"Target:   {base}")
    print(f"Payload:  {summarize(payload)}")

    if args.dry_run:
        redacted = json.loads(json.dumps(payload))
        for entry in redacted.get("wifi") or []:
            if isinstance(entry, dict) and entry.get("password"):
                entry["password"] = "***"
        print(json.dumps(redacted, indent=2, ensure_ascii=False))
        return 0

    try:
        live = json.loads(http_get(f"{base}/?getJSON").decode())
        print(f"Device:   {live.get('firmware')} vol={live.get('volume')}")
    except Exception as exc:
        raise SystemExit(f"Target not reachable: {exc}") from exc

    try:
        print("saveconfig:", http_post_json(f"{base}/?saveconfig", payload).strip()[:200])
        time.sleep(0.5)
        alarms = {
            "version": int(payload.get("version") or 3),
            "alarms": payload.get("alarms") or [],
        }
        print("savealarms:", http_post_json(f"{base}/?savealarms", alarms).strip()[:200])
        if payload.get("features"):
            time.sleep(0.3)
            print(
                "savefeatures:",
                http_post_json(f"{base}/?savefeatures", {"features": payload["features"]}).strip()[
                    :200
                ],
            )
        time.sleep(0.3)
        apply_live_runtime(base, payload)
        time.sleep(1.0)
        dst = json.loads(http_get(f"{base}/?getconfig").decode())
        src_urls = [p.get("url") for p in (payload.get("presets") or [])]
        dst_urls = [p.get("url") for p in (dst.get("presets") or [])]
        src_alarms = [
            (a.get("id"), a.get("start"), a.get("stop"), tuple(a.get("days") or []))
            for a in (payload.get("alarms") or [])
        ]
        dst_alarms = [
            (a.get("id"), a.get("start"), a.get("stop"), tuple(a.get("days") or []))
            for a in (dst.get("alarms") or [])
        ]
        print(
            "verify:",
            f"presets={'OK' if src_urls == dst_urls else 'DIFF'}",
            f"alarms={'OK' if src_alarms == dst_alarms else 'DIFF'}",
            f"volume={dst.get('volume')}",
            f"wifi={[w.get('ssid') for w in (dst.get('wifi') or [])]}",
        )
        if src_urls != dst_urls or src_alarms != dst_alarms:
            return 2
    except urllib.error.URLError as exc:
        raise SystemExit(f"Apply failed: {exc}") from exc

    print("Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
