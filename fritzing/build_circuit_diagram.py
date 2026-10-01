#!/usr/bin/env python3
"""
Generate Fritzing wiring diagrams (.fzz) for ESP32-Webradio hardware variants.

Pin defaults are taken from src/config/hardware_profile.h
(wroom_tft and cam_headless profiles).

Wires use orthogonal (Manhattan) multi-segment routes. Schematic nets prefer a
horizontal highway above the parts with short vertical stub columns at the MCU
and at the outside edge of peripherals (VS1053 SPI pins face right, so stubs
approach from the right and do not cut through the symbol).

Usage:
  python3 build_circuit_diagram.py            # both variants
  python3 build_circuit_diagram.py wroom_tft
  python3 build_circuit_diagram.py cam_headless
  python3 build_circuit_diagram.py all

Outputs:
  circuit_diagram_wroom_tft.fzz
  circuit_diagram_cam_headless.fzz
  circuit_diagram.fzz  (alias copy of the WROOM diagram)

PNG / SVG export:
  Fritzing Flatpak supports headless SVG export of all sketches in a folder:
    flatpak run org.fritzing.Fritzing -svg /path/to/folder_with_fzz
  Open a .fzz in the Fritzing GUI to export PNG (File > Export > as Image).
  This script does not require the GUI; it always writes valid .fzz archives.
"""

from __future__ import annotations

import argparse
import math
import shutil
import zipfile
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from xml.dom import minidom
from xml.etree.ElementTree import Element, SubElement, tostring, parse as xml_parse

SCRIPT_DIR = Path(__file__).resolve().parent
PARTS_SRC = SCRIPT_DIR / "parts"
BUILD_DIR = SCRIPT_DIR / "_build"

FRITZING_CORE = Path(
    "/var/lib/flatpak/app/org.fritzing.Fritzing/x86_64/stable/active/"
    "files/share/fritzing/parts/core"
)

VARIANT_WROOM = "wroom_tft"
VARIANT_CAM = "cam_headless"

# Bus pitch in Fritzing grid units (aligns near 0.1in grid).
SCH_BUS_PITCH = 8.0
BB_BUS_PITCH = 10.0
COORD_EPS = 0.05
# Extra clearance so stub columns stay outside part bodies.
SCH_STUB_GAP = 12.0

# ---------------------------------------------------------------------------
# Pin maps from src/config/hardware_profile.h
# ---------------------------------------------------------------------------
PINS_WROOM = {
    "vs_cs": 5,
    "vs_dcs": 32,
    "vs_dreq": 4,
    "spi_sck": 18,
    "spi_miso": 19,
    "spi_mosi": 23,
    "tft_cs": 15,
    "tft_dc": 2,
    "tft_bl": 33,
    "sd_cs": 21,
    "enc_clk": 25,
    "enc_dt": 26,
    "enc_sw": 27,
}

PINS_CAM = {
    "vs_cs": 5,
    "vs_dcs": 32,
    "vs_dreq": 4,
    "spi_sck": 18,
    "spi_miso": 19,
    "spi_mosi": 23,
    "enc_clk": 25,
    "enc_dt": 26,
    "enc_sw": 27,
    # Onboard SD_MMC 1-bit (AI-Thinker, internal to board, not wired to VS1053)
    "sd_mmc_d0": 2,
    "sd_mmc_clk": 14,
    "sd_mmc_cmd": 15,
}

# ESP32-38PinWide.fzp: connector id by GPIO number
WROOM_GPIO = {
    0: "connector24",
    2: "connector23",
    4: "connector25",
    5: "connector28",
    15: "connector22",
    18: "connector29",
    19: "connector30",
    21: "connector32",
    23: "connector36",
    25: "connector8",
    26: "connector9",
    27: "connector10",
    32: "connector6",
    33: "connector7",
}
WROOM_3V3 = "connector0"
WROOM_GND = ["connector13", "connector31", "connector37"]

# Custom ESP32-CAM-AIThinker.fzp connector map
CAM_GPIO = {
    2: "connector11",
    4: "connector2",
    5: "connector3",
    14: "connector12",
    15: "connector13",
    18: "connector4",
    19: "connector5",
    23: "connector6",
    25: "connector7",
    26: "connector8",
    27: "connector9",
    32: "connector10",
}
CAM_3V3 = "connector0"
CAM_GND = ["connector1", "connector1", "connector1"]

VS1053 = {
    "3v3_a": "connector58",
    "3v3_b": "connector71",
    "gnd_a": "connector55",
    "gnd_b": "connector59",
    "cs": "connector64",
    "dcs": "connector66",
    "dreq": "connector56",
    "sck": "connector62",
    "miso": "connector60",
    "mosi": "connector61",
    "rst": "connector63",
    "sdcs": "connector65",
}

TFT = {
    "vcc": "connector27",
    "gnd": "connector28",
    "cs": "connector23",
    "dc": "connector25",
    "mosi": "connector22",
    "sck": "connector21",
    "miso": "connector20",
    "bl": "connector19",
    "rst": "connector26",
}

ENC = {"clk": "connector0", "gnd": "connector1", "dt": "connector2"}
SW = {"sig": "connector0", "gnd": "connector2"}

COLORS = {
    "power": "#ff0000",
    "gnd": "#000000",
    "spi": "#33ffc7",
    "vs1053": "#cc6600",
    "tft": "#33ccff",
    "sd": "#9966ff",
    "enc": "#418dd9",
}

IDX_PCB = 5768
IDX_BB = 5770
IDX_MCU = 100003
IDX_VS1053 = 100004
IDX_TFT = 100005
IDX_ENC = 100006
IDX_SW = 100007
IDX_WIRE = 100100


@dataclass
class BoardConfig:
    """MCU part placement and connector lookup for one hardware variant."""

    variant: str
    title: str
    pins: dict[str, int]
    gpio: dict[int, str]
    pin_3v3: str
    pin_gnd: list[str]
    module_id: str
    fzp_path: str
    part_title: str
    bread_xy: tuple[float, float]
    sch_xy: tuple[float, float]
    pcb_xy: tuple[float, float]
    include_tft: bool
    include_spi_sd: bool
    # Schematic highway: horizontal tracks above parts, stub columns at sides.
    sch_left_wrap_x: float
    sch_hwy_y0: float
    sch_bot_bus_y: float
    # Breadboard routing channels
    bb_bus_x0: float
    bb_bot_bus_y: float


BOARD_WROOM = BoardConfig(
    variant=VARIANT_WROOM,
    title="ESP32-Webradio WROOM + TFT",
    pins=PINS_WROOM,
    gpio=WROOM_GPIO,
    pin_3v3=WROOM_3V3,
    pin_gnd=WROOM_GND,
    module_id="ESP32-38PinWide",
    fzp_path="part.ESP32-38PinWide.fzp",
    part_title="ESP32-WROOM",
    # MCU left; VS top-right; TFT bottom-right; encoder below MCU
    bread_xy=(40, 25),
    sch_xy=(30, -220),
    pcb_xy=(40, 20),
    include_tft=True,
    include_spi_sd=True,
    sch_left_wrap_x=8.0,
    sch_hwy_y0=-340.0,
    sch_bot_bus_y=140.0,
    bb_bus_x0=180.0,
    bb_bot_bus_y=270.0,
)

BOARD_CAM = BoardConfig(
    variant=VARIANT_CAM,
    title="ESP32-Webradio CAM headless",
    pins=PINS_CAM,
    gpio=CAM_GPIO,
    pin_3v3=CAM_3V3,
    pin_gnd=CAM_GND,
    module_id="ESP32-CAM-AIThinker",
    fzp_path="part.ESP32-CAM-AIThinker.fzp",
    part_title="ESP32-CAM",
    # Facing layout: VS left (SPI on right) + CAM right (SPI on left) for short gap buses.
    bread_xy=(280, 30),
    sch_xy=(240, -200),
    pcb_xy=(200, 20),
    include_tft=False,
    include_spi_sd=False,
    sch_left_wrap_x=100.0,
    sch_hwy_y0=-290.0,
    sch_bot_bus_y=70.0,
    bb_bus_x0=180.0,
    bb_bot_bus_y=250.0,
)

BOARDS = {
    VARIANT_WROOM: BOARD_WROOM,
    VARIANT_CAM: BOARD_CAM,
}


