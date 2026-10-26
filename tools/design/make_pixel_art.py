#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Draw the pixel icons and gems of Toybox Pixel into apps/app/qml/kit/PixelArt.js.

    python3 tools/design/make_pixel_art.py            # write the file
    python3 tools/design/make_pixel_art.py --show     # print the pictures as text, write nothing
    python3 tools/design/make_pixel_art.py --check    # fail if the file is not what this script writes

The art is drawn in code (docs/design/toybox/DESIGN.md, section 16: no sprite files). Each picture is a
small grid of characters. Icons are one colour, '#' on '.'. Gems have an ink outline 'o', a fill 'f', a
highlight 'h' at the top left and a shade 's' at the bottom right (the segment bevel of Snake), so the
colour of a gem is only its fill and its shape says its tier: a diamond for one star, a cut gem for two
and a star for five.
"""

from __future__ import annotations

import argparse
import math
import sys
from pathlib import Path

TARGET = Path(__file__).resolve().parent.parent.parent / "apps" / "app" / "qml" / "kit" / "PixelArt.js"


class Grid:
    def __init__(self, size: int):
        self.n = size
        self.c = [["." for _ in range(size)] for _ in range(size)]

    def put(self, x: int, y: int, ch: str = "#") -> None:
        if 0 <= x < self.n and 0 <= y < self.n:
            self.c[y][x] = ch

    def line(self, x0: int, y0: int, x1: int, y1: int, thick: bool = True) -> None:
        dx, dy = abs(x1 - x0), -abs(y1 - y0)
        sx, sy = (1 if x0 < x1 else -1), (1 if y0 < y1 else -1)
        err = dx + dy
        while True:
            self.put(x0, y0)
            if thick:
                # Two cells wide where the line is steep, two tall where it is shallow: a 2 px stroke.
                self.put(x0 + 1, y0) if abs(x1 - x0) <= abs(y1 - y0) else self.put(x0, y0 + 1)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 >= dy:
                err += dy
                x0 += sx
            if e2 <= dx:
                err += dx
                y0 += sy

    def rect(self, x: int, y: int, w: int, h: int, fill: bool = False) -> None:
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                if fill or yy in (y, y + h - 1) or xx in (x, x + w - 1):
                    self.put(xx, yy)

    def disc(self, cx: float, cy: float, r: float, ring: bool = False, width: float = 1.0) -> None:
        for y in range(self.n):
            for x in range(self.n):
                d = math.hypot(x + 0.5 - cx, y + 0.5 - cy)
                if (not ring and d <= r) or (ring and r - width < d <= r):
                    self.put(x, y)

    def rows(self) -> list[str]:
        return ["".join(r) for r in self.c]


# --- icons: 12 x 12, one colour ----------------------------------------------------------------------

def icon_back() -> Grid:
    g = Grid(12)
    g.line(8, 1, 3, 6)
    g.line(3, 6, 8, 11)
    return g


def icon_forward() -> Grid:
    g = Grid(12)
    g.line(3, 1, 8, 6)
    g.line(8, 6, 3, 11)
    return g


def icon_close() -> Grid:
    g = Grid(12)
    for y, row in enumerate((
        "............",
        ".##......##.",
        "..##....##..",
        "...##..##...",
        "....####....",
        ".....##.....",
        ".....##.....",
        "....####....",
        "...##..##...",
        "..##....##..",
        ".##......##.",
        "............",
    )):
        for x, ch in enumerate(row):
            if ch == "#":
                g.put(x, y)
    return g


def icon_done() -> Grid:
    g = Grid(12)
    g.line(1, 6, 4, 9)
    g.line(4, 9, 10, 3)
    return g


def icon_parent() -> Grid:
    g = Grid(12)
    g.disc(6, 3.6, 2.7)
    # shoulders: a half-ellipse, flat at the bottom
    for y in range(7, 12):
        for x in range(12):
            if ((x + 0.5 - 6) / 5.6) ** 2 + ((y + 0.5 - 12) / 5.2) ** 2 <= 1:
                g.put(x, y)
    return g


def icon_history() -> Grid:
    g = Grid(12)
    for x, y in ((0, 0), (7, 0), (0, 7), (7, 7)):
        g.rect(x, y, 5, 5)
    return g


def icon_jewel() -> Grid:
    g = Grid(12)
    pts = [(3, 1), (8, 1), (11, 4), (6, 11), (1, 4)]
    for a, b in zip(pts, pts[1:] + pts[:1]):
        g.line(*a, *b, thick=False)
    g.line(1, 4, 11, 4, thick=False)
    g.line(4, 4, 6, 11, thick=False)
    g.line(8, 4, 6, 11, thick=False)
    return g


def icon_hint() -> Grid:
    g = Grid(12)
    g.disc(6, 6, 5.5, ring=True, width=1.6)
    g.rect(5, 5, 2, 2, fill=True)
    return g


def icon_again() -> Grid:
    g = Grid(12)
    for deg in range(35, 330, 4):
        a = math.radians(deg)
        g.put(int(6 + 4.3 * math.cos(a)), int(6 - 4.3 * math.sin(a)))
        g.put(int(6 + 3.5 * math.cos(a)), int(6 - 3.5 * math.sin(a)))
    # the arrow head at the end of the arc, pointing along it
    for dx, dy in ((0, 0), (1, 0), (2, 0), (0, 1), (1, 1), (0, 2)):
        g.put(8 + dx, 0 + dy)
    return g


def icon_shake() -> Grid:
    g = Grid(12)
    g.rect(2, 2, 8, 8)
    for x, y in ((4, 4), (7, 4), (4, 7), (7, 7)):
        g.put(x, y)
    g.line(0, 4, 0, 7, thick=False)
    g.line(11, 4, 11, 7, thick=False)
    return g


ICONS = {
    "back": icon_back, "forward": icon_forward, "close": icon_close, "done": icon_done,
    "parent": icon_parent, "history": icon_history, "jewel": icon_jewel, "hint": icon_hint,
    "again": icon_again, "shake": icon_shake,
}


# --- gems: hand drawn; o the ink outline, f the fill, h the highlight, s the shade -------------------------

GEMS = {
    # one star: a diamond
    "spark": [
        "....o....",
        "...oho...",
        "..ohffo..",
        ".ohfffso.",
        "ohfffffso",
        ".osfffso.",
        "..osfso..",
        "...oso...",
        "....o....",
    ],
    # two stars: a cut gem
    "power": [
        "..ooooooo..",
        ".ohhffffso.",
        "ohhfffffsso",
        "ooooooooooo",
        ".ohfffffso.",
        "..ohfffso..",
        "...ohfso...",
        "....oso....",
        ".....o.....",
    ],
    # five stars: a star
    "mega": [
        "......o......",
        ".....oho.....",
        "....ohfso....",
        "ooooohfsooooo",
        "ohhfffffffsso",
        ".ohhfffffsso.",
        "..ohhfffsso..",
        "..ohfffffso..",
        ".ohfffsfffso.",
        "..ohfsosfso..",
        "..ooo...ooo..",
    ],
}

for _name, _rows in GEMS.items():
    assert len({len(r) for r in _rows}) == 1, _name


RUNTIME = """// The SVG path of the cells of `rows` whose character is one of `chars`: one rectangle per run, so a
// picture is a handful of rectangles however many cells it has.
function path(rows, chars) {
    var d = "";
    for (var y = 0; y < rows.length; ++y) {
        var x = 0;
        while (x < rows[y].length) {
            if (chars.indexOf(rows[y][x]) < 0) {
                ++x;
                continue;
            }
            var start = x;
            while (x < rows[y].length && chars.indexOf(rows[y][x]) >= 0) {
                ++x;
            }
            d += "M" + start + " " + y + "h" + (x - start) + "v1h-" + (x - start) + "z";
        }
    }
    return d;
}
""".split("\n")


def js() -> str:
    lines = [
        "// SPDX-License-Identifier: GPL-3.0-or-later",
        ".pragma library",
        "",
        "// The pixel icons and gems of Toybox Pixel. Generated by tools/design/make_pixel_art.py: edit that,",
        "// not this. Icons are one colour ('#' on '.'). Gems: o the ink outline, f the fill, h the highlight",
        "// at the top left, s the shade at the bottom right; '.' is clear.",
        "",
        "var icons = {",
    ]
    names = list(ICONS)
    for i, name in enumerate(names):
        rows = ICONS[name]().rows()
        lines.append(f'    {name}: [')
        lines += [f'        "{r}"' + ("," if k < len(rows) - 1 else "") for k, r in enumerate(rows)]
        lines.append("    ]" + ("," if i < len(names) - 1 else ""))
    lines += ["};", "", "var gems = {"]
    names = list(GEMS)
    for i, name in enumerate(names):
        rows = GEMS[name]
        lines.append(f'    {name}: [')
        lines += [f'        "{r}"' + ("," if k < len(rows) - 1 else "") for k, r in enumerate(rows)]
        lines.append("    ]" + ("," if i < len(names) - 1 else ""))
    lines += ["};", ""]
    lines += RUNTIME
    return "\n".join(lines)


def show() -> None:
    for name, make in ICONS.items():
        print(f"icon {name}")
        for r in make().rows():
            print("   " + r.replace(".", " ").replace("#", "█"))
    for name, make in GEMS.items():
        print(f"gem {name}")
        for r in make:
            print("   " + r.replace(".", " "))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--show", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    if args.show:
        show()
        return 0
    text = js()
    if args.check:
        if not TARGET.exists() or TARGET.read_text(encoding="utf-8") != text:
            print(f"{TARGET} is not what tools/design/make_pixel_art.py writes; run it", file=sys.stderr)
            return 1
        return 0
    TARGET.write_text(text, encoding="utf-8", newline="\n")
    print(f"wrote {TARGET}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
