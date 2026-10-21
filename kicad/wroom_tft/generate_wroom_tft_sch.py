#!/usr/bin/env python3
"""Generate KiCad 8 schematic for ESP32-Webradio wroom_tft wiring."""

from __future__ import annotations

import uuid
from pathlib import Path

OUT = Path(__file__).resolve().parent


def uid() -> str:
    return str(uuid.uuid4())


def pin(num: str, name: str, x: float, y: float, rot: int, length: float = 2.54) -> str:
    # pin orientation: 0 right, 90 up, 180 left, 270 down
    return f"""        (pin passive line
          (at {x:.2f} {y:.2f} {rot})
          (length {length:.2f})
          (name "{name}" (effects (font (size 1.27 1.27))))
          (number "{num}" (effects (font (size 1.27 1.27))))
        )"""


def box_symbol(
    lib_id: str,
    ref_prefix: str,
    value: str,
    width: float,
    height: float,
    pins_left: list[tuple[str, str]],
    pins_right: list[tuple[str, str]],
) -> str:
    """Build an embedded lib_symbol with left/right pin columns."""
    hx, hy = width / 2, height / 2
    body = f"""      (rectangle
        (start {-hx:.2f} {-hy:.2f})
        (end {hx:.2f} {hy:.2f})
        (stroke (width 0.254) (type default))
        (fill (type background))
      )
      (text "{value}"
        (exclude_from_sim no)
        (at 0 {hy + 2.54:.2f} 0)
        (effects (font (size 1.27 1.27)) (justify bottom))
      )"""

    pin_blocks: list[str] = []
    # left pins: number from top
    if pins_left:
        step = height / (len(pins_left) + 1)
        for i, (num, name) in enumerate(pins_left):
            y = hy - step * (i + 1)
            pin_blocks.append(pin(num, name, -hx, y, 180))
    if pins_right:
        step = height / (len(pins_right) + 1)
        for i, (num, name) in enumerate(pins_right):
            y = hy - step * (i + 1)
            pin_blocks.append(pin(num, name, hx, y, 0))

    return f"""    (symbol "{lib_id}"
      (pin_names (offset 1.016))
      (exclude_from_sim no)
      (in_bom yes)
      (on_board yes)
      (property "Reference" "{ref_prefix}"
        (at 0 {hy + 5.08:.2f} 0)
        (effects (font (size 1.27 1.27)))
      )
      (property "Value" "{value}"
        (at 0 {-hy - 2.54:.2f} 0)
        (effects (font (size 1.27 1.27)))
      )
      (property "Footprint" ""
        (at 0 0 0)
        (effects (font (size 1.27 1.27)) hide)
      )
      (property "Datasheet" ""
        (at 0 0 0)
        (effects (font (size 1.27 1.27)) hide)
      )
      (symbol "{lib_id}_0_1"
{body}
{chr(10).join(pin_blocks)}
      )
    )"""


def symbol_instance(
    lib_id: str,
    ref: str,
    value: str,
    x: float,
    y: float,
    uuid_str: str,
) -> str:
    return f"""  (symbol
    (lib_id "{lib_id}")
    (at {x:.2f} {y:.2f} 0)
    (unit 1)
    (exclude_from_sim no)
    (in_bom yes)
    (on_board yes)
    (dnp no)
    (uuid "{uuid_str}")
    (property "Reference" "{ref}"
      (at {x:.2f} {y + 20:.2f} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Value" "{value}"
      (at {x:.2f} {y - 22:.2f} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Footprint" ""
      (at {x:.2f} {y:.2f} 0)
      (effects (font (size 1.27 1.27)) hide)
    )
    (property "Datasheet" ""
      (at {x:.2f} {y:.2f} 0)
      (effects (font (size 1.27 1.27)) hide)
    )
  )"""


def label(name: str, x: float, y: float, rot: int = 0) -> str:
    return f"""  (label "{name}"
    (at {x:.2f} {y:.2f} {rot})
    (effects (font (size 1.27 1.27)) (justify left bottom))
    (uuid "{uid()}")
  )"""


def wire(x1: float, y1: float, x2: float, y2: float) -> str:
    return f"""  (wire
    (pts
      (xy {x1:.2f} {y1:.2f})
      (xy {x2:.2f} {y2:.2f})
    )
    (stroke (width 0) (type default))
    (uuid "{uid()}")
  )"""