@dataclass
class PartViews:
    """Holds Fritzing view elements for one placed component."""

    model_index: int
    breadboard_view: Element
    schematic_view: Element
    pcb_view: Element | None = None
    sch_x: float = 0.0
    sch_y: float = 0.0
    bb_x: float = 0.0
    bb_y: float = 0.0
    sch_flip_x: bool = False
    terminals: dict[str, tuple[float, float]] = field(default_factory=dict)
    bb_terminals: dict[str, tuple[float, float]] = field(default_factory=dict)
    connects: dict[str, list[tuple[int, str, str, str]]] = field(
        default_factory=lambda: defaultdict(list)
    )

    def _sch_local(self, connector_id: str) -> tuple[float, float]:
        """Local schematic terminal, applying optional horizontal flip."""
        tx, ty = self.terminals[connector_id]
        if not self.sch_flip_x or not self.terminals:
            return tx, ty
        max_x = max(p[0] for p in self.terminals.values())
        return max_x - tx, ty

    def abs_terminal(self, connector_id: str) -> tuple[float, float]:
        """Absolute schematic coordinates for a connector terminal."""
        tx, ty = self._sch_local(connector_id)
        return self.sch_x + tx, self.sch_y + ty

    def abs_bb_terminal(self, connector_id: str) -> tuple[float, float]:
        """Absolute breadboard coordinates for a connector terminal."""
        tx, ty = self.bb_terminals[connector_id]
        return self.bb_x + tx, self.bb_y + ty


def resolve_fzp_path(path: str) -> Path | None:
    """Resolve embedded or core Fritzing part path to a local file."""
    if path.startswith(":/resources/parts/core/"):
        local = FRITZING_CORE / Path(path).name
        return local if local.is_file() else None
    local = PARTS_SRC / Path(path).name
    return local if local.is_file() else None


def find_schematic_svg(fzp_path: Path) -> Path | None:
    """Locate the schematic SVG referenced by a .fzp file."""
    root = xml_parse(fzp_path).getroot()
    image_ref = None
    for layers in root.findall(".//schematicView/layers"):
        image_ref = layers.get("image")
        if image_ref:
            break
    if not image_ref:
        return None

    svg_name = Path(image_ref).name
    sibling = fzp_path.parent / f"svg.schematic.{svg_name}"
    if sibling.is_file():
        return sibling

    core_svg = FRITZING_CORE.parent / "svg" / "core" / "schematic" / svg_name
    if core_svg.is_file():
        return core_svg

    core_svg_flat = FRITZING_CORE.parent / "svg" / "core" / svg_name
    if core_svg_flat.is_file():
        return core_svg_flat

    return None


def find_breadboard_svg(fzp_path: Path) -> Path | None:
    """Locate the breadboard SVG referenced by a .fzp file."""
    root = xml_parse(fzp_path).getroot()
    image_ref = None
    for layers in root.findall(".//breadboardView/layers"):
        image_ref = layers.get("image")
        if image_ref:
            break
    if not image_ref:
        return None

    svg_name = Path(image_ref).name
    sibling = fzp_path.parent / f"svg.breadboard.{svg_name}"
    if sibling.is_file():
        return sibling

    core_svg = FRITZING_CORE.parent / "svg" / "core" / "breadboard" / svg_name
    if core_svg.is_file():
        return core_svg

    core_svg_flat = FRITZING_CORE.parent / "svg" / "core" / svg_name
    if core_svg_flat.is_file():
        return core_svg_flat

    return None


def parse_svg_length(raw: str | None) -> float | None:
    """Parse Fritzing SVG width/height attributes into inches."""
    if not raw:
        return None
    value = raw.strip()
    if value.endswith("in"):
        return float(value[:-2] or 0)
    if value.endswith("px"):
        return float(value[:-2] or 0) / 72.0
    if value.endswith("cm"):
        return float(value[:-2] or 0) / 2.54
    try:
        return float(value)
    except ValueError:
        return None


def svg_to_grid_factor(svg_root: Element) -> tuple[float, float]:
    """
    Convert Fritzing SVG user units to breadboard/schematic grid units.

    Fritzing stores wire endpoints in grid space where pin artwork is scaled by
    roughly ``width_in * 90 / viewBox_width``. Raw SVG coordinates break wires.
    """
    view_box = svg_root.get("viewBox", "0 0 0 0").split()
    if len(view_box) < 4:
        return 1.0, 1.0

    vb_w = float(view_box[2])
    vb_h = float(view_box[3])
    if vb_w <= 0 or vb_h <= 0:
        return 1.0, 1.0

    width_in = parse_svg_length(svg_root.get("width"))
    height_in = parse_svg_length(svg_root.get("height"))
    if width_in is None:
        width_in = vb_w / 1000.0
    if height_in is None:
        height_in = vb_h / 1000.0

    return width_in * 90.0 / vb_w, height_in * 90.0 / vb_h


def schematic_svg_to_grid_factor(svg_root: Element) -> tuple[float, float]:
    """Backward-compatible alias used by schematic terminal loading."""
    return svg_to_grid_factor(svg_root)


def parse_svg_transform(raw: str | None) -> tuple[float, float, float, float, float, float]:
    """Parse a 2D SVG matrix(), rotate(), or translate() transform."""
    if not raw:
        return 1.0, 0.0, 0.0, 1.0, 0.0, 0.0
    values = raw.strip()
    if values.startswith("matrix("):
        inner = values[7:-1]
        parts = [float(part) for part in inner.replace(",", " ").split()]
        if len(parts) == 6:
            return parts[0], parts[1], parts[2], parts[3], parts[4], parts[5]
    if values.startswith("translate("):
        inner = values[10:-1].replace(",", " ").split()
        tx = float(inner[0]) if inner else 0.0
        ty = float(inner[1]) if len(inner) > 1 else 0.0
        return 1.0, 0.0, 0.0, 1.0, tx, ty
    if values.startswith("rotate("):
        inner = values[7:-1].replace(",", " ").split()
        angle = float(inner[0])
        radians = math.radians(angle)
        cos_a = math.cos(radians)
        sin_a = math.sin(radians)
        if len(inner) >= 3:
            cx = float(inner[1])
            cy = float(inner[2])
            return cos_a, sin_a, -sin_a, cos_a, cx - cos_a * cx + sin_a * cy, cy - sin_a * cx - cos_a * cy
        return cos_a, sin_a, -sin_a, cos_a, 0.0, 0.0
    return 1.0, 0.0, 0.0, 1.0, 0.0, 0.0


def compose_svg_transforms(
    outer: tuple[float, float, float, float, float, float],
    inner: tuple[float, float, float, float, float, float],
) -> tuple[float, float, float, float, float, float]:
    """Compose two SVG affine transforms (outer * inner)."""
    a1, b1, c1, d1, e1, f1 = outer
    a2, b2, c2, d2, e2, f2 = inner
    return (
        a1 * a2 + c1 * b2,
        b1 * a2 + d1 * b2,
        a1 * c2 + c1 * d2,
        b1 * c2 + d1 * d2,
        a1 * e2 + c1 * f2 + e1,
        b1 * e2 + d1 * f2 + f1,
    )


def apply_svg_transform(
    x: float,
    y: float,
    matrix: tuple[float, float, float, float, float, float],
) -> tuple[float, float]:
    """Apply an SVG affine transform to a point."""
    a, b, c, d, e, f = matrix
    return a * x + c * y + e, b * x + d * y + f


def collect_svg_points(
    svg_root: Element,
    point_fn,
) -> dict[str, tuple[float, float]]:
    """Collect named SVG elements with absolute coordinates including group transforms."""
    points: dict[str, tuple[float, float]] = {}
    identity = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)

    def walk(node: Element, matrix: tuple[float, float, float, float, float, float]) -> None:
        tag = node.tag.rsplit("}", 1)[-1]
        next_matrix = matrix
        if tag == "g":
            next_matrix = compose_svg_transforms(matrix, parse_svg_transform(node.get("transform")))

        elem_id = node.get("id")
        if elem_id:
            local = point_fn(node)
            if local is not None:
                node_matrix = compose_svg_transforms(
                    next_matrix,
                    parse_svg_transform(node.get("transform")),
                )
                points[elem_id] = apply_svg_transform(local[0], local[1], node_matrix)

        for child in list(node):
            walk(child, next_matrix)

    walk(svg_root, identity)
    return points


