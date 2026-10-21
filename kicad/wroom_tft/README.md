# KiCad: ESP32-Webradio wroom_tft

Verdrahtungsmodell (Schaltplan) für:

- ESP32-WROOM
- TFT ST7735 1.8" 128x160 SPI
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

## Bild

- `ESP32-Webradio-wroom_tft-wiring.png`
- Kopie: `wiring/wroom_tft_wiring.png`

```bash
python3 export_wiring_image.py
```
