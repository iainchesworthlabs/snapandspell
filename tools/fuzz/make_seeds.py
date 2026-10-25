#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Write the seed inputs of the libFuzzer harnesses (docs/fuzzing.md).

    python3 tools/fuzz/make_seeds.py

The seeds are small and readable: the real files of data/, the golden text of the tests, and a few
hand-made cases at the edge of each format (a Qu tile, a missing header, a ragged row). A seed is a
starting point for the mutator, not a test; the files libFuzzer finds are not kept (docs/fuzzing.md).
Every file is written into libs/<lib>/fuzz/corpus/<target>/, and the regressions directories are
created beside them.
"""

from __future__ import annotations

import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
CORE = ROOT / "libs" / "core" / "fuzz"
SCAN = ROOT / "libs" / "scan" / "fuzz"


def write(directory: Path, name: str, data: bytes | str) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    (directory / name).write_bytes(data.encode() if isinstance(data, str) else data)


def fnv1a(text: str) -> int:
    value = 14695981039346656037
    for byte in text.encode():
        value ^= byte
        value = (value * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return value


def seal(body: str) -> str:
    """The store file's last line: the checksum of every line before it (store_format.cpp)."""
    return body + f"end {fnv1a(body):016x}\n"


def board_seeds() -> None:
    d = CORE / "corpus" / "board"
    write(d, "rows.txt", "CAT\nORE\nDGS\n")
    write(d, "spaces.txt", "S A T\nP I N\nT A P\n")
    write(d, "slashes.txt", "S A T/P I N/T A P")
    write(d, "qu.txt", "QU A T\nS I N\nT A P\n")
    write(d, "commas.txt", "S,A,T,P/I,N,S,A/T,P,I,N/A,T,S,P")
    write(d, "ragged.txt", "S A T\nP I\nT A P\n")
    write(d, "lowercase.txt", "sat\npin\ntap\n")
    write(d, "digit.txt", "S A 7\nP I N\nT A P\n")
    write(d, "empty-lines.txt", "\n\n\n")


def dictionary_seeds() -> None:
    d = CORE / "corpus" / "dictionary"
    words = (ROOT / "data" / "dictionaries" / "en.txt").read_text(encoding="utf-8").splitlines()
    write(d, "head.txt", "\n".join(words[:60]) + "\n")
    write(d, "tiny.txt", "cat\ncare\ntar\nore\n")
    write(d, "mixed.txt", "Cat\nCARE\ntar \n\n  ore\nnot a word\nx\n")
    write(d, "crlf.txt", "cat\r\ncare\r\ntar\r\n")
    write(d, "no-final-newline.txt", "cat\ncare")


def dice_seeds() -> None:
    d = CORE / "corpus" / "dice_set"
    for size in (3, 4):
        text = (ROOT / "data" / "dice" / f"{size}x{size}.txt").read_text(encoding="utf-8")
        write(d, f"{size}x{size}.txt", text)
    write(d, "two-by-two.txt", "# 2x2\nA B C D E F\nG H I J K L\nM N O P Q R\nS T U V W X\n")
    write(d, "qu-face.txt", "QU A E I O U\nT R S N L D\nC M P B F G\nH K V W Y Z\n")
    write(d, "not-square.txt", "A B C\nD E F\nG H I\n")
    write(d, "bad-face.txt", "ABCD A B C\n1 2 3\n")
    write(d, "commas.txt", "A,B,C,D,E,F\nG,H,I,J,K,L\nM,N,O,P,Q,R\nS,T,U,V,W,X\n")


GRAPHEMES = (
    "# phase: A\n"
    "a    /t1/  clip:c1\n"
    "a_e  /t2/  clip:c2\n"
    "ai   /t3/  clip:c3\n"
    "c    /t4/  clip:c4\n"
    "k    /t5/  clip:c5\n"
    "n    /t6/  clip:c6\n"
    "o    /tb/  clip:cb\n"
    "o    /tc/  clip:cc\n"
    "p    /t7/  clip:c7\n"
    "r    /t8/  clip:c8\n"
    "s    /t9/  clip:c9\n"
    "t    /ta/  clip:ca\n"
)
WORDS = (
    "# phase: A\n"
    "cake  c a_ k _e\n"
    "pot   p o:2 t\n"
    "rain  r ai n\n"
    "sat   s a t\n"
    "the   tricky\n"
)
REVIEWED = (
    "# phase: A\n# reviewed-by: A. Reviewer\n# reviewed-on: 2026-10-01\n"
    "# source: a hand-made seed for the fuzzer, not teaching data\n"
)


def phase_seeds() -> None:
    g = CORE / "corpus" / "phase_graphemes"
    write(g, "phase-a.txt", GRAPHEMES)
    write(g, "reviewed.txt", REVIEWED + GRAPHEMES.split("\n", 1)[1])
    write(g, "no-header.txt", GRAPHEMES.split("\n", 1)[1])
    write(g, "duplicate.txt", GRAPHEMES + "a    /t1/  clip:c1\n")
    write(g, "bad-sound.txt", "# phase: A\na    t1  clip:c1\n")
    w = CORE / "corpus" / "phase_words"
    write(w, "phase-a.txt", WORDS)
    write(w, "reviewed.txt", REVIEWED + WORDS.split("\n", 1)[1])
    write(w, "no-header.txt", WORDS.split("\n", 1)[1])
    write(w, "unsorted.txt", "# phase: A\nsat   s a t\ncake  c a_ k _e\n")
    write(w, "tricky-only.txt", "# phase: A\nthe   tricky\nsaid  tricky\n")


