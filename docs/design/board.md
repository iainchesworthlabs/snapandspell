# Board design

The board: its sizes, how its letters are chosen, how it is laid out on each
kind of screen, how it looks, how it is shaken and dropped, and what drawing it
costs in licence terms. Written from the
[product requirements](../product-requirements.md) sections 3, 5, 7 and 12.
Proposals unless marked **Fact**. What the first release (the MVP, section 12)
takes from this page is marked **MVP**; the rest is the full product. Where the
build differs from a sentence, it says **As built**.

## Sizes

| Size | Dice | Faces | Mode B, Solo Play | Mode A, the scanner | Words of 3+ letters on a board, any dictionary word |
| --- | --- | --- | --- | --- | --- |
| 3x3 | 9 | 54 | **MVP**: the first board (**as built**, and the default) | Typed in only (no physical set) | 31 (10th to 90th percentile 9 to 59) |
| 4x4 | 16 | 96 | **MVP**: the design's default | **MVP**: the physical board | 120 (52 to 196) |
| 5x5 | 25 | 150 | **As built**, on the standard board, where a tile fits (the decision of 2026-10-16, PRD 12.7) | Offered; read and measured on synthetic frames only | 254 (134 to 404) |
| 6x6 | 36 | 216 | The same | The same | 435 (262 to 617) |

**Default size.** The design and PRD 5 say 4x4; the code and its test start a new
device at 3x3 (`settings.hpp`, `settings_test`), the parent can change the first
size offered to 3 or 4, and a child's pick on the box becomes the next default. The
author has not decided between them; this page and the code disagree until they do.

