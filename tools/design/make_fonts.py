#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Cut the static Pixelify Sans files of assets/fonts from the upstream variable font.

    python3 tools/design/make_fonts.py --variable PixelifySans[wght].ttf --out assets/fonts

Pixelify Sans is published as one variable font (weights 400 to 700). The interface uses two weights
(DESIGN.md, section 4: Medium 500 for labels, Bold 700 for titles and buttons), and Qt picks a weight by
the family name and the file's OS/2 weight class, so each is a static font of its own: the same
family, "Pixelify Sans", with the weight class of its instance.

Needs fontTools (pip install fonttools). The upstream files are google/fonts, ofl/pixelifysans and
ofl/silkscreen, under the SIL Open Font License 1.1, which names no Reserved Font Name for either, so the
instances keep the name. Silkscreen is used as published (Silkscreen-Regular.ttf).
"""

from __future__ import annotations

import argparse
from pathlib import Path

from fontTools.ttLib import TTFont
from fontTools.varLib import instancer

INSTANCES = ((500, "Medium", False), (700, "Bold", True))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--variable", required=True, type=Path, help="PixelifySans[wght].ttf")
    ap.add_argument("--out", required=True, type=Path, help="the directory to write PixelifySans-<Style>.ttf into")
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    for weight, style, bold in INSTANCES:
        font = instancer.instantiateVariableFont(
            TTFont(args.variable), {"wght": weight}, inplace=False, updateFontNames=True
        )
        # RIBBI naming: the style-linked names say Regular or Bold; the typographic names keep the style.
        for record in font["name"].names:
            if record.nameID == 2:
                record.string = "Bold" if bold else "Regular"
            elif record.nameID == 17:
                record.string = style
        font["OS/2"].usWeightClass = weight
        if bold:
            font["OS/2"].fsSelection = (font["OS/2"].fsSelection | 0x20) & ~0x40
            font["head"].macStyle |= 1
        target = args.out / f"PixelifySans-{style}.ttf"
        font.save(target)
        print(f"wrote {target} (weight {weight})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
