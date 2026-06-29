#!/usr/bin/env python3
"""
Generate circuit_diagram.fzz for ESP32-Webradio hardware wiring.

Pin defaults are taken from src/handler/setup.h (readIOprefs / klist).
"""

from __future__ import annotations

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
OUTPUT_FZZ = SCRIPT_DIR / "circuit_diagram.fzz"

FRITZING_CORE = Path(
    "/var/lib/flatpak/app/org.fritzing.Fritzing/x86_64/stable/"
    "86f886fa2f3b217722c1f44cf1ac3bac5940cbc97cc19cd18355219bfcd91778/"
    "files/share/fritzing/parts/core"
)

# ---------------------------------------------------------------------------
# Default GPIO mapping (ini_block / klist in setup.h)
# ---------------------------------------------------------------------------
PINS = {
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

# ESP32-38PinWide.fzp: connector id by GPIO number
ESP32_GPIO = {
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
ESP32_3V3 = "connector0"
ESP32_GND = ["connector13", "connector31", "connector37"]

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
IDX_ESP32 = 100003
IDX_VS1053 = 100004
IDX_TFT = 100005
IDX_ENC = 100006
IDX_SW = 100007
IDX_WIRE = 100100


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
    terminals: dict[str, tuple[float, float]] = field(default_factory=dict)
    bb_terminals: dict[str, tuple[float, float]] = field(default_factory=dict)
    connects: dict[str, list[tuple[int, str, str, str]]] = field(
        default_factory=lambda: defaultdict(list)
    )

    def abs_terminal(self, connector_id: str) -> tuple[float, float]:
        """Absolute schematic coordinates for a connector terminal."""
        tx, ty = self.terminals[connector_id]
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


def schematic_wire_geom(
    parts: dict[int, PartViews],
    end_a: tuple[int, str],
    end_b: tuple[int, str],
) -> tuple[float, float, float, float]:
    """Compute schematic wire geometry from part pin terminal positions."""
    x1, y1 = parts[end_a[0]].abs_terminal(end_a[1])
    x2, y2 = parts[end_b[0]].abs_terminal(end_b[1])
    return x1, y1, x2 - x1, y2 - y1


def breadboard_wire_geom(
    parts: dict[int, PartViews],
    end_a: tuple[int, str],
    end_b: tuple[int, str],
) -> tuple[float, float, float, float]:
    """Compute breadboard wire geometry from part pin terminal positions."""
    x1, y1 = parts[end_a[0]].abs_bb_terminal(end_a[1])
    x2, y2 = parts[end_b[0]].abs_bb_terminal(end_b[1])
    return x1, y1, x2 - x1, y2 - y1


def prettify_xml(element: Element) -> str:
    rough = tostring(element, encoding="unicode")
    parsed = minidom.parseString(rough)
    return parsed.toprettyxml(indent="    ", encoding="UTF-8").decode("UTF-8")


def add_geometry(parent: Element, x: float, y: float, z: float) -> None:
    SubElement(parent, "geometry", {"z": f"{z:.5f}", "x": f"{x}", "y": f"{y}"})


def esp32_pin(gpio: int) -> tuple[int, str]:
    conn = ESP32_GPIO[gpio]
    return IDX_ESP32, conn


def register_connection(
    parts: dict[int, PartViews],
    wire_idx: int,
    end_a: tuple[int, str],
    end_b: tuple[int, str],
    wire_end_a: str,
    wire_end_b: str,
) -> None:
    """Register bidirectional wire endpoints on both components."""
    part_a, conn_a = end_a
    part_b, conn_b = end_b
    for part_idx, conn_id, wire_end, comp_layer, wire_layer in (
        (part_a, conn_a, wire_end_a, "breadboard", "breadboardWire"),
        (part_b, conn_b, wire_end_b, "breadboard", "breadboardWire"),
    ):
        parts[part_idx].connects[conn_id].append((wire_idx, wire_end, comp_layer, wire_layer))
    for part_idx, conn_id, wire_end, comp_layer, wire_layer in (
        (part_a, conn_a, wire_end_a, "schematic", "schematicTrace"),
        (part_b, conn_b, wire_end_b, "schematic", "schematicTrace"),
    ):
        parts[part_idx].connects[conn_id].append((wire_idx, wire_end, comp_layer, wire_layer))
    for part_idx, conn_id, wire_end, comp_layer, wire_layer in (
        (part_a, conn_a, wire_end_a, "copper0", "copper0trace"),
        (part_b, conn_b, wire_end_b, "copper0", "copper0trace"),
    ):
        parts[part_idx].connects[conn_id].append((wire_idx, wire_end, comp_layer, wire_layer))


def make_wire(
    instances: Element,
    parts: dict[int, PartViews],
    wire_idx: int,
    title: str,
    color: str,
    end_a: tuple[int, str],
    end_b: tuple[int, str],
) -> int:
    """Create wire with connects on both endpoints (component side filled later)."""
    bx, by, bx2, by2 = breadboard_wire_geom(parts, end_a, end_b)
    sx, sy, sx2, sy2 = schematic_wire_geom(parts, end_a, end_b)

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

    for view_name, layer, trace_layer, geom, mils, wire_flags in (
        ("breadboardView", "breadboardWire", "breadboard", (bx, by, bx2, by2), "22.2222", "64"),
        ("schematicView", "schematicTrace", "schematic", (sx, sy, sx2, sy2), "9.7222", "128"),
    ):
        view = SubElement(views, view_name, {"layer": layer})
        SubElement(
            view,
            "geometry",
            {
                "z": "3.5" if view_name == "breadboardView" else "5.5",
                "x": str(geom[0]),
                "y": str(geom[1]),
                "x1": "0",
                "y1": "0",
                "x2": str(geom[2]),
                "y2": str(geom[3]),
                "wireFlags": wire_flags,
            },
        )
        SubElement(view, "wireExtras", {"mils": mils, "color": color, "opacity": "1", "banded": "0"})
        conns = SubElement(view, "connectors")
        for end_idx, (model, conn) in enumerate([end_a, end_b]):
            c = SubElement(conns, "connector", {"connectorId": f"connector{end_idx}", "layer": layer})
            SubElement(c, "geometry", {"x": "0", "y": "0"})
            connects = SubElement(c, "connects")
            SubElement(
                connects,
                "connect",
                {"connectorId": conn, "modelIndex": str(model), "layer": trace_layer},
            )

    # PCB trace (bottom layer)
    pcb_view = SubElement(views, "pcbView", {"layer": "copper0trace"})
    SubElement(
        pcb_view,
        "geometry",
        {
            "z": "6.5",
            "x": str(bx),
            "y": str(by),
            "x1": "0",
            "y1": "0",
            "x2": str(bx2),
            "y2": str(by2),
            "wireFlags": "4",
        },
    )
    SubElement(pcb_view, "wireExtras", {"mils": "11.1111", "color": color, "opacity": "1", "banded": "0"})
    pcb_conns = SubElement(pcb_view, "connectors")
    for end_idx, (model, conn) in enumerate([end_a, end_b]):
        c = SubElement(pcb_conns, "connector", {"connectorId": f"connector{end_idx}", "layer": "copper0trace"})
        SubElement(c, "geometry", {"x": "0", "y": "0"})
        connects = SubElement(c, "connects")
        SubElement(
            connects,
            "connect",
            {"connectorId": conn, "modelIndex": str(model), "layer": "copper0"},
        )

    register_connection(parts, wire_idx, end_a, end_b, "connector0", "connector1")
    return wire_idx


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

    sch = SubElement(views_el, "schematicView", {"layer": "schematic"})
    add_geometry(sch, *sch_xy, 2.5)

    pcb = None
    if pcb_xy:
        pcb = SubElement(views_el, "pcbView", {"layer": "copper0"})
        add_geometry(pcb, *pcb_xy, 5.5)

    part = PartViews(model_index, bb, sch, pcb, sch_x=sch_xy[0], sch_y=sch_xy[1], bb_x=bread_xy[0], bb_y=bread_xy[1])
    fzp_local = resolve_fzp_path(path)
    if fzp_local is not None:
        part.terminals = load_part_terminals(fzp_local)
        part.bb_terminals = load_part_breadboard_terminals(fzp_local)
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
                    tx, ty = part.terminals[conn_id]
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


def build_connections(parts: dict[int, PartViews], instances: Element) -> None:
    """All signal connections matching firmware defaults."""
    wire = IDX_WIRE

    def w(title, color, a, b):
        nonlocal wire
        make_wire(instances, parts, wire, title, color, a, b)
        wire += 1

    e = esp32_pin

    # --- Power ---
    w("3V3_ESP_VS1053_A", COLORS["power"], (IDX_ESP32, ESP32_3V3), (IDX_VS1053, VS1053["3v3_a"]))
    w("3V3_VS1053_B", COLORS["power"], (IDX_VS1053, VS1053["3v3_b"]), (IDX_ESP32, ESP32_3V3))
    w("3V3_ESP_TFT", COLORS["power"], (IDX_ESP32, ESP32_3V3), (IDX_TFT, TFT["vcc"]))
    w("3V3_VS1053_RST", COLORS["power"], (IDX_ESP32, ESP32_3V3), (IDX_VS1053, VS1053["rst"]))
    w("3V3_TFT_RST", COLORS["power"], (IDX_ESP32, ESP32_3V3), (IDX_TFT, TFT["rst"]))

    w("GND_ESP_VS1053", COLORS["gnd"], (IDX_ESP32, ESP32_GND[0]), (IDX_VS1053, VS1053["gnd_a"]))
    w("GND_VS1053_B", COLORS["gnd"], (IDX_VS1053, VS1053["gnd_b"]), (IDX_ESP32, ESP32_GND[1]))
    w("GND_ESP_TFT", COLORS["gnd"], (IDX_ESP32, ESP32_GND[1]), (IDX_TFT, TFT["gnd"]))
    w("GND_ENC", COLORS["gnd"], (IDX_ENC, ENC["gnd"]), (IDX_ESP32, ESP32_GND[2]))
    w("GND_ENC_SW", COLORS["gnd"], (IDX_SW, SW["gnd"]), (IDX_ESP32, ESP32_GND[2]))

    # --- VS1053 (pin_vs_cs / pin_vs_dcs / pin_vs_dreq) ---
    w("GPIO5_VS_CS", COLORS["vs1053"], e(PINS["vs_cs"]), (IDX_VS1053, VS1053["cs"]))
    w("GPIO32_VS_DCS", COLORS["vs1053"], e(PINS["vs_dcs"]), (IDX_VS1053, VS1053["dcs"]))
    w("GPIO4_VS_DREQ", COLORS["vs1053"], e(PINS["vs_dreq"]), (IDX_VS1053, VS1053["dreq"]))

    # --- Shared SPI (pin_spi_sck / miso / mosi) ---
    w("GPIO18_SCK_VS", COLORS["spi"], e(PINS["spi_sck"]), (IDX_VS1053, VS1053["sck"]))
    w("GPIO18_SCK_TFT", COLORS["spi"], e(PINS["spi_sck"]), (IDX_TFT, TFT["sck"]))
    w("GPIO19_MISO_VS", COLORS["spi"], e(PINS["spi_miso"]), (IDX_VS1053, VS1053["miso"]))
    w("GPIO19_MISO_TFT", COLORS["spi"], e(PINS["spi_miso"]), (IDX_TFT, TFT["miso"]))
    w("GPIO23_MOSI_VS", COLORS["spi"], e(PINS["spi_mosi"]), (IDX_VS1053, VS1053["mosi"]))
    w("GPIO23_MOSI_TFT", COLORS["spi"], e(PINS["spi_mosi"]), (IDX_TFT, TFT["mosi"]))

    # --- SD card on VS1053 breakout (pin_sd_cs) ---
    w("GPIO21_SDCS", COLORS["sd"], e(PINS["sd_cs"]), (IDX_VS1053, VS1053["sdcs"]))

    # --- TFT (pin_tft_cs / pin_tft_dc / pin_tft_bl) ---
    w("GPIO15_TFT_CS", COLORS["tft"], e(PINS["tft_cs"]), (IDX_TFT, TFT["cs"]))
    w("GPIO2_TFT_DC", COLORS["tft"], e(PINS["tft_dc"]), (IDX_TFT, TFT["dc"]))
    w("GPIO33_TFT_BL", COLORS["tft"], e(PINS["tft_bl"]), (IDX_TFT, TFT["bl"]))

    # --- Rotary encoder KY-040 (pin_enc_clk / dt / sw, INPUT_PULLUP in firmware) ---
    w("GPIO25_ENC_CLK", COLORS["enc"], e(PINS["enc_clk"]), (IDX_ENC, ENC["clk"]))
    w("GPIO26_ENC_DT", COLORS["enc"], e(PINS["enc_dt"]), (IDX_ENC, ENC["dt"]))
    w("GPIO27_ENC_SW", COLORS["enc"], e(PINS["enc_sw"]), (IDX_SW, SW["sig"]))


def build_fz() -> Element:
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
        instances, parts, "ESP32-38PinWide", IDX_ESP32, "part.ESP32-38PinWide.fzp",
        "ESP32", (100, 30), (60, -140), (50, 30),
    )
    make_part_instance(
        instances, parts, "adafruit_0c04540bc18ecb11ce3a6133b298df64_1", IDX_VS1053,
        "part.adafruit_0c04540bc18ecb11ce3a6133b298df64_1.fzp",
        "VS1053", (400, 10), (400, -240), (280, 10),
    )
    make_part_instance(
        instances, parts, "adafruit_e371fbe9722138e9f73efa6ef50567a6_1", IDX_TFT,
        "part.adafruit_e371fbe9722138e9f73efa6ef50567a6_1.fzp",
        "TFT_ST7735", (400, 170), (400, -80), (280, 110),
    )
    make_part_instance(
        instances, parts, "alps_stec12e07_fix", IDX_ENC,
        ":/resources/parts/core/Rotary_Encoder_fix.fzp",
        "Encoder", (30, 250), (60, 30), (30, 190),
    )
    make_part_instance(
        instances, parts, "SparkFun-Electromechanical-SWITCH-MOMENTARY-2-12MM", IDX_SW,
        ":/resources/parts/core/sparkfun-electromechanical-switch-momentary-2-12mm.fzp",
        "Enc_SW", (180, 250), (180, 30), (110, 190),
    )

    build_connections(parts, instances)
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


def main() -> None:
    if not PARTS_SRC.is_dir() or not any(PARTS_SRC.iterdir()):
        raise SystemExit(f"Missing Fritzing parts in {PARTS_SRC}")

    if BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)
    BUILD_DIR.mkdir(parents=True)

    copy_embedded_parts(BUILD_DIR)
    (BUILD_DIR / "circuit_diagram.fz").write_text(prettify_xml(build_fz()), encoding="utf-8")
    package_fzz(BUILD_DIR, OUTPUT_FZZ)

    print(f"Created: {OUTPUT_FZZ}")
    print("GPIO wiring from setup.h defaults:")
    for key, gpio in PINS.items():
        print(f"  {key}: GPIO{gpio}")


if __name__ == "__main__":
    main()