def text_note(txt: str, x: float, y: float, size: float = 2.0) -> str:
    return f"""  (text "{txt}"
    (exclude_from_sim no)
    (at {x:.2f} {y:.2f} 0)
    (effects (font (size {size:.2f} {size:.2f})) (justify left top))
    (uuid "{uid()}")
  )"""


def main() -> None:
    # Pin lists (num, name) — order top to bottom on each side
    esp_left = [
        ("3V3", "3V3"),
        ("GND", "GND"),
        ("2", "GPIO2"),
        ("4", "GPIO4"),
        ("5", "GPIO5"),
        ("15", "GPIO15"),
        ("18", "GPIO18"),
        ("19", "GPIO19"),
        ("21", "GPIO21"),
        ("23", "GPIO23"),
        ("25", "GPIO25"),
        ("26", "GPIO26"),
        ("27", "GPIO27"),
        ("32", "GPIO32"),
        ("33", "GPIO33"),
        ("35", "GPIO35"),
    ]
    esp_right: list[tuple[str, str]] = []

    vs_left = [
        ("3V3A", "3V3"),
        ("3V3B", "3V3"),
        ("GNDA", "GND"),
        ("GNDB", "GND"),
        ("RST", "RST"),
        ("CS", "CS"),
        ("DCS", "DCS"),
        ("DREQ", "DREQ"),
        ("SCK", "SCK"),
        ("MISO", "MISO"),
        ("MOSI", "MOSI"),
        ("SDCS", "SDCS"),
    ]

    tft_left = [
        ("VCC", "VCC"),
        ("GND", "GND"),
        ("RST", "RST"),
        ("CS", "CS"),
        ("DC", "DC"),
        ("SCK", "SCL/SCK"),
        ("MOSI", "SDA/MOSI"),
        ("BL", "BL"),
    ]

    enc_left = [
        ("GND", "GND"),
        ("PLUS", "+"),
        ("SW", "SW"),
        ("DT", "DT"),
        ("CLK", "CLK"),
    ]

    lib_symbols = "\n".join(
        [
            box_symbol("Wiring:ESP32_WROOM", "U", "ESP32-WROOM", 35.56, 60.96, esp_left, esp_right),
            box_symbol("Wiring:VS1053", "U", "VS1053 Breakout", 30.48, 45.72, vs_left, []),
            box_symbol("Wiring:TFT_ST7735", "U", "TFT ST7735 1.8in", 30.48, 35.56, tft_left, []),
            box_symbol("Wiring:KY040", "SW", "KY-040 Encoder", 25.4, 25.4, enc_left, []),
        ]
    )

    # Component positions
    esp_x, esp_y = 50.8, 100.0
    vs_x, vs_y = 140.0, 55.0
    tft_x, tft_y = 140.0, 115.0
    enc_x, enc_y = 140.0, 165.0

    uu_esp, uu_vs, uu_tft, uu_enc = uid(), uid(), uid(), uid()

    instances = "\n".join(
        [
            symbol_instance("Wiring:ESP32_WROOM", "U1", "ESP32-WROOM", esp_x, esp_y, uu_esp),
            symbol_instance("Wiring:VS1053", "U2", "VS1053", vs_x, vs_y, uu_vs),
            symbol_instance("Wiring:TFT_ST7735", "U3", "ST7735 128x160", tft_x, tft_y, uu_tft),
            symbol_instance("Wiring:KY040", "SW1", "KY-040", enc_x, enc_y, uu_enc),
        ]
    )

    # Net labels placed at ESP pin exits (left side of ESP box → labels to the left)
    # ESP box width 35.56 → half 17.78; pin length 2.54 → label at esp_x - 17.78 - 2.54
    esp_pin_x = esp_x - 17.78 - 2.54
    esp_h = 60.96
    esp_hy = esp_h / 2
    esp_step = esp_h / (len(esp_left) + 1)

    def esp_pin_y(i: int) -> float:
        return esp_y + esp_hy - esp_step * (i + 1)

    # Index map for esp_left
    esp_idx = {name: i for i, (_, name) in enumerate(esp_left)}

    nets = [
        ("+3V3", "3V3"),
        ("GND", "GND"),
        ("TFT_DC", "GPIO2"),
        ("VS_DREQ", "GPIO4"),
        ("VS_CS", "GPIO5"),
        ("TFT_CS", "GPIO15"),
        ("SPI_SCK", "GPIO18"),
        ("SPI_MISO", "GPIO19"),
        ("SD_CS", "GPIO21"),
        ("SPI_MOSI", "GPIO23"),
        ("ENC_CLK", "GPIO25"),
        ("ENC_DT", "GPIO26"),
        ("ENC_SW", "GPIO27"),
        ("VS_DCS", "GPIO32"),
        ("TFT_BL", "GPIO33"),
        ("IR_OPT", "GPIO35"),
    ]

    labels: list[str] = []
    for net, gpio in nets:
        i = esp_idx[gpio]
        y = esp_pin_y(i)
        # label just left of ESP pin
        labels.append(label(net, esp_pin_x - 12.7, y, 0))
        labels.append(wire(esp_pin_x - 12.7, y, esp_pin_x, y))

    # VS1053 pin labels on its left
    vs_pin_x = vs_x - 15.24 - 2.54
    vs_h = 45.72
    vs_hy = vs_h / 2
    vs_step = vs_h / (len(vs_left) + 1)
    vs_map = {
        "3V3A": "+3V3",
        "3V3B": "+3V3",
        "GNDA": "GND",
        "GNDB": "GND",
        "RST": "+3V3",
        "CS": "VS_CS",
        "DCS": "VS_DCS",
        "DREQ": "VS_DREQ",
        "SCK": "SPI_SCK",
        "MISO": "SPI_MISO",
        "MOSI": "SPI_MOSI",
        "SDCS": "SD_CS",
    }
    for i, (num, _name) in enumerate(vs_left):
        y = vs_y + vs_hy - vs_step * (i + 1)
        net = vs_map[num]
        labels.append(label(net, vs_pin_x - 15.24, y, 0))
        labels.append(wire(vs_pin_x - 15.24, y, vs_pin_x, y))

    # TFT
    tft_pin_x = tft_x - 15.24 - 2.54
    tft_h = 35.56
    tft_hy = tft_h / 2
    tft_step = tft_h / (len(tft_left) + 1)
    tft_map = {
        "VCC": "+3V3",
        "GND": "GND",
        "RST": "+3V3",
        "CS": "TFT_CS",
        "DC": "TFT_DC",
        "SCK": "SPI_SCK",
        "MOSI": "SPI_MOSI",
        "BL": "TFT_BL",
    }
    for i, (num, _name) in enumerate(tft_left):
        y = tft_y + tft_hy - tft_step * (i + 1)
        net = tft_map[num]
        labels.append(label(net, tft_pin_x - 15.24, y, 0))
        labels.append(wire(tft_pin_x - 15.24, y, tft_pin_x, y))

    # Encoder (+ to 3V3 typical for KY-040 modules)
    enc_pin_x = enc_x - 12.7 - 2.54
    enc_h = 25.4
    enc_hy = enc_h / 2
    enc_step = enc_h / (len(enc_left) + 1)
    enc_map = {
        "GND": "GND",
        "PLUS": "+3V3",
        "SW": "ENC_SW",
        "DT": "ENC_DT",
        "CLK": "ENC_CLK",
    }
    for i, (num, _name) in enumerate(enc_left):
        y = enc_y + enc_hy - enc_step * (i + 1)
        net = enc_map[num]
        labels.append(label(net, enc_pin_x - 15.24, y, 0))
        labels.append(wire(enc_pin_x - 15.24, y, enc_pin_x, y))

    notes = "\n".join(
        [
            text_note(
                "ESP32-Webradio wroom_tft – Verdrahtungsmodell\\n"
                "Quelle: wiring/pin_connections.txt + hardware_profile.h\\n"
                "Gemeinsamer SPI: SCK=18 MOSI=23 MISO=19\\n"
                "TFT RST und VS1053 RST fest an +3V3 (Firmware: tft rst=-1)\\n"
                "TFT MISO optional (viele 8-Pin-Module ohne MISO)\\n"
                "GPIO35 = IR optional",
                10.0,
                10.0,
                1.5,
            ),
            text_note(
                "Netzliste (Kurz)\\n"
                "+3V3: ESP, VS1053 3V3+RST, TFT VCC+RST, Encoder +\\n"
                "GND: ESP, VS1053, TFT, Encoder\\n"
                "SPI_SCK/MOSI/MISO: ESP↔VS1053↔TFT\\n"
                "VS: CS=5 DCS=32 DREQ=4 SDCS=21\\n"
                "TFT: CS=15 DC=2 BL=33\\n"
                "ENC: CLK=25 DT=26 SW=27",
                10.0,
                200.0,
                1.5,
            ),
        ]
    )

    sch = f"""(kicad_sch
  (version 20231120)
  (generator "eeschema")
  (generator_version "8.0")
  (uuid "{uid()}")
  (paper "A3")
  (title_block
    (title "ESP32-Webradio wroom_tft Wiring")
    (date "2026-10-14")
    (rev "1")
    (company "loresoft")
    (comment 1 "ESP32-WROOM + ST7735 + VS1053 + KY-040")
    (comment 2 "Generated from project pin map")
  )
  (lib_symbols
{lib_symbols}
  )
{instances}
{chr(10).join(labels)}
{notes}
  (sheet_instances
    (path "/" (page "1"))
  )
)
"""

    sch_path = OUT / "ESP32-Webradio-wroom_tft.kicad_sch"
    sch_path.write_text(sch, encoding="utf-8")
    print("wrote", sch_path)

    pro_path = OUT / "ESP32-Webradio-wroom_tft.kicad_pro"
    pro_path.write_text(
        """{
  "meta": {
    "filename": "ESP32-Webradio-wroom_tft.kicad_pro",
    "version": 1
  },
  "sheets": [
    [
      "root",
      "Root"
    ]
  ],
  "text_variables": {},
  "libraries": {
    "pinned_symbol_libs": [],
    "pinned_footprint_libs": []
  },
  "schematic": {
    "meta": {
      "version": 1
    },
    "drawing": {
      "default_line_thickness": 6.0,
      "default_text_size": 50.0
    }
  },
  "board": {
    "design_settings": {
      "defaults": {},
      "rules": {}
    }
  },
  "net_settings": {
    "classes": [
      {
        "name": "Default",
        "wire_width": 6,
        "bus_width": 12,
        "clearance": 0.2,
        "track_width": 0.25,
        "via_diameter": 0.8,
        "via_drill": 0.4
      }
    ],
    "meta": {
      "version": 3
    }
  }
}
""",
        encoding="utf-8",
    )
    print("wrote", pro_path)

    readme = OUT / "README.md"
    readme.write_text(
        """# KiCad: ESP32-Webradio wroom_tft

Verdrahtungsmodell (Schaltplan) für:

- ESP32-WROOM
- TFT ST7735 1.8\" 128x160 SPI
- VS1053 Breakout
- KY-040 Rotary Encoder

## Öffnen

```bash
kicad ESP32-Webradio-wroom_tft.kicad_pro
```

Oder nur den Schaltplan: `ESP32-Webradio-wroom_tft.kicad_sch`

## Netz-Zuordnung

| Netz | ESP32 | Ziel |
|------|-------|------|
| +3V3 | 3V3 | VS1053 3V3+RST, TFT VCC+RST, Encoder + |
| GND | GND | VS1053, TFT, Encoder |
| SPI_SCK | GPIO18 | VS1053 SCK, TFT SCL |
| SPI_MOSI | GPIO23 | VS1053 MOSI, TFT SDA |
| SPI_MISO | GPIO19 | VS1053 MISO (TFT optional) |
| VS_CS | GPIO5 | VS1053 CS |
| VS_DCS | GPIO32 | VS1053 DCS |
| VS_DREQ | GPIO4 | VS1053 DREQ |
| SD_CS | GPIO21 | VS1053 SDCS |
| TFT_CS | GPIO15 | TFT CS |
| TFT_DC | GPIO2 | TFT DC/A0 |
| TFT_BL | GPIO33 | TFT BL/LED |
| ENC_CLK | GPIO25 | Encoder CLK |
| ENC_DT | GPIO26 | Encoder DT |
| ENC_SW | GPIO27 | Encoder SW |
| IR_OPT | GPIO35 | IR optional |

Neu erzeugen:

```bash
python3 generate_wroom_tft_sch.py
```
""",
        encoding="utf-8",
    )
    print("wrote", readme)


if __name__ == "__main__":
    main()