**Fact, measured:** the last column is from 200 boards a size, letters drawn at
random in proportion to their counts in ENABLE, solved with the repository's
library. It is a test corpus, not a dice set. It is the number of words
*anywhere in the dictionary*, which the MVP does not use as its word set (see
[Game design](game.md#words-and-what-each-does)). **Fact:** the library takes any
board from 1x1 to 16x16 and a tile of one to three letters.

Notes.

- The minimum word length is 3 on every size.
- A tile is one to three letters. The `Qu` face is one tile of two letters.
  Some larger sets carry other pairs (`Th`, `In`); this is **unverified**,
  read from a search summary of the Wikipedia article, and the library allows
  it. The real 6x6 game's four-letter minimum and four-minute timer are from the
  same summary and are the family's game, not the child's list.
- The 3x3 is the same game on nine cells and is **not** a window onto a 4x4. In
  the scanner there is no 3x3: a real board is 4x4 or larger. (**As built** the
  scanner's reader and size picker accept 3 to 8 and 3 to 6; the author has not
  settled the sizes Mode A offers.)

## Where the letters come from

There are two different problems, and the MVP has only the first.

### The phase board (MVP)

In Mode B the letters are **seeded**, not shaken from dice: a board is made so
that it holds three to five words of the active phonics phase and so is quickly
cleared (section 12.3). The method, the measurements behind it and the
alternatives are on [Phonics](phonics.md#seeding). What this page needs from it:

- A board is nine or sixteen cells. Some cells may be **dead jewels**: a tile
  that is drawn as a jewel, has no letter, and cannot be part of a word. They
  hold the count of viable words down without showing the child letters the
  phase has not taught. To the library a dead jewel is a letter that no word of
  the phase uses (the app maps it; the library is unchanged).
- The roll is a pure function of the phase data and a seed, in the library, so a
  board can be replayed and tested.
- The animation is given a finished board and the dice it was rolled with
  ([What the scene is given](#what-the-scene-is-given)).

### Dice (the standard board, in the MVP: decided 2026-10-16)

The standard board, the MVP's default, comes from dice, as in the physical game.
The author chose the third source below, the project's own dice, and
`tools/data/make_dice.ps1` and `data/dice` are built. The faces of a set the
author owns are for the scanner mode, where a physical board is on the table and
the app reads it with OCR and does not mix letters.

| Source | What it is | Standing |
| --- | --- | --- |
| The printed faces of a set the author owns | Facts read off an object | Whether a set's letters, as a selection, can belong to someone is a legal question and is **not answered here** ([L]) |
| A list found online | Unknown origin | Not usable. `data/README.md` says "Do not add a list whose origin you cannot state" |
| Made by this project from a fixed English letter-frequency table | The project's own work, from published Scrabble-style tile counts | Clean |

The third is the recommendation and is what is built. `tools/data/make_dice.ps1`
deals six different faces to each die in proportion to the fixed table
(A9 B2 C2 D4 E12 ... Z1, the 100-tile English counts with the two blanks not
counted), with one to three vowels and at most one of J K Q V X Z on a die. Q is
the tile QU. A set is dealt again unless 30% to 40% of its faces are vowels and S
is at most 8% of them, so a small board has letters that make words rather than
a heap of plurals. It writes `data/dice/<n>x<n>.txt` with the table, the method
and the seed in the header, as `data/README.md` requires. Nothing here copies a
product's distribution of letters: the table is published frequency data, not a
printed die set. In the scanner the real dice are read from the table, so no
product's distribution is needed at all. Boggle is another company's name:
public text should say "a word-finding game" ([L]).

**Fact, measured, so the open dictionary can be designed later.** Share of
boards a floor of K known words would reject, on the test corpus:

| Word list | Size | K=1 | K=3 | K=5 | K=8 |
| --- | --- | --- | --- | --- | --- |
| Core and ten categories (689) | 3x3 | 13% | 47% | 73% | 91% |
| | 4x4 | 0% | 3% | 14% | 38% |
| | 5x5 | 0% | 0% | 0% | 6% |
| | 6x6 | 0% | 0% | 0% | 0% |
| Core tier only (237) | 3x3 | 29% | 72% | 91% | 99% |
| | 4x4 | 2% | 23% | 55% | 82% |
| | 5x5 | 0% | 3% | 16% | 48% |
| | 6x6 | 0% | 0% | 0% | 14% |

## Layouts and sizes

### The rule

The window is split by its shape, not by what device it is. **MVP**: this is
the brief's two-thirds / one-third split, everywhere.

- **Wide** (width at least height): the board is in the leading two-thirds of
  the width, the panel in the trailing third, both full height.
- **Tall**: the board is in the top two-thirds of the height, the panel in the
  bottom third, both full width. The board is a square that fits the width, so
  the board's area is never taller than the window is wide: on a phone held
  upright (390 x 772 under its bar) the board has 390 and the panel the other
  382, not 257, and no tile gets smaller for it. The panel is also never less than
  300 dp (the foot with Clear and Done, and the words above it), so a short, tall
  window (a phone of 640 dp high) gives the board a little less: 5 x 5 is then 44 dp
  on a 360 x 640 phone and is not offered.

The board is the largest square that fits its area less an inset, drawn
centred. The inset is 12 dp when the area's shorter side is under 600 dp, 24 dp
from 600 dp up. The gap between tiles is 6 dp, or 8 dp from 600 dp up. These
are this app's own numbers. Cubeulator's page margins are 24 dp (16 under 360),
32 and 40 by width class, and it singles out a "short" window (height under
480) for a phone on its side; here the inset is chosen from the board area, so
a phone on its side gets the same board as one held upright.

```text
tile = (S - (n - 1) * gap) / n        S = the shorter side of the area - 2 * inset
```

Rotating keeps the game, the path being drawn and the tray's scroll position.
Only the geometry is recomputed.

### Reference windows

**Estimate.** Logical sizes of typical devices, from memory; the author's own
devices have to replace them (the MVP is an iPad; its model is not known to
this repository). dp is Android's unit and about the same as an iOS point.

| Window | Shape | Board area | Panel | 3x3 | 4x4 | 5x5 | 6x6 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Phone 360 x 740 | Tall | 360 x 360 | 360 x 380 | 108 | 80 | 62 | 51 |
| Phone 390 x 800 | Tall | 390 x 390 | 390 x 410 | 118 | 87 | 68 | 56 |
| Phone 320 x 600 | Tall | 320 x 300 | 320 x 300 | 88 | 65 | 50 | **41** |
| Phone on its side 740 x 360 | Wide | 493 x 360 | 247 x 360 | 108 | 80 | 62 | 51 |
| Small tablet 744 x 1133 | Tall | 744 x 744 | 744 x 389 | 227 | 168 | 133 | 109 |
| Tablet 800 x 1280 | Tall | 800 x 800 | 800 x 480 | 245 | 182 | 144 | 119 |
| Tablet 1280 x 800 | Wide | 853 x 800 | 427 x 800 | 245 | 182 | 144 | 119 |
| Desktop window 1100 x 700 | Wide | 733 x 700 | 367 x 700 | 212 | 157 | 124 | 102 |
| Narrow browser 420 x 800 | Tall | 420 x 420 | 420 x 380 | 128 | 95 | 74 | 61 |

[layouts.svg](layouts.svg) draws four of these to scale (its tall windows show the board's area at two thirds of the height, as it was before the area was capped to the width). On an iPad the MVP's
3x3 and 4x4 are 160 dp or more, which is large for a finger.

### Which sizes are offered

A size is offered only if its tile is at least **48 dp**, Cubeulator's floor for
any target. A size that does not fit is not shown, not shown disabled (the
Cubeulator rule: "if nothing the person can do would enable it, it is not
shown"). 56 dp and over is comfortable; 48 to 55 relies on the tracing
tolerance. In the scanner the real board decides the size, so a replica smaller
than 48 dp is still shown, for reading and for the highlighted tile.

### The panel at each size

| Panel | Where | Words I found | Hints | History |
| --- | --- | --- | --- | --- |
| 300 dp or more on both sides | Tablets, desktop | A tray, 3 or more rows | A button and a display | Three thumbnails inline |
| Anything smaller | Phones | A tray in the space left | One button, 64 dp | One button that opens a sheet |

**Not built as a table.** The panel is shaped by the window's shape, not by size:
the same tray, hint, clock and Finish/Clear/Done column everywhere, and history is
always a sheet opened from the bar or from home. The table above is the design for
phones, which have no layout yet.

## The tile

| Part | Rule |
| --- | --- |
| Shape | A square with square corners. Cubeulator's rule: flat, no shadows, no gradients, regions ruled, "the cube is the colour". The dice are plain and the jewels are the colour. In the 3D scene (below) the dice are lit solids and the faces keep this look. **As built:** the tile's corners are slightly rounded (a small radius token) |
| Letter | Upper case, centred, upright, about 55% of the tile's height (a starting value); `Qu` at 80% of that so it fits (**as built** 78%). The real board is upper case, so the replica matches it. The typeface and the school-font setting are on [Application design](application.md#type) |
| Rest | Cream face, dark letter, dark edge |
| Lit | On the path: the face turns amber, the edge thickens to 4 dp, and the tile scales to 106%. Not colour alone. **As built:** the edge thickens to 3 dp (a token) and the lit tile is not scaled |
| Grapheme link | Tiles that make one sound share a bar under them and a common edge colour; a split digraph is joined by an arc over the tile between. Shape first, colour second. **Not built:** the live board does not draw the bar or the arc (the library binds the blocks; the layer that drew them was deleted with `TraceLayer`) |
| Found | The path's tiles flash once in the order they were traced, 60 ms apart. **Not built:** the live board has no staggered flash |
| Start tile | A pulsing ring: shape and motion, not colour alone. **MVP**: the only help |
| Dead jewel | A faceted shape with no letter, not part of any path; a touch on it does nothing |
| Doubtful | Scanner check only: a ring and a `?` |

## The shake and the drop

The brief expected "a 3D dice shake in a virtual 3D container" in "the same style as the Rubik's
cube in Cubeulator". The first build drew it in Qt Quick 3D. On 2026-10-24 the owner chose, for
the Toybox Pixel restyle, a **2D pixel scene** instead ([Drawn in 2D](#drawn-in-2d)): flat dice
with a hard shadow, a box that jumps in whole steps and a die that falls through six frames. The
stages, the seam and the seeded order are the same; what changed is how it is drawn and how long
each stage takes.

### The stages

All numbers are **starting values to tune on a device**. They are `apps/app/qml/solo/SceneScript.js`'s
and `docs/design/toybox/DESIGN.md`'s ("Motion"); `scene_script_test.cpp` holds them.

| Stage | Full motion | Reduced motion |
| --- | --- | --- |
| Shake | The closed box hops in whole steps of 6 px, one every 75 ms (eight in 600 ms, a harder shake a little faster), 0.6 s at least and while the device shakes, 3 s at most. A rattle signalled no more than 8 a second for the sound and the haptic taps | The box is still; one soft rattle, 0.3 s |
| Lid | Lifts away in four steps, 250 ms | Cross-fades, 120 ms |
| Drop | Each die falls through six frames in 380 ms, started in a fixed pseudo-random order. The gap between starts is the smaller of 40 ms and 700 ms divided by the number of dice. One click each when it lands (the fourth frame), pitched a little differently | All dice fade in together, 120 ms |
| Settle | A hold of 150 ms, then the flat board | None |
| Layout | The board slides to the leading two-thirds and the panel slides in, 380 ms | Cross-fade, 120 ms |
| Total after the shake | About 1.4 s (4x4) and 1.5 s (6x6) | Under 0.6 s |

### No physics engine decides the board

The board is decided by the library first. So nothing simulates: where each die ends is its tile,
and the fall is a script of whole pixels. A game would otherwise have to read the board back from
a simulation, and in Mode B the board is a *seeded* one that must hold three to five words. The
script is a pure function of the board's size, the seed and the time, so a seeded play replays
exactly, in any build, which is what a test, a recording and a "play this board again" need. The
rattle answers the device only in how fast the box hops (`pushMotion`'s two numbers, `agitation`
and `tilt`); it never changes where a die lands. (The first build simulated rigid cubes in about
fourteen hundred lines of plain C++, and Qt Quick 3D Physics, which is GPL-only, was never an
option. Both are gone with the 3D scene.)

### What the scene is given

A die has six faces with six different letters, and a shake leaves one up. The
scene draws that, so it is given more than a finished board: the library's roll
returns the dice with the board, and the scene takes them as they are.

```text
sns::roll_with_placements(dice, seed) -> RolledBoard
    board:      the Board, as sns::roll returns it
    placements: for each tile, row-major: the die on it (an index into the set)
                and which of its faces is up

scene.play(board, seed)         board = { rows, columns, tiles, faces, up }
    tiles[t]: the letters of the finished board's tile t
    faces[t]: the six strings on the die on that tile ("A", "QU", ...)
    up[t]:    which of the six is up, so that faces[t][up[t]] == tiles[t]
```

The pixel scene draws the die's upright letter and nothing else: `faces` and `up` are the 3D
dice's and the scene ignores them (a board without them, from the phonics game or the store, is
drawn like any other). It decides nothing and reads no letter: the tile strings are the library's,
passed through, and a library test holds that the face a placement names is the tile's letters,
for 300 boards of each shipped size. A dead jewel (the phonics board's) is a die with a pixel
jewel on it. `scene.prepare(board)` is kept in the seam and does nothing: there is no texture to
draw ahead.

**Built.** `SoloBoard` (`apps/app/src/solo/solo_game.hpp`) carries `faces` and `up`
beside the tiles. `RealSoloGame::board()` takes them from
`sns::roll_with_placements` with the dealt board's seed on the standard board;
the phonics board and the fake game carry none, and a game brought back from the
store carries none (its board's roll seed was not kept). The scene is the application's own QML,
`apps/app/qml/solo/BoardScene.qml` with `SceneDie.qml` and `SceneScript.js`; the Quick Test suites
of the Solo screen give it a stub of the same interface instead.

### Drawn in 2D

| | A: Qt Quick 3D (the first build) | B: 2D Qt Quick, scripted (**chosen**) |
| --- | --- | --- |
| Look | Lit, glossy dice in a box: the brief's "3D container" and Cubeulator's cube | Flat pixel dice with a hard shadow, a box that hops in steps: Toybox Pixel |
| Licence the build can carry | GPL-3.0 only | GPL-3.0-or-later, Qt under the LGPL or GPL |
| Runs in a browser | Heavily (WebGL 2, large) | Yes, Qt Quick alone |
| Cost | About 140 ms to create each 3D view in Cubeulator, and a baked light probe | Small |

**Fact, checked in the Qt 6.12.0 SBOMs of the Windows kit:** `Quick3D` and `Quick3DParticles`
are `LicenseRef-Qt-Commercial OR GPL-3.0-only`; `Quick`, `QuickShapes`, `QuickParticles` and
`QuickEffects` are `LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only`.

**Decided (the author, 2026-10-16): A for the MVP, behind a seam, with the choice taken at D2.**
**Decided (the author, 2026-10-24, Stage 2 of the Toybox restyle): B.** The seam made it a rewrite
of one component and not of the game. The 3D scene, its dice simulation, its face atlas, its light
probe and the harness that measured it are removed; the binary is GPL-3.0-or-later again, and D2
no longer has the licence cost to weigh ([Licensing](../licensing.md#qt)).

### How it is drawn

A die is the kit's `TileFace` (a cream face, a 3 px ink outline, a hard 4 px shadow, the Andika
letter) or the same face with a pixel jewel; the tray is the kit's `BoardTray` (a 6 px outline, a
12 px hard drop) and `BoardWell` (a teal checker, a cell behind each die); the lid is a cream slab
with an ink outline, a hard shadow and a coral label. They are the flat board's own components, at
the flat board's own positions, so when the scene settles and the flat board takes over nothing
changes by a pixel. Only whole-pixel `x` and `y` and `opacity` are animated; no shaders, no effect
layers, no blur, no rotation. At most 36 dice are alive at once.

Confetti and stars are a 2D particle emitter (Qt Quick Particles, LGPL set) laid
over the view. **Reduced motion** removes it, and its flash rate is at most
three a second (see [Application design](application.md#motion)).

## Unknowns

- Whether the fall reads well at a child's eye height: the cheapest thing to look at and the
  first thing to show. The pixel scene has been seen in pictures and tested, not on a device.
- The tolerance for tracing on a 51 dp tile with a small finger. On an iPad the
  tiles are large and this matters on phones.
- Whether dead jewels are acceptable to the literacy reviewer, or whether a
  board with untaught letters is what a classroom would expect. It is a
  question for the review (G7).
- Whether the window, the scene included, runs well on the author's iPad. Nothing has been measured on
  a device.
