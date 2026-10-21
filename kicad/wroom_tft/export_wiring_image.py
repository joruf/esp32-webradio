#!/usr/bin/env python3
"""Render ESP32-Webradio wroom_tft wiring as PNG + SVG."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parent
W = 1600
PIN_STEP = 30
TITLE_GAP = 50


def font(size: int) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    for path in (
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    ):
        p = Path(path)
        if p.exists():
            return ImageFont.truetype(str(p), size)
    return ImageFont.load_default()


def rounded_box(draw: ImageDraw.ImageDraw, xy, fill, outline, radius=16, width=3):
    draw.rounded_rectangle(xy, radius=radius, fill=fill, outline=outline, width=width)


def box_bottom_for(y_start: int, n_pins: int) -> int:
    """Bottom edge so last pin text + dot stay inside the box."""
    return y_start + (n_pins - 1) * PIN_STEP + 18 + 48


def main() -> None:
    esp_pins = [
        ("3V3", "+3V3"),
        ("GND", "GND"),
        ("EN", "VS1053 RST"),
        ("GPIO2", "TFT DC"),
        ("GPIO4", "VS DREQ"),
        ("GPIO5", "VS CS"),
        ("GPIO15", "TFT CS"),
        ("GPIO18", "SPI SCK"),
        ("GPIO19", "SPI MISO"),
        ("GPIO21", "SD CS → VS SDCS"),
        ("GPIO23", "SPI MOSI"),
        ("GPIO25", "ENC CLK"),
        ("GPIO26", "ENC DT"),
        ("GPIO27", "ENC SW"),
        ("GPIO32", "VS DCS"),
        ("GPIO33", "TFT BL"),
        ("GPIO35", "IR (optional)"),
    ]
    vs_pins = [
        ("3V3 (×2)", "+3V3"),
        ("GND (×2)", "GND"),
        ("RST", "ESP EN"),
        ("CS", "GPIO5"),
        ("DCS", "GPIO32"),
        ("DREQ", "GPIO4"),
        ("SCK", "GPIO18"),
        ("MISO", "GPIO19"),
        ("MOSI", "GPIO23"),
        ("SDCS", "GPIO21"),
    ]
    tft_pins = [
        ("VCC", "+3V3"),
        ("GND", "GND"),
        ("RES / RST", "+3V3 (fest)"),
        ("CS", "GPIO15"),
        ("DC / A0", "GPIO2"),
        ("SCL / SCK", "GPIO18"),
        ("SDA / MOSI", "GPIO23"),
        ("BL / LED", "GPIO33"),
    ]
    enc_pins = [
        ("GND", "GND"),
        ("+", "+3V3"),
        ("CLK", "GPIO25"),
        ("DT", "GPIO26"),
        ("SW", "GPIO27"),
    ]

    esp_y0 = 115 + TITLE_GAP
    vs_y0 = 115 + TITLE_GAP
    esp_bottom = box_bottom_for(esp_y0, len(esp_pins))
    vs_bottom = box_bottom_for(vs_y0, len(vs_pins))
    tft_y0 = vs_bottom + 24 + TITLE_GAP
    tft_bottom = box_bottom_for(tft_y0, len(tft_pins))
    enc_y0 = tft_bottom + 24 + TITLE_GAP
    enc_bottom = box_bottom_for(enc_y0, len(enc_pins))

    footer_y = max(esp_bottom, enc_bottom) + 30
    footer_h = 260
    H = footer_y + footer_h + 20

    img = Image.new("RGB", (W, H), "#f4f6f8")
    draw = ImageDraw.Draw(img)
    f_title = font(32)
    f_h = font(22)
    f_b = font(18)
    f_s = font(15)
    f_tiny = font(13)

    draw.text((40, 24), "ESP32-Webradio  ·  wroom_tft Verdrahtung", fill="#1a2332", font=f_title)
    draw.text(
        (40, 68),
        "ESP32-WROOM + ST7735 1.8\" + VS1053 + KY-040   |   gemeinsamer SPI-Bus",
        fill="#4a5568",
        font=f_b,
    )

    boxes = {
        "esp": (60, 115, 420, esp_bottom, "ESP32-WROOM", "#dbeafe"),
        "vs": (980, 115, 1520, vs_bottom, "VS1053 Breakout", "#dcfce7"),
        "tft": (980, tft_y0 - TITLE_GAP, 1520, tft_bottom, "TFT ST7735 128×160", "#fef3c7"),
        "enc": (980, enc_y0 - TITLE_GAP, 1520, enc_bottom, "KY-040 Encoder", "#ede9fe"),
    }
    for _key, (x0, y0, x1, y1, title, fill) in boxes.items():
        rounded_box(draw, (x0, y0, x1, y1), fill=fill, outline="#334155")
        draw.text((x0 + 20, y0 + 12), title, fill="#0f172a", font=f_h)

    esp_anchor = {}
    y = esp_y0
    for gpio, role in esp_pins:
        draw.text((80, y), gpio, fill="#1e293b", font=f_s)
        draw.text((200, y), role, fill="#475569", font=f_s)
        ay = y + 8
        esp_anchor[gpio] = (420, ay)
        draw.ellipse((408, ay - 5, 418, ay + 5), fill="#2563eb")
        y += PIN_STEP

    def list_pins(x0: int, y_start: int, pins: list[tuple[str, str]], color: str):
        anchors = {}
        yy = y_start
        for name, net in pins:
            draw.text((x0 + 28, yy), name, fill="#1e293b", font=f_s)
            draw.text((x0 + 210, yy), net, fill="#64748b", font=f_tiny)
            ay = yy + 8
            anchors[name] = (x0, ay)
            draw.ellipse((x0 + 6, ay - 5, x0 + 16, ay + 5), fill=color)
            yy += PIN_STEP
        return anchors

    vs_a = list_pins(980, vs_y0, vs_pins, "#16a34a")
    tft_a = list_pins(980, tft_y0, tft_pins, "#ca8a04")
    enc_a = list_pins(980, enc_y0, enc_pins, "#7c3aed")

    routes = [
        ("EN", vs_a["RST"], "#ef4444"),
        ("3V3", tft_a["RES / RST"], "#f97316"),
        ("GPIO5", vs_a["CS"], "#16a34a"),
        ("GPIO32", vs_a["DCS"], "#16a34a"),
        ("GPIO4", vs_a["DREQ"], "#16a34a"),
        ("GPIO18", vs_a["SCK"], "#0ea5e9"),
        ("GPIO19", vs_a["MISO"], "#0ea5e9"),
        ("GPIO23", vs_a["MOSI"], "#0ea5e9"),
        ("GPIO21", vs_a["SDCS"], "#16a34a"),
        ("GPIO15", tft_a["CS"], "#ca8a04"),
        ("GPIO2", tft_a["DC / A0"], "#ca8a04"),
        ("GPIO18", tft_a["SCL / SCK"], "#0ea5e9"),
        ("GPIO23", tft_a["SDA / MOSI"], "#0ea5e9"),
        ("GPIO33", tft_a["BL / LED"], "#ca8a04"),
        ("GPIO25", enc_a["CLK"], "#7c3aed"),
        ("GPIO26", enc_a["DT"], "#7c3aed"),
        ("GPIO27", enc_a["SW"], "#7c3aed"),
    ]

    # Orthogonal channel router: private vx / hy / wx per net.
    # Long horizontals sit near each net's natural mid-Y (not on pin rows),
    # so different nets never share a colinear segment (crossings = 1 px OK).
    from collections import defaultdict

    TRACK = 14
    CLEAR = 5  # min distance from pin-row stubs/approaches (line width 2)
    n_routes = len(routes)
    vx = [436 + i * TRACK for i in range(n_routes)]
    wx = [974 - i * TRACK for i in range(n_routes)]
    assert vx[-1] < wx[-1] - TRACK, f"track bands collide: vx={vx[-1]} wx={wx[-1]}"

    pin_ys: set[int] = set()
    for _g, (_x, ay) in esp_anchor.items():
        pin_ys.add(ay)
    for anchors in (vs_a, tft_a, enc_a):
        for _n, (_x, ay) in anchors.items():
            pin_ys.add(ay)

    def y_ok(y: int, taken: set[int]) -> bool:
        if y < 96 or y > footer_y - 10:
            return False
        if any(abs(y - py) < CLEAR for py in pin_ys):
            return False
        if any(abs(y - ty) < TRACK for ty in taken):
            return False
        return True

    def pick_hy(prefer: int, taken: set[int]) -> int:
        for delta in range(0, 900):
            for y in (prefer + delta, prefer - delta):
                if y_ok(y, taken):
                    return y
        raise RuntimeError(f"no free hy near {prefer}")

    by_src: dict[str, list[tuple[int, tuple[int, int], str]]] = defaultdict(list)
    for i, (gpio, dest, color) in enumerate(routes):
        by_src[gpio].append((i, dest, color))

    # Assign hy near midpoint(sy, ty), resolving collisions
    hy = [0] * n_routes
    taken_hy: set[int] = set()
    order = sorted(range(n_routes), key=lambda i: (esp_anchor[routes[i][0]][1] + routes[i][1][1]) // 2)
    for i in order:
        sx, sy = esp_anchor[routes[i][0]]
        tx, ty = routes[i][1]
        prefer = (sy + ty) // 2
        # Prefer a slight jog off the pin row even when sy ~= ty
        if abs(sy - ty) < CLEAR:
            prefer = sy - TRACK if y_ok(sy - TRACK, taken_hy) else sy + TRACK
        hy[i] = pick_hy(prefer, taken_hy)
        taken_hy.add(hy[i])

    segments: list[tuple[str, int, int, int, int]] = []

    def draw_seg(x1: int, y1: int, x2: int, y2: int, color: str, net_id: int) -> None:
        if x1 == x2 and y1 == y2:
            return
        draw.line([(x1, y1), (x2, y2)], fill=color, width=2)
        if y1 == y2:
            a, b = sorted((x1, x2))
            if a != b:
                segments.append(("h", y1, a, b, net_id))
        elif x1 == x2:
            a, b = sorted((y1, y2))
            if a != b:
                segments.append(("v", x1, a, b, net_id))

    for gpio, items in by_src.items():
        sx, sy = esp_anchor[gpio]
        items = sorted(items, key=lambda t: vx[t[0]])
        trunk_id = items[0][0]
        trunk_color = items[0][2]
        junc_x = vx[items[0][0]]
        draw_seg(sx, sy, junc_x, sy, trunk_color, trunk_id)
        rightmost = vx[items[-1][0]]
        if rightmost != junc_x:
            draw_seg(junc_x, sy, rightmost, sy, trunk_color, trunk_id)

        for idx, (tx, ty), color in items:
            vxi, wxi, hyi = vx[idx], wx[idx], hy[idx]
            draw_seg(vxi, sy, vxi, hyi, color, idx)
            draw_seg(vxi, hyi, wxi, hyi, color, idx)
            draw_seg(wxi, hyi, wxi, ty, color, idx)
            draw_seg(wxi, ty, tx, ty, color, idx)

    overlaps = 0
    for i in range(len(segments)):
        a1, f1, lo1, hi1, n1 = segments[i]
        for j in range(i + 1, len(segments)):
            a2, f2, lo2, hi2, n2 = segments[j]
            if n1 == n2 or a1 != a2 or f1 != f2:
                continue
            lo = max(lo1, lo2)
            hi = min(hi1, hi2)
            if hi - lo > 1:
                overlaps += 1
                print(f"OVERLAP {a1}@{f1} nets {n1}/{n2} len={hi - lo}")
    print(f"parallel_overlaps={overlaps}")
    if overlaps:
        raise RuntimeError(f"stacked parallel wire runs: {overlaps}")

    rounded_box(draw, (60, footer_y, 780, footer_y + footer_h), fill="#ffffff", outline="#94a3b8", radius=12, width=2)
    draw.text((80, footer_y + 16), "Versorgung / Reset", fill="#0f172a", font=f_h)
    power_lines = [
        "ESP 3V3  →  VS1053 3V3, TFT VCC, Encoder +",
        "ESP 3V3  →  TFT RES / RST  (fest High)",
        "ESP EN   →  VS1053 RST     (gemeinsamer Reset)",
        "ESP GND  →  VS1053, TFT, Encoder GND",
        "SPI gemeinsam: SCK=18  MOSI=23  MISO=19",
        "Firmware: TFT RST-GPIO = -1 (nur Hardware-RES)",
    ]
    py = footer_y + 52
    for line in power_lines:
        draw.text((80, py), line, fill="#334155", font=f_s)
        py += 26

    rounded_box(draw, (820, footer_y, 1520, footer_y + footer_h), fill="#ffffff", outline="#94a3b8", radius=12, width=2)
    draw.text((840, footer_y + 16), "Farblegende", fill="#0f172a", font=f_h)
    legend = [
        ("#0ea5e9", "SPI-Bus (SCK / MOSI / MISO)"),
        ("#16a34a", "VS1053 Steuerung / SDCS"),
        ("#ca8a04", "TFT Steuerung / Backlight"),
        ("#f97316", "TFT RES → +3V3"),
        ("#ef4444", "VS1053 RST → EN"),
        ("#7c3aed", "Rotary Encoder"),
    ]
    ly = footer_y + 52
    for color, text in legend:
        draw.line([(850, ly + 8), (930, ly + 8)], fill=color, width=5)
        draw.text((950, ly), text, fill="#334155", font=f_s)
        ly += 28

    out_png = OUT / "ESP32-Webradio-wroom_tft-wiring.png"
    img.save(out_png, "PNG", optimize=True)
    print("wrote", out_png, f"{W}x{H}")

    wiring = OUT.parent.parent / "wiring" / "wroom_tft_wiring.png"
    wiring.parent.mkdir(parents=True, exist_ok=True)
    img.save(wiring, "PNG", optimize=True)
    print("wrote", wiring)

    svg_path = OUT / "ESP32-Webradio-wroom_tft-wiring.svg"
    svg_path.write_text(
        "\n".join(
            [
                '<?xml version="1.0" encoding="UTF-8"?>',
                f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">',
                '<rect width="100%" height="100%" fill="#f4f6f8"/>',
                f'<image href="{out_png.name}" x="0" y="0" width="{W}" height="{H}"/>',
                "</svg>",
            ]
        ),
        encoding="utf-8",
    )
    print("wrote", svg_path)
    print(
        "layout",
        {
            "vs": (vs_y0, vs_bottom, vs_y0 + (len(vs_pins) - 1) * PIN_STEP),
            "tft": (tft_y0, tft_bottom, tft_y0 + (len(tft_pins) - 1) * PIN_STEP),
            "enc": (enc_y0, enc_bottom, enc_y0 + (len(enc_pins) - 1) * PIN_STEP),
        },
    )


if __name__ == "__main__":
    main()