def local_pin_point(elem: Element) -> tuple[float, float] | None:
    """Return the local breadboard connection point for a pin SVG element."""
    tag = elem.tag.rsplit("}", 1)[-1]
    if tag == "line":
        x1 = float(elem.get("x1", 0) or 0)
        y1 = float(elem.get("y1", 0) or 0)
        x2 = float(elem.get("x2", 0) or 0)
        y2 = float(elem.get("y2", 0) or 0)
        if abs(y2) >= abs(y1):
            return x2, y2
        return x1, y1
    if tag == "rect":
        x = float(elem.get("x", 0) or 0)
        y = float(elem.get("y", 0) or 0)
        w = float(elem.get("width", 0) or 0)
        h = float(elem.get("height", 0) or 0)
        return x + w / 2.0, y + h
    if tag in ("circle", "ellipse"):
        return float(elem.get("cx", 0) or 0), float(elem.get("cy", 0) or 0)
    return None


def schematic_pin_point(elem: Element) -> tuple[float, float] | None:
    """Return the local schematic connection point for a pin SVG element."""
    tag = elem.tag.rsplit("}", 1)[-1]
    if tag == "line":
        x1 = float(elem.get("x1", 0) or 0)
        y1 = float(elem.get("y1", 0) or 0)
        x2 = float(elem.get("x2", 0) or 0)
        y2 = float(elem.get("y2", 0) or 0)
        return x2, y2
    if tag == "rect":
        x = float(elem.get("x", 0) or 0)
        y = float(elem.get("y", 0) or 0)
        w = float(elem.get("width", 0) or 0)
        h = float(elem.get("height", 0) or 0)
        return x + w / 2.0, y + h / 2.0
    if tag in ("circle", "ellipse"):
        return float(elem.get("cx", 0) or 0), float(elem.get("cy", 0) or 0)
    return None


def load_part_terminals(fzp_path: Path) -> dict[str, tuple[float, float]]:
    """
    Map Fritzing connector ids to schematic terminal coordinates.

    Coordinates are relative to the part origin in schematic grid units.
    """
    svg_path = find_schematic_svg(fzp_path)
    if svg_path is None:
        return {}

    fzp_root = xml_parse(fzp_path).getroot()
    connector_pins: dict[str, str] = {}
    terminal_ids: dict[str, str] = {}
    for conn in fzp_root.findall(".//connector"):
        conn_id = conn.get("id")
        if not conn_id:
            continue
        for pin in conn.findall(".//schematicView/p"):
            svg_id = pin.get("svgId")
            term_id = pin.get("terminalId")
            if svg_id:
                connector_pins[conn_id] = svg_id
            if term_id:
                terminal_ids[conn_id] = term_id
            if svg_id or term_id:
                break

    svg_root = xml_parse(svg_path).getroot()
    scale_x, scale_y = schematic_svg_to_grid_factor(svg_root)
    svg_points = collect_svg_points(svg_root, schematic_pin_point)

    terminals: dict[str, tuple[float, float]] = {}
    for conn_id, term_id in terminal_ids.items():
        pin_id = connector_pins.get(conn_id)
        svg_x, svg_y = None, None
        if term_id in svg_points:
            svg_x, svg_y = svg_points[term_id]
        elif pin_id and pin_id in svg_points:
            svg_x, svg_y = svg_points[pin_id]
        if svg_x is None or svg_y is None:
            continue
        terminals[conn_id] = (svg_x * scale_x, svg_y * scale_y)
    return terminals


def load_part_breadboard_terminals(fzp_path: Path) -> dict[str, tuple[float, float]]:
    """
    Map Fritzing connector ids to breadboard terminal coordinates.

    Coordinates are relative to the part origin in breadboard grid units.
    """
    svg_path = find_breadboard_svg(fzp_path)
    if svg_path is None:
        return {}

    fzp_root = xml_parse(fzp_path).getroot()
    connector_pins: dict[str, str] = {}
    connector_legs: dict[str, str] = {}
    for conn in fzp_root.findall(".//connector"):
        conn_id = conn.get("id")
        if not conn_id:
            continue
        for pin in conn.findall(".//breadboardView/p"):
            svg_id = pin.get("svgId")
            leg_id = pin.get("legId")
            if svg_id:
                connector_pins[conn_id] = svg_id
            if leg_id:
                connector_legs[conn_id] = leg_id
            if svg_id or leg_id:
                break

    svg_root = xml_parse(svg_path).getroot()
    scale_x, scale_y = svg_to_grid_factor(svg_root)
    svg_points = collect_svg_points(svg_root, local_pin_point)

    terminals: dict[str, tuple[float, float]] = {}
    for conn_id, pin_id in connector_pins.items():
        leg_id = connector_legs.get(conn_id)
        svg_x, svg_y = None, None
        if leg_id and leg_id in svg_points:
            svg_x, svg_y = svg_points[leg_id]
        elif pin_id in svg_points:
            svg_x, svg_y = svg_points[pin_id]
        if svg_x is None or svg_y is None:
            continue
        terminals[conn_id] = (svg_x * scale_x, svg_y * scale_y)
    return terminals


# ---------------------------------------------------------------------------
# Orthogonal routing
# ---------------------------------------------------------------------------

def _same_point(a: tuple[float, float], b: tuple[float, float]) -> bool:
    return abs(a[0] - b[0]) <= COORD_EPS and abs(a[1] - b[1]) <= COORD_EPS


def dedupe_points(points: list[tuple[float, float]]) -> list[tuple[float, float]]:
    """Remove consecutive duplicate points from a polyline."""
    if not points:
        return points
    out = [points[0]]
    for pt in points[1:]:
        if not _same_point(pt, out[-1]):
            out.append(pt)
    return out


def flatten_colinear(points: list[tuple[float, float]]) -> list[tuple[float, float]]:
    """Drop middle points that keep the previous and next segment colinear."""
    pts = dedupe_points(points)
    if len(pts) < 3:
        return pts
    out = [pts[0]]
    for i in range(1, len(pts) - 1):
        x0, y0 = out[-1]
        x1, y1 = pts[i]
        x2, y2 = pts[i + 1]
        horizontal = abs(y0 - y1) <= COORD_EPS and abs(y1 - y2) <= COORD_EPS
        vertical = abs(x0 - x1) <= COORD_EPS and abs(x1 - x2) <= COORD_EPS
        if horizontal or vertical:
            continue
        out.append(pts[i])
    out.append(pts[-1])
    return dedupe_points(out)


def route_via_vertical_bus(
    x1: float,
    y1: float,
    x2: float,
    y2: float,
    bus_x: float,
) -> list[tuple[float, float]]:
    """Manhattan path using a vertical bus lane at bus_x."""
    return flatten_colinear(
        [
            (x1, y1),
            (bus_x, y1),
            (bus_x, y2),
            (x2, y2),
        ]
    )


def route_via_horizontal_bus(
    x1: float,
    y1: float,
    x2: float,
    y2: float,
    bus_y: float,
) -> list[tuple[float, float]]:
    """Manhattan path using a horizontal bus lane at bus_y."""
    return flatten_colinear(
        [
            (x1, y1),
            (x1, bus_y),
            (x2, bus_y),
            (x2, y2),
        ]
    )


def route_via_highway(
    x1: float,
    y1: float,
    x2: float,
    y2: float,
    src_col: float,
    dst_col: float,
    track_y: float,
) -> list[tuple[float, float]]:
    """
    Orthogonal highway: vertical stub to a horizontal track, then stub to destination.

    Keeps long runs on parallel tracks and short stubs at the part edges so wires
    do not cut through schematic symbols.
    """
    return flatten_colinear(
        [
            (x1, y1),
            (src_col, y1),
            (src_col, track_y),
            (dst_col, track_y),
            (dst_col, y2),
            (x2, y2),
        ]
    )


def part_pin_side(part: PartViews, connector_id: str) -> str:
    """Classify a schematic pin as left/right/top/bottom on its symbol."""
    tx, ty = part._sch_local(connector_id)
    xs = [part._sch_local(cid)[0] for cid in part.terminals]
    ys = [part._sch_local(cid)[1] for cid in part.terminals]
    min_x, max_x = min(xs), max(xs)
    min_y, max_y = min(ys), max(ys)
    # Prefer vertical edges when the pin sits on a side.
    if abs(tx - min_x) <= abs(tx - max_x) and abs(tx - min_x) <= 8.0:
        if abs(ty - min_y) <= 4.0:
            return "top"
        if abs(ty - max_y) <= 4.0:
            return "bottom"
        return "left"
    if abs(tx - max_x) <= 8.0:
        if abs(ty - min_y) <= 4.0:
            return "top"
        if abs(ty - max_y) <= 4.0:
            return "bottom"
        return "right"
    if abs(ty - min_y) <= abs(ty - max_y):
        return "top"
    return "bottom"


