# SPDX-License-Identifier: GPL-3.0-or-later
"""The pixel art in apps/app/qml/kit/PixelArt.js is what tools/design/make_pixel_art.py writes."""

import importlib.util
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("make_pixel_art", HERE / "make_pixel_art.py")
ART = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ART)


class PixelArtTest(unittest.TestCase):
    def test_the_file_is_what_the_script_writes(self):
        self.assertEqual(ART.TARGET.read_text(encoding="utf-8"), ART.js(),
                         "run python3 tools/design/make_pixel_art.py")

    def test_every_icon_is_twelve_cells_square_and_not_blank(self):
        for name, make in ART.ICONS.items():
            rows = make().rows()
            self.assertEqual({len(r) for r in rows}, {12}, name)
            self.assertEqual(len(rows), 12, name)
            self.assertTrue(any("#" in r for r in rows), name + " is blank")

    def test_every_gem_is_a_rectangle_with_an_outline_and_a_fill(self):
        for name, rows in ART.GEMS.items():
            self.assertEqual(len({len(r) for r in rows}), 1, name)
            text = "".join(rows)
            self.assertLessEqual(set(text), set("ofhs."), name)
            self.assertIn("o", text, name)
            self.assertIn("f", text, name)

    def test_the_tiers_grow(self):
        sizes = [sum(len(r) - r.count(".") for r in ART.GEMS[t]) for t in ("spark", "power", "mega")]
        self.assertEqual(sizes, sorted(sizes))


if __name__ == "__main__":
    unittest.main()
