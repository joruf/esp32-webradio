#!/usr/bin/env python3
"""
Generate PROGMEM web asset headers before PlatformIO builds.

- nav_menu.html is the editable navigation source (pure HTML).
- scripts/embed_web_assets.py writes partials/nav_menu.h (single PROGMEM copy).
- Page headers in src/web_assets/pages/ keep @include_nav@; the HTTP handler
  injects web_nav_menu_html at serve time (no duplicated nav in flash).
- app.js.h / app.css.h are gzip-compressed into *.gz.h so the firmware fits
  the OTA partition (saves ~100 KB+ of flash vs raw PROGMEM strings).
"""

from __future__ import annotations

import gzip
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PARTIALS_DIR = ROOT / "src" / "web_assets" / "partials"
PAGES_DIR = ROOT / "src" / "web_assets" / "pages"
STYLES_DIR = ROOT / "src" / "web_assets" / "styles"
SCRIPTS_DIR = ROOT / "src" / "web_assets" / "scripts"
NAV_HTML = PARTIALS_DIR / "nav_menu.html"
NAV_HEADER = PARTIALS_DIR / "nav_menu.h"
NAV_PLACEHOLDER = "@include_nav@"

RAW_STRING_RE = re.compile(
    r'R"=====\((.*)\)====="\s*;',
    re.DOTALL,
)


def normalize_html(content: str) -> str:
    """Trim leading/trailing blank lines from the HTML source."""
    lines = content.splitlines()
    while lines and not lines[0].strip():
        lines.pop(0)
    while lines and not lines[-1].strip():
        lines.pop()
    return "\n".join(lines) + "\n"


def load_nav_html() -> str:
    """Load navigation HTML from nav_menu.html."""
    content = normalize_html(NAV_HTML.read_text(encoding="utf-8"))
    if "=====" in content:
        raise ValueError("nav_menu.html must not contain the raw-string delimiter '====='")
    return content


def generate_nav_header(nav_html: str) -> None:
    """Write nav_menu.h with a single PROGMEM navigation string."""
    header = (
        "// Auto-generated from partials/nav_menu.html - do not edit manually.\n"
        "#ifndef WEB_NAV_MENU_H\n"
        "#define WEB_NAV_MENU_H\n"
        "\n"
        f'const char web_nav_menu_html[] PROGMEM = R"=====({nav_html})=====";\n'
        "\n"
        "#endif\n"
    )
    NAV_HEADER.write_text(header, encoding="utf-8")
    print(f"embed_web_assets: generated {NAV_HEADER.relative_to(ROOT)}")


def extract_raw_string_payload(header_path: Path) -> bytes:
    """Extract the R\"=====(...)=====\" payload from a PROGMEM header."""
    text = header_path.read_text(encoding="utf-8")
    match = RAW_STRING_RE.search(text)
    if not match:
        raise ValueError(f"{header_path}: missing R\"=====(...)=====\" payload")
    # Keep exact bytes as UTF-8 (source is Latin-1 friendly ASCII/UTF-8)
    return match.group(1).encode("utf-8")


def write_gzip_header(source_header: Path, out_header: Path, symbol: str) -> None:
    """Gzip-compress a raw-string PROGMEM header into a uint8_t PROGMEM array."""
    payload = extract_raw_string_payload(source_header)
    compressed = gzip.compress(payload, compresslevel=9, mtime=0)
    guard = out_header.name.upper().replace(".", "_").replace("-", "_")
    lines = [
        f"// Auto-generated gzip of {source_header.name} - do not edit manually.",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "#include <Arduino.h>",
        "",
        f"const unsigned int {symbol}_len = {len(compressed)};",
        f"const uint8_t {symbol}[] PROGMEM = {{",
    ]
    row: list[str] = []
    for i, byte in enumerate(compressed):
        row.append(f"0x{byte:02x}")
        if len(row) == 16:
            lines.append("  " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("  " + ", ".join(row) + ",")
    lines.extend(
        [
            "};",
            "",
            "#endif",
            "",
        ]
    )
    out_header.write_text("\n".join(lines), encoding="utf-8")
    ratio = (100.0 * len(compressed) / len(payload)) if payload else 0.0
    print(
        f"embed_web_assets: {source_header.name} {len(payload)} -> "
        f"{out_header.name} {len(compressed)} bytes ({ratio:.1f}%)"
    )


def generate_gzip_assets() -> None:
    """Compress large CSS/JS PROGMEM assets for flash savings."""
    write_gzip_header(
        SCRIPTS_DIR / "app.js.h",
        SCRIPTS_DIR / "app.js.gz.h",
        "app_js_gz",
    )
    write_gzip_header(
        STYLES_DIR / "app.css.h",
        STYLES_DIR / "app.css.gz.h",
        "app_css_gz",
    )


def verify_page_placeholders() -> None:
    """Ensure every page header still contains the nav placeholder."""
    for page_path in sorted(PAGES_DIR.glob("*_page.h")):
        text = page_path.read_text(encoding="utf-8")
        if NAV_PLACEHOLDER not in text:
            raise ValueError(
                f"{page_path.relative_to(ROOT)} is missing {NAV_PLACEHOLDER!r}"
            )


def main() -> None:
    command = sys.argv[1] if len(sys.argv) > 1 else "generate"
    nav_html = load_nav_html()

    if command == "generate":
        verify_page_placeholders()
        generate_nav_header(nav_html)
        generate_gzip_assets()
        print(
            "embed_web_assets: pages keep @include_nav@; CSS/JS served as gzip PROGMEM."
        )
    else:
        raise SystemExit(f"Unknown command: {command}")


if __name__ == "__main__":
    main()