def part_edge_x(part: PartViews, side: str) -> float:
    """Absolute X of the left or right schematic pin edge."""
    xs = [part.abs_terminal(cid)[0] for cid in part.terminals]
    return min(xs) if side == "left" else max(xs)


def part_edge_y(part: PartViews, side: str) -> float:
    """Absolute Y of the top or bottom schematic pin edge."""
    ys = [part.abs_terminal(cid)[1] for cid in part.terminals]
    return min(ys) if side == "top" else max(ys)


def stub_column_for_pin(
    part: PartViews,
    connector_id: str,
    lane: int,
    pitch: float = 5.0,
) -> float:
    """
    Vertical stub column outside the part body for the given pin.

    Right-facing pins approach from the right; left-facing from the left;
    top/bottom pins use the pin X (drop/rise onto the highway).
    Lane offsets stay tight so parallel stubs read as one bus.
    """
    side = part_pin_side(part, connector_id)
    x, _ = part.abs_terminal(connector_id)
    offset = SCH_STUB_GAP + lane * pitch
    if side == "right":
        return part_edge_x(part, "right") + offset
    if side == "left":
        return part_edge_x(part, "left") - offset
    return x


def mcu_src_column(
    board: BoardConfig,
    parts: dict[int, PartViews],
    connector_id: str,
    lane: int,
) -> float:
    """Stub column leaving the MCU toward the bus (tight shared column + lane)."""
    mcu = parts[IDX_MCU]
    side = part_pin_side(mcu, connector_id)
    if side == "left":
        return part_edge_x(mcu, "left") - SCH_STUB_GAP - lane * 4.0
    if side == "right":
        return part_edge_x(mcu, "right") + SCH_STUB_GAP + lane * 5.0
    x, _ = mcu.abs_terminal(connector_id)
    return x


def y_clears_part(part: PartViews, y: float, margin: float = 10.0) -> bool:
    """True when a horizontal at absolute y does not cut through the part body."""
    top = part_edge_y(part, "top") - margin
    bottom = part_edge_y(part, "bottom") + margin
    return y < top or y > bottom


def route_right_bus(
    x1: float,
    y1: float,
    x2: float,
    y2: float,
    src_col: float,
    dst_col: float,
    clear_y: float | None = None,
) -> list[tuple[float, float]]:
    """
    Prefer a single horizontal at the source pin Y into a shared right stub column.

    If clear_y is set (obstacle in the way), jog via that Y instead.
    """
    if clear_y is None:
        return flatten_colinear(
            [
                (x1, y1),
                (src_col, y1),
                (dst_col, y1),
                (dst_col, y2),
                (x2, y2),
            ]
        )
    return flatten_colinear(
        [
            (x1, y1),
            (src_col, y1),
            (src_col, clear_y),
            (dst_col, clear_y),
            (dst_col, y2),
            (x2, y2),
        ]
    )



def prettify_xml(element: Element) -> str:
    rough = tostring(element, encoding="unicode")
    parsed = minidom.parseString(rough)
    return parsed.toprettyxml(indent="    ", encoding="UTF-8").decode("UTF-8")


def add_geometry(
    parent: Element,
    x: float,
    y: float,
    z: float,
    flip_x: bool = False,
    flip_width: float = 0.0,
) -> None:
    """Write a Fritzing geometry element, optionally with a horizontal flip."""
    geom = SubElement(parent, "geometry", {"z": f"{z:.5f}", "x": f"{x}", "y": f"{y}"})
    if flip_x:
        # Qt/Fritzing matrix: x' = -x + width (mirror about local x=width/2).
        SubElement(
            geom,
            "transform",
            {
                "m11": "-1",
                "m12": "0",
                "m13": "0",
                "m21": "0",
                "m22": "1",
                "m23": "0",
                "m31": f"{flip_width:.4f}".rstrip("0").rstrip("."),
                "m32": "0",
                "m33": "1",
            },
        )


def mcu_pin(board: BoardConfig, gpio: int) -> tuple[int, str]:
    """Return (modelIndex, connectorId) for an MCU GPIO on the active board."""
    return IDX_MCU, board.gpio[gpio]


def register_part_wire_end(
    parts: dict[int, PartViews],
    part_idx: int,
    conn_id: str,
    wire_idx: int,
    wire_end: str,
) -> None:
    """Register one wire endpoint on a component for all views."""
    parts[part_idx].connects[conn_id].append((wire_idx, wire_end, "breadboard", "breadboardWire"))
    parts[part_idx].connects[conn_id].append((wire_idx, wire_end, "schematic", "schematicTrace"))
    parts[part_idx].connects[conn_id].append((wire_idx, wire_end, "copper0", "copper0trace"))


def _fmt_coord(value: float) -> str:
    if abs(value) < 1e-9:
        return "0"
    return f"{value:.4f}".rstrip("0").rstrip(".")


def make_wire_segment(
    instances: Element,
    wire_idx: int,
    title: str,
    color: str,
    sch_x: float,
    sch_y: float,
    sch_dx: float,
    sch_dy: float,
    bb_x: float,
    bb_y: float,
    bb_dx: float,
    bb_dy: float,
    end0_links: list[tuple[int, str, str]],
    end1_links: list[tuple[int, str, str]],
) -> None:
    """
    Create one WireModuleID segment.

    Each end*_links entry is (modelIndex, connectorId, layer) where layer is the
    *target* layer: schematic / schematicTrace / breadboard / breadboardWire /
    copper0 / copper0trace.
    """
    inst = SubElement(
        instances,
        "instance",
        {
            "moduleIdRef": "WireModuleID",
            "modelIndex": str(wire_idx),
            "path": ":/resources/parts/core/wire.fzp",
        },
    )
    SubElement(inst, "title").text = title
    views = SubElement(inst, "views")

    view_specs = (
        (
            "breadboardView",
            "breadboardWire",
            (bb_x, bb_y, bb_dx, bb_dy),
            "22.2222",
            "64",
            "3.5",
            "breadboardWire",
            "breadboard",
        ),
        (
            "schematicView",
            "schematicTrace",
            (sch_x, sch_y, sch_dx, sch_dy),
            "9.7222",
            "128",
            "5.5",
            "schematicTrace",
            "schematic",
        ),
        (
            "pcbView",
            "copper0trace",
            (bb_x, bb_y, bb_dx, bb_dy),
            "11.1111",
            "4",
            "6.5",
            "copper0trace",
            "copper0",
        ),
    )

    for view_name, layer, geom, mils, wire_flags, z, wire_layer, part_layer in view_specs:
        view = SubElement(views, view_name, {"layer": layer})
        SubElement(
            view,
            "geometry",
            {
                "z": z,
                "x": _fmt_coord(geom[0]),
                "y": _fmt_coord(geom[1]),
                "x1": "0",
                "y1": "0",
                "x2": _fmt_coord(geom[2]),
                "y2": _fmt_coord(geom[3]),
                "wireFlags": wire_flags,
            },
        )
        SubElement(view, "wireExtras", {"mils": mils, "color": color, "opacity": "1", "banded": "0"})
        conns = SubElement(view, "connectors")
        for end_idx, links in ((0, end0_links), (1, end1_links)):
            c = SubElement(conns, "connector", {"connectorId": f"connector{end_idx}", "layer": layer})
            SubElement(c, "geometry", {"x": "0", "y": "0"})
            connects = SubElement(c, "connects")
            for model, conn, target_kind in links:
                if target_kind == "wire":
                    target_layer = wire_layer
                else:
                    target_layer = part_layer
                SubElement(
                    connects,
                    "connect",
                    {
                        "connectorId": conn,
                        "modelIndex": str(model),
                        "layer": target_layer,
                    },
                )


