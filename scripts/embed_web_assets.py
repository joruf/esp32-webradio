#!/usr/bin/env python3
"""
Generate PROGMEM web asset headers before PlatformIO builds.

- nav_menu.html is the editable navigation source (pure HTML).
- scripts/embed_web_assets.py writes partials/nav_menu.h (single PROGMEM copy).
- Page headers in src/web_assets/pages/ keep @include_nav@; the HTTP handler
  injects web_nav_menu_html at serve time (no duplicated nav in flash).
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PARTIALS_DIR = ROOT / "src" / "web_assets" / "partials"
PAGES_DIR = ROOT / "src" / "web_assets" / "pages"
NAV_HTML = PARTIALS_DIR / "nav_menu.html"
NAV_HEADER = PARTIALS_DIR / "nav_menu.h"
NAV_PLACEHOLDER = "@include_nav@"


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
        "// Auto-generated from partials/nav_menu.html — do not edit manually.\n"
        "#ifndef WEB_NAV_MENU_H\n"
        "#define WEB_NAV_MENU_H\n"
        "\n"
        f'const char web_nav_menu_html[] PROGMEM = R"=====({nav_html})=====";\n'
        "\n"
        "#endif\n"
    )
    NAV_HEADER.write_text(header, encoding="utf-8")
    print(f"embed_web_assets: generated {NAV_HEADER.relative_to(ROOT)}")


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
        print(
            "embed_web_assets: pages keep @include_nav@; firmware expands it at HTTP serve time."
        )
    else:
        raise SystemExit(f"Unknown command: {command}")


if __name__ == "__main__":
    main()