GOLDEN = (
    "wjs-store 3\n"
    "settings help=off board=phonics phase=B focus=ai,ee size=4 sound_it_out=full "
    "celebration=full sound=1 vibration=1 shake=1 motion=system colour=dark timer=down "
    "timer_minutes=15 text=125 font=- language=-\n"
    "total stars=12 next_board=4\n"
    "best 3 5\n"
    "best 4 9\n"
    "word pin 20738 20739 3 2\n"
    "word sat 20738 20738 1 1\n"
    "find sat 1 3 A 20738 0 2\n"
    "find pin 3 3 A 20739 1 2\n"
    "day 20738 0 1\n"
    "day 20739 1 3\n"
    "board 2 20738 3 A 0 2 4 3 S,A,T/P,I,N/T,A,P sat,pin,tap\n"
    "board 3 20739 4 - 0 0 0 40 S,A,T,P/I,N,S,A/T,P,I,N/A,T,S,P -\n"
    "session state=playing size=3 seed=77 lit=pin\n"
    "sboard A 3 S,A,T/P,I,N/T,A,P 3 sat,pin,tap\n"
    "sfind sat 0 20739 1\n"
)
V2_SETTINGS = (
    "settings help=1 board=standard phase=A focus=- size=3 sound_it_out=full celebration=full "
    "sound=1 vibration=1 shake=1 motion=system colour=system text=100 font=- language=zh\n"
    "total stars=0 next_board=1\n"
)


def store_seeds() -> None:
    d = CORE / "corpus" / "store"
    write(d, "golden-v3.txt", seal(GOLDEN))
    write(d, "golden-v3-no-checksum.txt", GOLDEN)
    write(d, "migrate-v2.txt", seal("wjs-store 2\n" + V2_SETTINGS))
    write(d, "minimal-v3.txt", seal("wjs-store 3\n" + V2_SETTINGS.replace("language=zh", "language=-")))
    write(d, "header-only.txt", "wjs-store 3\n")
    write(d, "newer-version.txt", seal("wjs-store 99\n"))
    write(d, "crlf-v3.txt", seal(GOLDEN).replace("\n", "\r\n"))


def frame(width: int, height: int, fmt: int, pixels: bytes, stride: int = 0) -> bytes:
    """The harness's input: format, width - 1, height - 1, stride (little endian), pixels."""
    return struct.pack("<BBBH", fmt, width - 1, height - 1, stride) + pixels


def frame_seeds() -> None:
    d = SCAN / "corpus" / "frame_reader"
    gray = 1
    write(d, "black-8x8-gray.bin", frame(8, 8, 0, bytes(64)))
    write(d, "white-8x8-gray.bin", frame(8, 8, 0, bytes([255]) * 64))
    ramp = bytes((x * 255 // 63) for _ in range(64) for x in range(64))
    write(d, "ramp-64x64-gray.bin", frame(64, 64, 0, ramp))
    # A board-like picture: a dark square with a grid of light lines and a bright mark in each cell.
    side, cells = 96, 4
    image = bytearray([40]) * (side * side)
    cell = side // cells
    for y in range(side):
        for x in range(side):
            on_line = x % cell in (0, cell - 1) or y % cell in (0, cell - 1)
            mark = cell // 3 < x % cell < 2 * cell // 3 and cell // 3 < y % cell < 2 * cell // 3
            if on_line or mark:
                image[y * side + x] = 220
    write(d, "grid-96x96-gray.bin", frame(side, side, 0, bytes(image)))
    rgb = bytearray()
    for value in image:
        rgb += bytes([value, value, value])
    write(d, "grid-96x96-rgb.bin", frame(side, side, gray + 1, bytes(rgb)))
    write(d, "short-buffer.bin", frame(32, 32, 0, bytes(100)))
    write(d, "stride-too-small.bin", frame(16, 16, 0, bytes(256), stride=4))
    write(d, "stride-wide.bin", frame(16, 16, 0, bytes(16 * 40), stride=40))
    write(d, "header-only.bin", frame(1, 1, 4, b""))


def regressions() -> None:
    for lib, targets in (
        (CORE, "board dictionary dice_set phase_graphemes phase_words store"),
        (SCAN, "frame_reader"),
    ):
        for target in targets.split():
            directory = lib / "regressions" / target
            directory.mkdir(parents=True, exist_ok=True)
            keep = directory / ".gitkeep"
            if not any(p for p in directory.iterdir() if p.name != ".gitkeep") and not keep.exists():
                keep.write_text("")


def main() -> int:
    board_seeds()
    dictionary_seeds()
    dice_seeds()
    phase_seeds()
    store_seeds()
    frame_seeds()
    regressions()
    print("seeds written")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
