#!/usr/bin/env python3
"""
Embed HTML partials into PROGMEM page headers before PlatformIO builds.

- nav_menu.html is the editable navigation source (pure HTML).
- Each *_page.h contains @include_nav@ inside its R\"=====(...)=====\" block.
- pre-build: placeholder -> nav HTML (for compilation)
- post-build: nav HTML -> placeholder (keeps git sources editable)
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PARTIALS_DIR = ROOT / "src" / "web_assets" / "partials"
PAGES_DIR = ROOT / "src" / "web_assets" / "pages"

NAV_PLACEHOLDER = "@include_nav@"
NAV_HTML = PARTIALS_DIR / "nav_menu.html"


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


def stitch_nav_into_pages(nav_html: str) -> None:
    """Replace @include_nav@ in all page headers with the navigation HTML."""
    for page_path in sorted(PAGES_DIR.glob("*_page.h")):
        text = page_path.read_text(encoding="utf-8")
        if NAV_PLACEHOLDER not in text:
            continue
        page_path.write_text(text.replace(NAV_PLACEHOLDER, nav_html), encoding="utf-8")
        print(f"embed_web_assets: stitched nav into {page_path.relative_to(ROOT)}")


def restore_nav_placeholders(nav_html: str) -> None:
    """Restore @include_nav@ placeholders after a build."""
    for page_path in sorted(PAGES_DIR.glob("*_page.h")):
        text = page_path.read_text(encoding="utf-8")
        if nav_html not in text:
            continue
        page_path.write_text(text.replace(nav_html, NAV_PLACEHOLDER), encoding="utf-8")
        print(f"embed_web_assets: restored placeholder in {page_path.relative_to(ROOT)}")


def main() -> None:
    command = sys.argv[1] if len(sys.argv) > 1 else "stitch"
    nav_html = load_nav_html()

    if command == "stitch":
        stitch_nav_into_pages(nav_html)
    elif command == "restore":
        restore_nav_placeholders(nav_html)
    else:
        raise SystemExit(f"Unknown command: {command}")


if __name__ == "__main__":
    main()
