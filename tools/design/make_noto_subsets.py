#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Cut the Noto Sans SC, JP and Hebrew files of assets/fonts from the upstream variable fonts.

    python3 tools/design/make_noto_subsets.py --upstream DIR --out assets/fonts
    python3 tools/design/make_noto_subsets.py --upstream DIR --report    # sizes and glyph counts only

DIR holds the three upstream files, each a variable font from google/fonts (SIL Open Font License 1.1),
under these names:

    NotoSansSC.ttf       ofl/notosanssc/NotoSansSC[wght].ttf        (Adobe's Source Han Sans, as Noto Sans SC)
    NotoSansJP.ttf       ofl/notosansjp/NotoSansJP[wght].ttf
    NotoSansHebrew.ttf   ofl/notosanshebrew/NotoSansHebrew[wdth,wght].ttf

Each is cut to the characters the interface can show in its language (DESIGN.md, section 12.4): every
character of the translations of that language's catalogue in apps/app/translations/, the languages' own
names that ParentWording.qml shows in every language, and printable ASCII, the no-break space and the
punctuation of the catalogues. The words on the board are English and are drawn in Andika, so the
catalogues are the only CJK and Hebrew text there is. Hebrew also keeps the whole Hebrew block and its
presentation forms, since its file is small and Yiddish needs the ligatures (U+05F0..05F2).

Two static weights are made of each: Regular (400) and Bold (700), the same family with the weight
class of its instance, as make_fonts.py does for Pixelify Sans. The interface asks Qt for Medium and
Bold; Qt takes the nearest of the two.

Rerun it, and commit the three pairs of files, whenever a catalogue gains a character the files lack
(the test "the bundled faces carry every character of every catalogue" says so). Needs fontTools
(pip install fonttools). The OFL texts are the upstream OFL.txt files, kept beside the faces.

The Source Han Sans files name "Source" as a Reserved Font Name in their copyright line. The family
here stays "Noto Sans SC" / "Noto Sans JP", which is the name upstream publishes it under and not one that
uses the reserved name, so the subset is a Modified Version with the original family name, as the OFL
allows for a name that is not reserved.
"""

from __future__ import annotations

import argparse
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

from fontTools import subset
from fontTools.ttLib import TTFont
from fontTools.varLib import instancer

ROOT = Path(__file__).resolve().parent.parent.parent
TRANSLATIONS = ROOT / "apps" / "app" / "translations"
WORDING = ROOT / "apps" / "app" / "qml" / "parent" / "ParentWording.qml"

# family file stem, upstream file, catalogue languages, axes beyond weight that are pinned
FONTS = (
    ("NotoSansSC", "NotoSansSC.ttf", ("zh_CN",), {}),
    ("NotoSansJP", "NotoSansJP.ttf", ("ja",), {}),
    ("NotoSansHebrew", "NotoSansHebrew.ttf", ("he", "yi"), {"wdth": 100}),
)
WEIGHTS = ((400, "Regular", False), (700, "Bold", True))
# Typefaces the interface tells apart by this block.
HEBREW_BLOCKS = ((0x0590, 0x05FF), (0xFB1D, 0xFB4F))


def catalogue_text(code: str) -> str:
    """Every translation of apps/app/translations/wjs_<code>.ts, as one string."""
    root = ET.parse(TRANSLATIONS / f"wjs_{code}.ts").getroot()
    parts: list[str] = []
    for translation in root.iter("translation"):
        if translation.text:
            parts.append(translation.text)
        for form in translation.iter("numerusform"):
            if form.text:
                parts.append(form.text)
    return "".join(parts)


def language_names() -> str:
    """The labels of ParentWording.languageChoices: each language in its own script, shown in all."""
    text = WORDING.read_text(encoding="utf-8")
    block = text[text.index("languageChoices"):]
    block = block[: block.index("]")]
    return "".join(re.findall(r'label: "([^"]+)"', block))


def characters(codes: tuple[str, ...], hebrew: bool) -> set[int]:
    text = "".join(catalogue_text(code) for code in codes) + language_names()
    wanted = {ord(c) for c in text}
    wanted |= set(range(0x20, 0x7F))
    wanted.add(0xA0)
    if hebrew:
        for low, high in HEBREW_BLOCKS:
            wanted |= set(range(low, high + 1))
    return wanted


def make(upstream: Path, name: str, source: str, codes: tuple[str, ...], pins: dict[str, int], out: Path | None) -> int:
    options = subset.Options()
    options.layout_features = ["*"]
    options.name_IDs = ["*"]
    options.name_languages = ["*"]
    options.notdef_outline = True
    options.glyph_names = False
    options.hinting = False
    options.drop_tables += ["DSIG"]
    options.legacy_kern = False
    font = TTFont(upstream / source)
    unicodes = characters(codes, name == "NotoSansHebrew")
    subsetter = subset.Subsetter(options)
    subsetter.populate(unicodes=unicodes)
    subsetter.subset(font)
    cmap = font.getBestCmap()
    kept = len(cmap)
    total = 0
    for weight, style, bold in WEIGHTS:
        # Hebrew's width axis is pinned at normal; every font's weight is pinned at the instance's.
        location = {"wght": weight, **pins}
        instance = instancer.instantiateVariableFont(font, location, inplace=False, updateFontNames=True)
        for record in instance["name"].names:
            if record.nameID == 2:
                record.string = "Bold" if bold else "Regular"
            elif record.nameID == 17:
                record.string = style
        instance["OS/2"].usWeightClass = weight
        if bold:
            instance["OS/2"].fsSelection = (instance["OS/2"].fsSelection | 0x20) & ~0x40
            instance["head"].macStyle |= 1
        else:
            instance["OS/2"].fsSelection = (instance["OS/2"].fsSelection | 0x40) & ~0x20
            instance["head"].macStyle &= ~1
        target = (out or Path("/dev/null")) / f"{name}-{style}.ttf"
        if out is not None:
            instance.save(target)
            size = target.stat().st_size
        else:
            import io

            buffer = io.BytesIO()
            instance.save(buffer)
            size = buffer.tell()
        total += size
        print(f"{name}-{style}.ttf: {size:>9,} bytes, {kept} characters" + (f", wrote {target}" if out else ""))
    return total


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--upstream", required=True, type=Path, help="the directory holding the three upstream files")
    group = ap.add_mutually_exclusive_group(required=True)
    group.add_argument("--out", type=Path, help="the directory to write <Family>-Regular.ttf and -Bold.ttf into")
    group.add_argument("--report", action="store_true", help="print the sizes and write nothing")
    args = ap.parse_args()
    if args.out is not None:
        args.out.mkdir(parents=True, exist_ok=True)
    total = 0
    for name, source, codes, pins in FONTS:
        if not (args.upstream / source).exists():
            print(f"{args.upstream / source} is missing", file=sys.stderr)
            return 1
        total += make(args.upstream, name, source, codes, pins, args.out)
    print(f"total {total:,} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
