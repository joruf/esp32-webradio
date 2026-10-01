# Fritzing wiring diagrams (ESP32-Webradio)

Generated sketches for both hardware profiles from `src/config/hardware_profile.h`.

## Files

| File | Profile |
|------|---------|
| `circuit_diagram_wroom_tft.fzz` (+ `.png`, breadboard/schematic/pcb `.svg`) | ESP32-WROOM + VS1053 + encoder + TFT + SPI SD |
| `circuit_diagram_cam_headless.fzz` (+ `.png`, breadboard/schematic/pcb `.svg`) | ESP32-CAM (no camera) + VS1053 + encoder + onboard SD_MMC |
| `circuit_diagram.fzz` / `circuit_diagram.png` | Alias of the WROOM diagram |

## Regenerate

```bash
cd fritzing
python3 build_circuit_diagram.py          # both (+ SVG/PNG export if tools available)
python3 build_circuit_diagram.py wroom_tft
python3 build_circuit_diagram.py cam_headless
python3 build_circuit_diagram.py all --skip-export   # .fzz only
```

Wires are generated as orthogonal multi-segment buses (horizontal/vertical only) with
fixed lane pitch. Schematic PNG is the primary preview; breadboard PNG is also
exported. Custom part breadboard SVGs must include a group with `id="breadboard"`
(lowercase) or Fritzing omits the part from breadboard export.


## Open / export images

1. Install Fritzing (Flatpak: `org.fritzing.Fritzing`).
2. Open the `.fzz` file in Fritzing.
3. Export PNG via File > Export > as Image if needed.

Headless SVG export (used by `build_circuit_diagram.py --svg-export`):

```bash
flatpak run org.fritzing.Fritzing -svg /path/to/folder_with_fzz
```

Generated breadboard/schematic/pcb SVGs are written next to the `.fzz` files when export succeeds.

## CAM notes

- Custom part: `parts/part.ESP32-CAM-AIThinker.fzp` (minimal; not a full AI-Thinker footprint).
- GPIOs 5/18/19/23/25/26/27/32 are camera-interface pads when a camera is fitted; use without camera.
- SD_MMC 1-bit is onboard: CLK=GPIO14, CMD=GPIO15, D0=GPIO2. No external SD CS wire.
- No TFT on the CAM diagram.
- Avoid GPIOs 12/13/16/17 for extras (CAM / flash strapping constraints).