def make_routed_wire(
    instances: Element,
    parts: dict[int, PartViews],
    wire_start: int,
    title: str,
    color: str,
    end_a: tuple[int, str],
    end_b: tuple[int, str],
    sch_points: list[tuple[float, float]],
    bb_points: list[tuple[float, float]],
) -> int:
    """
    Create a chain of orthogonal wire segments between two part connectors.

    Returns the next free wire modelIndex.
    """
    sch_points = flatten_colinear(sch_points)
    bb_points = flatten_colinear(bb_points)
    if len(sch_points) < 2:
        raise ValueError(f"{title}: schematic path too short")
    if len(bb_points) < 2:
        raise ValueError(f"{title}: breadboard path too short")

    def expand_to_len(
        points: list[tuple[float, float]],
        target_len: int,
    ) -> list[tuple[float, float]]:
        """Pad a polyline to target_len points by repeating the last bend."""
        pts = list(points)
        while len(pts) < target_len:
            insert_at = max(1, len(pts) - 1)
            pts = pts[:insert_at] + [pts[insert_at - 1]] + pts[insert_at:]
        return pts

    target = max(len(sch_points), len(bb_points))
    sch_points = expand_to_len(sch_points, target)
    bb_points = expand_to_len(bb_points, target)
    n_seg = len(sch_points) - 1
    # Drop zero-length segments that can appear after pad-alignment.
    sch_segs: list[tuple[tuple[float, float], tuple[float, float]]] = []
    bb_segs: list[tuple[tuple[float, float], tuple[float, float]]] = []
    for i in range(n_seg):
        if _same_point(sch_points[i], sch_points[i + 1]) and _same_point(bb_points[i], bb_points[i + 1]):
            continue
        # Keep a tiny schematic stub if only breadboard moves (should be rare).
        s0, s1 = sch_points[i], sch_points[i + 1]
        b0, b1 = bb_points[i], bb_points[i + 1]
        if _same_point(s0, s1):
            s1 = (s0[0] + COORD_EPS, s0[1])
        if _same_point(b0, b1):
            b1 = (b0[0] + COORD_EPS, b0[1])
        sch_segs.append((s0, s1))
        bb_segs.append((b0, b1))
    if not sch_segs:
        raise ValueError(f"{title}: no wire segments after cleanup")

    wire_ids = list(range(wire_start, wire_start + len(sch_segs)))

    for i, ((s0, s1), (b0, b1)) in enumerate(zip(sch_segs, bb_segs)):
        sx0, sy0 = s0
        sx1, sy1 = s1
        bx0, by0 = b0
        bx1, by1 = b1

        if i == 0:
            end0_links = [(end_a[0], end_a[1], "part")]
        else:
            end0_links = [(wire_ids[i - 1], "connector1", "wire")]

        if i == len(sch_segs) - 1:
            end1_links = [(end_b[0], end_b[1], "part")]
        else:
            end1_links = [(wire_ids[i + 1], "connector0", "wire")]

        seg_title = title if len(sch_segs) == 1 else f"{title}_{i}"
        make_wire_segment(
            instances,
            wire_ids[i],
            seg_title,
            color,
            sx0,
            sy0,
            sx1 - sx0,
            sy1 - sy0,
            bx0,
            by0,
            bx1 - bx0,
            by1 - by0,
            end0_links,
            end1_links,
        )

    register_part_wire_end(parts, end_a[0], end_a[1], wire_ids[0], "connector0")
    register_part_wire_end(parts, end_b[0], end_b[1], wire_ids[-1], "connector1")
    return wire_start + len(sch_segs)


def make_part_instance(
    instances: Element,
    parts: dict[int, PartViews],
    module_id: str,
    model_index: int,
    path: str,
    title: str,
    bread_xy: tuple[float, float],
    sch_xy: tuple[float, float],
    pcb_xy: tuple[float, float] | None = None,
    sch_flip_x: bool = False,
) -> PartViews:
    inst = SubElement(
        instances,
        "instance",
        {"moduleIdRef": module_id, "modelIndex": str(model_index), "path": path},
    )
    SubElement(inst, "title").text = title
    views_el = SubElement(inst, "views")

    bb = SubElement(views_el, "breadboardView", {"layer": "breadboard"})
    add_geometry(bb, *bread_xy, 2.5)

    fzp_local = resolve_fzp_path(path)
    terminals: dict[str, tuple[float, float]] = {}
    bb_terminals: dict[str, tuple[float, float]] = {}
    if fzp_local is not None:
        terminals = load_part_terminals(fzp_local)
        bb_terminals = load_part_breadboard_terminals(fzp_local)

    flip_width = max((p[0] for p in terminals.values()), default=0.0) if sch_flip_x else 0.0
    sch = SubElement(views_el, "schematicView", {"layer": "schematic"})
    add_geometry(sch, *sch_xy, 2.5, flip_x=sch_flip_x, flip_width=flip_width)

    pcb = None
    if pcb_xy:
        pcb = SubElement(views_el, "pcbView", {"layer": "copper0"})
        add_geometry(pcb, *pcb_xy, 5.5)

    part = PartViews(
        model_index,
        bb,
        sch,
        pcb,
        sch_x=sch_xy[0],
        sch_y=sch_xy[1],
        bb_x=bread_xy[0],
        bb_y=bread_xy[1],
        sch_flip_x=sch_flip_x,
        terminals=terminals,
        bb_terminals=bb_terminals,
    )
    parts[model_index] = part
    return part


def apply_part_connectors(parts: dict[int, PartViews]) -> None:
    """Write accumulated wire references into each component view."""
    view_layers = (
        ("breadboard_view", "breadboard", "breadboardWire"),
        ("schematic_view", "schematic", "schematicTrace"),
    )
    for part in parts.values():
        if not part.connects:
            continue
        for view_attr, comp_layer, wire_layer in view_layers:
            view = getattr(part, view_attr)
            conns_el = SubElement(view, "connectors")
            for conn_id, links in sorted(part.connects.items()):
                layer_links = [lk for lk in links if lk[2] == comp_layer]
                if not layer_links:
                    continue
                conn_el = SubElement(conns_el, "connector", {"connectorId": conn_id, "layer": comp_layer})
                if comp_layer == "schematic" and conn_id in part.terminals:
                    tx, ty = part._sch_local(conn_id)
                    SubElement(conn_el, "geometry", {"x": str(tx), "y": str(ty)})
                elif comp_layer == "breadboard" and conn_id in part.bb_terminals:
                    tx, ty = part.bb_terminals[conn_id]
                    SubElement(conn_el, "geometry", {"x": str(tx), "y": str(ty)})
                else:
                    SubElement(conn_el, "geometry", {"x": "0", "y": "0"})
                connects_el = SubElement(conn_el, "connects")
                for wire_idx, wire_conn, _, _ in layer_links:
                    SubElement(
                        connects_el,
                        "connect",
                        {
                            "connectorId": wire_conn,
                            "modelIndex": str(wire_idx),
                            "layer": wire_layer,
                        },
                    )
        if part.pcb_view is not None and part.connects:
            conns_el = SubElement(part.pcb_view, "connectors")
            for conn_id, links in sorted(part.connects.items()):
                layer_links = [lk for lk in links if lk[2] == "copper0"]
                if not layer_links:
                    continue
                conn_el = SubElement(conns_el, "connector", {"connectorId": conn_id, "layer": "copper0"})
                SubElement(conn_el, "geometry", {"x": "0", "y": "0"})
                connects_el = SubElement(conn_el, "connects")
                for wire_idx, wire_conn, _, _ in layer_links:
                    SubElement(
                        connects_el,
                        "connect",
                        {
                            "connectorId": wire_conn,
                            "modelIndex": str(wire_idx),
                            "layer": "copper0trace",
                        },
                    )


def _mcu_pin_faces_left(board: BoardConfig, connector_id: str, parts: dict[int, PartViews]) -> bool:
    """True when the MCU schematic pin is on the left edge of the symbol."""
    return part_pin_side(parts[IDX_MCU], connector_id) == "left"


def build_connections(board: BoardConfig, parts: dict[int, PartViews], instances: Element) -> None:
    """Signal connections with horizontal highway buses and edge stubs."""
    wire = IDX_WIRE
    pins = board.pins
    lane = 0

    def next_lane() -> int:
        nonlocal lane
        idx = lane
        lane += 1
        return idx

    # Stable parallel highway tracks (top to bottom) per signal group.
    lanes = {
        "pwr": next_lane(),
        "spi_mosi": next_lane(),
        "spi_miso": next_lane(),
        "spi_sck": next_lane(),
        "vs_dreq": next_lane(),
        "vs_cs": next_lane(),
        "vs_dcs": next_lane(),
        "vs_rst": next_lane(),
        "sd_cs": next_lane(),
        "tft_cs": next_lane(),
        "tft_dc": next_lane(),
        "tft_bl": next_lane(),
        "tft_rst": next_lane(),
        "gnd": next_lane(),
        "enc_clk": next_lane(),
        "enc_dt": next_lane(),
        "enc_sw": next_lane(),
        "gnd_enc": next_lane(),
    }

    def track_y(name: str) -> float:
        """Horizontal track Y: top bus above VS, mid band for TFT, bottom for encoder."""
        if name in ("enc_clk", "enc_dt", "enc_sw", "gnd_enc"):
            return board.sch_bot_bus_y + (lanes[name] - lanes["enc_clk"]) * (SCH_BUS_PITCH * 0.5)
        if name == "gnd":
            return part_edge_y(parts[IDX_MCU], "bottom") + SCH_STUB_GAP + 10.0
        if name.startswith("tft_") and IDX_TFT in parts:
            vs_bottom = part_edge_y(parts[IDX_VS1053], "bottom")
            tft_top = part_edge_y(parts[IDX_TFT], "top")
            tft_order = ("tft_cs", "tft_dc", "tft_bl", "tft_rst")
            local = tft_order.index(name) if name in tft_order else 0
            base = vs_bottom + (tft_top - vs_bottom) * 0.35
            return base + local * SCH_BUS_PITCH
        # Top highway fully above VS body (more negative Y is higher).
        top_order = (
            "pwr",
            "spi_mosi",
            "spi_miso",
            "spi_sck",
            "vs_dreq",
            "vs_cs",
            "vs_dcs",
            "vs_rst",
            "sd_cs",
        )
        local = top_order.index(name) if name in top_order else 0
        vs_top = part_edge_y(parts[IDX_VS1053], "top")
        return vs_top - SCH_STUB_GAP - 4.0 - local * SCH_BUS_PITCH

    def bb_lane_x(name: str) -> float:
        return board.bb_bus_x0 + lanes[name] * BB_BUS_PITCH

    def sch_pts(a: tuple[int, str], b: tuple[int, str], lane_name: str) -> list[tuple[float, float]]:
        x1, y1 = parts[a[0]].abs_terminal(a[1])
        x2, y2 = parts[b[0]].abs_terminal(b[1])
        ty = track_y(lane_name)
        ln = lanes[lane_name]

        # Encoder / switch: bottom ribbon.
        if a[0] in (IDX_ENC, IDX_SW) or b[0] in (IDX_ENC, IDX_SW):
            if a[0] == IDX_MCU:
                src = mcu_src_column(board, parts, a[1], ln)
                return route_via_highway(x1, y1, x2, y2, src, x2, ty)
            return route_via_horizontal_bus(x1, y1, x2, y2, ty)

        if a[0] == IDX_MCU and b[0] in (IDX_VS1053, IDX_TFT):
            dest = parts[b[0]]
            mcu = parts[IDX_MCU]
            src = mcu_src_column(board, parts, a[1], ln)
            dst = stub_column_for_pin(dest, b[1], ln)
            src_side = part_pin_side(mcu, a[1])
            dst_side = part_pin_side(dest, b[1])

            # CAM facing gap: MCU left pins face VS right pins across a shared channel.
            if board.variant == VARIANT_CAM and b[0] == IDX_VS1053:
                if src_side == "left" and dst_side == "right":
                    gap_right = part_edge_x(mcu, "left") - SCH_STUB_GAP
                    gap_left = part_edge_x(dest, "right") + SCH_STUB_GAP
                    span = max(gap_right - gap_left, SCH_BUS_PITCH)
                    bus_x = gap_left + min(ln * 5.0, span - 4.0)
                    return route_via_vertical_bus(x1, y1, x2, y2, bus_x)
                # Right-facing CAM pins (DCS / encoder side): go above, never through the body.
                if src_side == "right":
                    return route_via_highway(x1, y1, x2, y2, src, dst, ty)
                return route_via_highway(x1, y1, x2, y2, src, dst, ty)

            # TFT left-facing control pins: vertical bus in the MCU/TFT gap.
            if b[0] == IDX_TFT and dst_side == "left" and src_side == "right":
                gap_left = part_edge_x(mcu, "right") + SCH_STUB_GAP
                gap_right = part_edge_x(dest, "left") - SCH_STUB_GAP
                bus_x = gap_left + min(ln * 5.0, max(gap_right - gap_left - 4.0, 0.0))
                return route_via_vertical_bus(x1, y1, x2, y2, bus_x)

            # WROOM / default: parallel highway into an outside stub column
            # (VS SPI pins face right, so stubs approach from the right).
            return route_via_highway(x1, y1, x2, y2, src, dst, ty)

        if b[0] == IDX_MCU and a[0] in (IDX_VS1053, IDX_TFT):
            src = stub_column_for_pin(parts[a[0]], a[1], ln)
            dst = mcu_src_column(board, parts, b[1], ln)
            return route_via_highway(x1, y1, x2, y2, src, dst, ty)

        return route_via_highway(
            x1, y1, x2, y2, x1 + SCH_STUB_GAP, x2 - SCH_STUB_GAP, ty
        )

    def bb_pts(a: tuple[int, str], b: tuple[int, str], lane_name: str) -> list[tuple[float, float]]:
        x1, y1 = parts[a[0]].abs_bb_terminal(a[1])
        x2, y2 = parts[b[0]].abs_bb_terminal(b[1])
        bus_x = bb_lane_x(lane_name)

        if a[0] in (IDX_ENC, IDX_SW) or b[0] in (IDX_ENC, IDX_SW):
            bus_y = board.bb_bot_bus_y + (lanes[lane_name] % 4) * (BB_BUS_PITCH * 0.5)
            return route_via_horizontal_bus(x1, y1, x2, y2, bus_y)

        return route_via_vertical_bus(x1, y1, x2, y2, bus_x)

    def w(title: str, color: str, a: tuple[int, str], b: tuple[int, str], lane_name: str) -> None:
        nonlocal wire
        wire = make_routed_wire(
            instances,
            parts,
            wire,
            title,
            color,
            a,
            b,
            sch_pts(a, b, lane_name),
            bb_pts(a, b, lane_name),
        )

    e = lambda gpio: mcu_pin(board, gpio)

    # --- Power ---
    w("3V3_MCU_VS1053_A", COLORS["power"], (IDX_MCU, board.pin_3v3), (IDX_VS1053, VS1053["3v3_a"]), "pwr")
    w("3V3_MCU_VS1053_B", COLORS["power"], (IDX_MCU, board.pin_3v3), (IDX_VS1053, VS1053["3v3_b"]), "pwr")
    w("3V3_VS1053_RST", COLORS["power"], (IDX_MCU, board.pin_3v3), (IDX_VS1053, VS1053["rst"]), "vs_rst")
    if board.include_tft:
        w("3V3_MCU_TFT", COLORS["power"], (IDX_MCU, board.pin_3v3), (IDX_TFT, TFT["vcc"]), "pwr")
        w("3V3_TFT_RST", COLORS["power"], (IDX_MCU, board.pin_3v3), (IDX_TFT, TFT["rst"]), "tft_rst")

    w("GND_MCU_VS1053", COLORS["gnd"], (IDX_MCU, board.pin_gnd[0]), (IDX_VS1053, VS1053["gnd_a"]), "gnd")
    w("GND_MCU_VS1053_B", COLORS["gnd"], (IDX_MCU, board.pin_gnd[1]), (IDX_VS1053, VS1053["gnd_b"]), "gnd")
    if board.include_tft:
        w("GND_MCU_TFT", COLORS["gnd"], (IDX_MCU, board.pin_gnd[1]), (IDX_TFT, TFT["gnd"]), "gnd")
    w("GND_ENC", COLORS["gnd"], (IDX_MCU, board.pin_gnd[2]), (IDX_ENC, ENC["gnd"]), "gnd_enc")
    w("GND_ENC_SW", COLORS["gnd"], (IDX_MCU, board.pin_gnd[2]), (IDX_SW, SW["gnd"]), "gnd_enc")

    # --- VS1053 control ---
    w("GPIO5_VS_CS", COLORS["vs1053"], e(pins["vs_cs"]), (IDX_VS1053, VS1053["cs"]), "vs_cs")
    w("GPIO32_VS_DCS", COLORS["vs1053"], e(pins["vs_dcs"]), (IDX_VS1053, VS1053["dcs"]), "vs_dcs")
    w("GPIO4_VS_DREQ", COLORS["vs1053"], e(pins["vs_dreq"]), (IDX_VS1053, VS1053["dreq"]), "vs_dreq")

    # --- Shared SPI (same track per net; separate wires to each peripheral) ---
    w("GPIO18_SCK_VS", COLORS["spi"], e(pins["spi_sck"]), (IDX_VS1053, VS1053["sck"]), "spi_sck")
    w("GPIO19_MISO_VS", COLORS["spi"], e(pins["spi_miso"]), (IDX_VS1053, VS1053["miso"]), "spi_miso")
    w("GPIO23_MOSI_VS", COLORS["spi"], e(pins["spi_mosi"]), (IDX_VS1053, VS1053["mosi"]), "spi_mosi")
    if board.include_tft:
        w("GPIO18_SCK_TFT", COLORS["spi"], e(pins["spi_sck"]), (IDX_TFT, TFT["sck"]), "spi_sck")
        w("GPIO19_MISO_TFT", COLORS["spi"], e(pins["spi_miso"]), (IDX_TFT, TFT["miso"]), "spi_miso")
        w("GPIO23_MOSI_TFT", COLORS["spi"], e(pins["spi_mosi"]), (IDX_TFT, TFT["mosi"]), "spi_mosi")

    # --- SD: SPI CS on VS1053 breakout (WROOM) or onboard SD_MMC (CAM, no wire) ---
    if board.include_spi_sd:
        w("GPIO21_SDCS", COLORS["sd"], e(pins["sd_cs"]), (IDX_VS1053, VS1053["sdcs"]), "sd_cs")

    # --- TFT ---
    if board.include_tft:
        w("GPIO15_TFT_CS", COLORS["tft"], e(pins["tft_cs"]), (IDX_TFT, TFT["cs"]), "tft_cs")
        w("GPIO2_TFT_DC", COLORS["tft"], e(pins["tft_dc"]), (IDX_TFT, TFT["dc"]), "tft_dc")
        w("GPIO33_TFT_BL", COLORS["tft"], e(pins["tft_bl"]), (IDX_TFT, TFT["bl"]), "tft_bl")

    # --- Rotary encoder KY-040 ---
    w("GPIO25_ENC_CLK", COLORS["enc"], e(pins["enc_clk"]), (IDX_ENC, ENC["clk"]), "enc_clk")
    w("GPIO26_ENC_DT", COLORS["enc"], e(pins["enc_dt"]), (IDX_ENC, ENC["dt"]), "enc_dt")
    w("GPIO27_ENC_SW", COLORS["enc"], e(pins["enc_sw"]), (IDX_SW, SW["sig"]), "enc_sw")


def build_fz(board: BoardConfig) -> Element:
    root = Element("module", {"fritzingVersion": "1.0.3", "icon": ".png"})

    boards = SubElement(root, "boards")
    SubElement(
        boards,
        "board",
        {
            "moduleId": "TwoLayerRectanglePCBModuleID",
            "title": "Rectangular PCB - Resizable",
            "instance": "PCB1",
            "width": "20cm",
            "height": "15cm",
        },
    )

    views = SubElement(root, "views")
    for name, extra in (
        ("breadboardView", {"colorWiresByLength": "0"}),
        ("schematicView", {}),
        ("pcbView", {"autorouteTraceWidth": "24"}),
    ):
        attrs = {
            "name": name,
            "backgroundColor": "#ffffff" if name != "pcbView" else "#333333",
            "gridSize": "0.1in",
            "showGrid": "1",
            "alignToGrid": "1",
            "viewFromBelow": "0",
        }
        attrs.update(extra)
        SubElement(views, "view", attrs)

    instances = SubElement(root, "instances")
    parts: dict[int, PartViews] = {}

    pcb_inst = SubElement(
        instances,
        "instance",
        {
            "moduleIdRef": "TwoLayerRectanglePCBModuleID",
            "modelIndex": str(IDX_PCB),
            "path": ":/resources/parts/core/rectangle_pcb_two_layers.fzp",
        },
    )
    SubElement(pcb_inst, "property", {"name": "layers", "value": "2"})
    SubElement(pcb_inst, "property", {"name": "width", "value": "200"})
    SubElement(pcb_inst, "property", {"name": "height", "value": "150"})
    SubElement(pcb_inst, "title").text = "PCB1"
    pcb_views = SubElement(pcb_inst, "views")
    pcb_v = SubElement(pcb_views, "pcbView", {"layer": "board"})
    add_geometry(pcb_v, 50, 8, 1.5)

    bb_inst = SubElement(
        instances,
        "instance",
        {
            "moduleIdRef": "Breadboard-RSR03MB102-ModuleID",
            "modelIndex": str(IDX_BB),
            "path": ":/resources/parts/core/breadboard2.fzp",
        },
    )
    SubElement(bb_inst, "title").text = "Breadboard1"
    bb_views = SubElement(bb_inst, "views")
    bb_v = SubElement(bb_views, "breadboardView", {"layer": "breadboardbreadboard"})
    add_geometry(bb_v, -10, 20, 1.5)

    make_part_instance(
        instances,
        parts,
        board.module_id,
        IDX_MCU,
        board.fzp_path,
        board.part_title,
        board.bread_xy,
        board.sch_xy,
        board.pcb_xy,
    )
    # Schematic: MCU left; VS top-right; TFT bottom-right; encoder below.
    # VS SPI pins face right, so routes use a stub column to the right of VS.
    if board.variant == VARIANT_CAM:
        # VS on the left so its right-edge SPI pins face CAM left-edge SPI pins.
        vs_bread, vs_sch, vs_pcb = (30, 25), (20, -205), (40, 10)
        enc_bread, enc_sch, enc_pcb = (280, 260), (280, 40), (220, 160)
        sw_bread, sw_sch, sw_pcb = (400, 260), (400, 40), (300, 160)
    else:
        vs_bread, vs_sch, vs_pcb = (250, 25), (200, -250), (260, 10)
        enc_bread, enc_sch, enc_pcb = (40, 310), (50, 90), (40, 190)
        sw_bread, sw_sch, sw_pcb = (160, 310), (150, 90), (120, 190)

    make_part_instance(
        instances,
        parts,
        "adafruit_0c04540bc18ecb11ce3a6133b298df64_1",
        IDX_VS1053,
        "part.adafruit_0c04540bc18ecb11ce3a6133b298df64_1.fzp",
        "VS1053",
        vs_bread,
        vs_sch,
        vs_pcb,
    )
    if board.include_tft:
        make_part_instance(
            instances,
            parts,
            "adafruit_e371fbe9722138e9f73efa6ef50567a6_1",
            IDX_TFT,
            "part.adafruit_e371fbe9722138e9f73efa6ef50567a6_1.fzp",
            "TFT_ST7735",
            (300, 300),
            (220, 20),
            (280, 120),
        )
    make_part_instance(
        instances,
        parts,
        "alps_stec12e07_fix",
        IDX_ENC,
        ":/resources/parts/core/Rotary_Encoder_fix.fzp",
        "Encoder",
        enc_bread,
        enc_sch,
        enc_pcb,
    )
    make_part_instance(
        instances,
        parts,
        "SparkFun-Electromechanical-SWITCH-MOMENTARY-2-12MM",
        IDX_SW,
        ":/resources/parts/core/sparkfun-electromechanical-switch-momentary-2-12mm.fzp",
        "Enc_SW",
        sw_bread,
        sw_sch,
        sw_pcb,
    )

    build_connections(board, parts, instances)
    apply_part_connectors(parts)

    return root


def copy_embedded_parts(build_dir: Path) -> None:
    for file_path in PARTS_SRC.iterdir():
        if file_path.is_file():
            shutil.copy2(file_path, build_dir / file_path.name)


def package_fzz(build_dir: Path, output_path: Path) -> None:
    if output_path.exists():
        output_path.unlink()
    with zipfile.ZipFile(output_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for file_path in sorted(build_dir.iterdir()):
            if file_path.is_file():
                archive.write(file_path, arcname=file_path.name)


def generate_variant(board: BoardConfig) -> Path:
    """Build one .fzz for the given board profile and return its path."""
    variant_build = BUILD_DIR / board.variant
    if variant_build.exists():
        shutil.rmtree(variant_build)
    variant_build.mkdir(parents=True)

    copy_embedded_parts(variant_build)
    fz_name = f"circuit_diagram_{board.variant}.fz"
    (variant_build / fz_name).write_text(prettify_xml(build_fz(board)), encoding="utf-8")

    output_path = SCRIPT_DIR / f"circuit_diagram_{board.variant}.fzz"
    package_fzz(variant_build, output_path)
    return output_path


def verify_fzz(path: Path) -> None:
    """Ensure the archive looks like a valid Fritzing sketch."""
    with zipfile.ZipFile(path, "r") as archive:
        names = archive.namelist()
        fz_files = [n for n in names if n.endswith(".fz")]
        if not fz_files:
            raise SystemExit(f"{path.name}: missing .fz sketch in archive")
        if not any(n.startswith("part.") for n in names):
            raise SystemExit(f"{path.name}: missing embedded part.fzp files")
        with archive.open(fz_files[0]) as handle:
            root = xml_parse(handle).getroot()
        instances = root.findall(".//instance")
        wires = [i for i in instances if i.get("moduleIdRef") == "WireModuleID"]
        # Count non-orthogonal schematic segments (should be near zero).
        diag = 0
        for w in wires:
            sch = w.find("./views/schematicView/geometry")
            if sch is None:
                continue
            x2 = abs(float(sch.get("x2", 0)))
            y2 = abs(float(sch.get("y2", 0)))
            if x2 > COORD_EPS and y2 > COORD_EPS:
                diag += 1
        print(
            f"  verify {path.name}: {len(names)} files, {len(instances)} instances, "
            f"{len(wires)} wire segments, {diag} diagonal schematic segments"
        )
        if diag:
            print(f"  warning: {diag} non-orthogonal schematic segments")


def try_svg_export(fzz_paths: list[Path]) -> list[Path]:
    """
    Attempt Fritzing headless SVG export.

    Returns exported file paths (may be empty if Fritzing is unavailable).
    """
    export_dir = SCRIPT_DIR / "_export_svg"
    if export_dir.exists():
        shutil.rmtree(export_dir)
    export_dir.mkdir(parents=True)
    for path in fzz_paths:
        shutil.copy2(path, export_dir / path.name)

    cmd = [
        "flatpak",
        "run",
        "org.fritzing.Fritzing",
        "-svg",
        str(export_dir),
    ]
    print("Trying Fritzing SVG export:", " ".join(cmd))
    try:
        import subprocess

        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=120,
            check=False,
        )
        if result.returncode != 0:
            print(f"  SVG export skipped (exit {result.returncode})")
            if result.stderr:
                print(" ", result.stderr.strip()[:300])
            return []
    except (FileNotFoundError, subprocess.TimeoutExpired) as exc:
        print(f"  SVG export skipped: {exc}")
        return []

    exported = sorted(export_dir.glob("*.svg"))
    for svg in exported:
        dest = SCRIPT_DIR / svg.name
        shutil.copy2(svg, dest)
        print(f"  exported SVG: {dest.name}")
    return exported


def _svg_to_png_chromium(svg_path: Path, png_path: Path, scale: float = 1.25) -> bool:
    """Rasterize an SVG via headless Chromium (more reliable than ImageMagick on Fritzing SVGs)."""
    import re
    import subprocess
    import tempfile

    svg_text = svg_path.read_text(encoding="utf-8", errors="replace")
    vb = re.search(r'viewBox=["\']([^"\']+)["\']', svg_text)
    width_px, height_px = 1400, 1000
    if vb:
        parts = [float(x) for x in vb.group(1).replace(",", " ").split()]
        if len(parts) == 4 and parts[2] > 0 and parts[3] > 0:
            # Fritzing viewBox is often in mils-like units; keep a readable pixel size.
            width_px = max(900, min(3200, int(parts[2] * scale / 3.5)))
            height_px = max(700, min(3600, int(parts[3] * scale / 3.5)))

    html = f"""<!DOCTYPE html>
<html><head><meta charset="utf-8">
<style>
  html, body {{ margin: 0; padding: 12px; background: #ffffff; overflow: hidden; }}
  svg, img {{ display: block; width: {width_px - 24}px; height: auto; }}
</style></head>
<body>{svg_text}</body></html>
"""
    with tempfile.TemporaryDirectory(prefix="fritzing_png_") as tmp:
        html_path = Path(tmp) / "preview.html"
        html_path.write_text(html, encoding="utf-8")
        out_png = Path(tmp) / "out.png"
        cmd = [
            "chromium",
            "--headless=new",
            "--disable-gpu",
            "--no-sandbox",
            "--hide-scrollbars",
            f"--window-size={width_px},{height_px}",
            f"--screenshot={out_png}",
            html_path.resolve().as_uri(),
        ]
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=90, check=False)
        if result.returncode != 0 or not out_png.is_file() or out_png.stat().st_size < 1000:
            return False
        shutil.copy2(out_png, png_path)
        return True


def _svg_to_png_imagemagick(svg_path: Path, png_path: Path, density: int) -> bool:
    """Fallback SVG rasterization via ImageMagick convert."""
    import subprocess

    result = subprocess.run(
        ["convert", "-background", "white", "-density", str(density), str(svg_path), str(png_path)],
        capture_output=True,
        text=True,
        check=False,
    )
    return result.returncode == 0 and png_path.is_file() and png_path.stat().st_size > 1000


def export_pngs() -> None:
    """Convert exported SVGs to PNG. Schematic is the primary preview image."""
    for svg in sorted(SCRIPT_DIR.glob("circuit_diagram_*_schematic.svg")):
        stem = svg.name.replace("_schematic.svg", "")
        png = SCRIPT_DIR / f"{stem}.png"
        ok = _svg_to_png_chromium(svg, png, scale=1.4) or _svg_to_png_imagemagick(svg, png, 160)
        if ok:
            print(f"Created PNG (schematic): {png.name}")
            if stem.endswith(VARIANT_WROOM):
                shutil.copy2(png, SCRIPT_DIR / "circuit_diagram.png")
        else:
            print(f"  PNG failed for {svg.name}")

    for svg in sorted(SCRIPT_DIR.glob("circuit_diagram_*_breadboard.svg")):
        stem = svg.name.replace("_breadboard.svg", "")
        png = SCRIPT_DIR / f"{stem}_breadboard.png"
        ok = _svg_to_png_chromium(svg, png, scale=1.2) or _svg_to_png_imagemagick(svg, png, 140)
        if ok:
            print(f"Created PNG (breadboard): {png.name}")
        else:
            print(f"  PNG failed for {svg.name}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate ESP32-Webradio Fritzing diagrams.")
    parser.add_argument(
        "variant",
        nargs="?",
        default="all",
        choices=["all", VARIANT_WROOM, VARIANT_CAM],
        help="Which diagram to generate (default: all)",
    )
    parser.add_argument(
        "--svg-export",
        action="store_true",
        help="Deprecated: SVG export via Flatpak Fritzing is always attempted",
    )
    parser.add_argument(
        "--skip-export",
        action="store_true",
        help="Skip Fritzing SVG export and PNG conversion",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    if not PARTS_SRC.is_dir() or not any(PARTS_SRC.iterdir()):
        raise SystemExit(f"Missing Fritzing parts in {PARTS_SRC}")

    cam_part = PARTS_SRC / "part.ESP32-CAM-AIThinker.fzp"
    if not cam_part.is_file():
        raise SystemExit(f"Missing CAM part: {cam_part}")

    if BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)
    BUILD_DIR.mkdir(parents=True)

    selected = list(BOARDS.values()) if args.variant == "all" else [BOARDS[args.variant]]
    outputs: list[Path] = []

    for board in selected:
        out = generate_variant(board)
        verify_fzz(out)
        outputs.append(out)
        print(f"Created: {out}")
        print(f"  profile: {board.variant} ({board.title})")
        print("  GPIO wiring from hardware_profile.h:")
        for key, gpio in board.pins.items():
            print(f"    {key}: GPIO{gpio}")
        if board.variant == VARIANT_CAM:
            print("  SD: onboard SD_MMC 1-bit (no external CS wire to VS1053)")
            print("  TFT: not used")

    # Keep legacy alias for the WROOM diagram
    wroom_out = SCRIPT_DIR / f"circuit_diagram_{VARIANT_WROOM}.fzz"
    if wroom_out.is_file():
        alias = SCRIPT_DIR / "circuit_diagram.fzz"
        shutil.copy2(wroom_out, alias)
        print(f"Alias: {alias} -> {wroom_out.name}")

    if args.skip_export:
        print("Skipping SVG/PNG export (--skip-export)")
        return

    exported = try_svg_export(outputs)
    if not exported and args.svg_export:
        print("Note: --svg-export requested but Fritzing SVG export produced no files.")
    try:
        export_pngs()
    except FileNotFoundError:
        print("ImageMagick convert not found; PNG export skipped")


if __name__ == "__main__":
    main()
