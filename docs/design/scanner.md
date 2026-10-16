# Scanner design

How Mode A reads a real board: the camera and the reader behind two platform
interfaces, the library that finds the board in a frame and reads each tile, the
check that stands between the reader and the solver, the accuracy it is held to
and the corpus that holds it there. Written from the
[game design](game.md#mode-a-the-family-assistant-and-scanner), the
[application design](application.md#privacy-built-in-and-checked), the
[roadmap](../roadmap.md) (M4) and [Platform isolation](../platform-isolation.md).
Proposals unless marked **Fact**; the marks are the ones on the
[overview](index.md#how-to-read-these-pages).

**Status.** The numeric target and the corpus plan below were written before
any recognition code. What was built and what it measured is in
[What was measured](#what-was-measured), at the end; **nothing here has been run
on a tablet, on an iPad or on a photograph of real dice.**

## What this slice is

The first working slice of the board reader: a frame in, a board and a doubt per
tile out, and the data flow that makes a grown-up confirm it before the solver
sees it. Not the screen (the Solo Play UI owns `src/app`), not the iOS reader
(it needs a Mac and an iPad, see [What is left](#what-is-left-for-a-mac-and-an-ipad)).

```text
 camera (platform)        wjs::scan (portable, Qt-free)                    UI binds to
 ─────────────────        ─────────────────────────────                    ───────────
 OwnedFrame ──▶ read_board(frame, options)                        BoardDraft
                  1. luma, working image
                  2. find the board: dice → lattice → convex quad
                  3. perspective warp to a square, count the cells
                  4. read each cell's one rotated glyph (templates)
                  ──▶ BoardReading  (letters, confidence, runners-up, per tile)
                                              from_reading ──▶ BoardDraft
                                                          set_tile / accept / turn / resize
                                                          doubtful()  (the ringed tiles)
                                                          confirm() ──▶ wjs::Board ──▶ solver
```

Three rules hold the shape of it:

1. **The solver sees only a confirmed board.** A `BoardReading` has no
   conversion to `Board`. The only way from a reading to a `Board` is
   `BoardDraft::confirm()`, which a grown-up's *Yes, play* calls (game design:
   "the check is never skipped in the MVP"). A test holds this at compile time.
2. **A doubt is data, not a message.** Each tile carries a confidence, its
   runners-up and a `doubtful` flag; what the screen does with them (a ring and
   a `?`, board design) is the UI's. Nothing in `wjs::scan` produces text for a
   child, lists a word, or calls the solver. Mode A highlights only the start
   tile (PRD 12.4); that is the session flow's, after `confirm()`.
3. **Frames live in memory only.** No code under `src/scan` or in the two
   interfaces opens a file, and a build check says so
   ([Privacy](#privacy-built-in-and-checked)).

## The target, set before the code

A number is needed because the check screen makes a *wrong* tile expensive in
one way only: a wrong tile the reader **did not doubt** is the one the
grown-up confirms without seeing, and the solver then answers a different
board. A tile the reader doubts costs one tap. So the targets bound the silent
error first, the cost of doubt second, and the plain accuracy third.

**Definitions**, per tile, over the frames in which the board was found:

| Term | Meaning |
| --- | --- |
| Correct | The reported letters equal the truth (`QU` is one tile) |
| Flagged | The reader marked the tile `doubtful` |
| **Silent error** | Not correct **and** not flagged. The costly outcome |
| Offered | The truth is the reading or one of its listed runners-up (at most two), which is what the letter picker puts first |
| Board found | The reader returned a board of the right size whose cells map onto the real dice. A frame it cannot read is `board_not_found`, which the screen answers with *Read again*: it costs a retake and never a wrong board |

**The target** (a threshold each, over the synthetic test split below; a
condition passes only if all five hold):

| Condition | Board found | Correct | Silent errors | Flagged | Offered |
| --- | --- | --- | --- | --- | --- |
| `upright`: tilt up to 10°, dice square in their cells | ≥ 99% | ≥ 97% | ≤ 0.3% | ≤ 12% | ≥ 99.7% |
| `angled`: tilt up to 35°, any turn of the board, dice square in their cells | ≥ 97% | ≥ 95% | ≤ 0.5% | ≤ 15% | ≥ 99.5% |
| `loose`: as `angled`, each die's letter at any angle | ≥ 97% | ≥ 92% | ≤ 1.0% | ≤ 20% | ≥ 99% |
| `glare`: as `angled`, with one or two soft highlights and a streak | ≥ 90% | ≥ 80% | ≤ 2.0% | ≤ 35% | ≥ 97% |

And on **frames with no board** (a room, a sheet of paper, a table edge): at
least 95% answer `board_not_found`, and none answers a board with more than
half of its tiles unflagged.

**Why these numbers.** They are proposals, chosen before measuring, from what
each costs a grown-up at the table; none is derived from a field study.

- *Silent errors* are the number that matters. At 0.5% a 4x4 board has about a
  1 in 12 chance of one unseen wrong tile; at 0.3%, about 1 in 20. That is
  not zero and cannot be made zero by a reader: the adult's *Yes, play* is the
  safety, which is why the check is never skipped.
- *Flagged* at 12 to 15% is about two ringed tiles on a 4x4 board: a grown-up
  can look at two. At 35% (glare) it is six, and *Read again* is the better
  answer; the glare row is there to be honest about it, not to set a standard.
- *Correct* is the convenience measure: the less the picker is opened, the better,
  but a wrong tile that is flagged is already cheap.
- *Offered* says the picker's first suggestions hold the truth, so a correction
  is one tap and not a scroll through A to Z.

The targets can be **revised only with a reason written beside the change**,
never moved to turn a red run green: the same rule as the performance budgets
([Testing](../testing.md#what-a-failure-means-and-how-to-move-a-budget)).
Nothing in them speaks to real dice, real light or a real child's room; see
[What this does not prove](#what-this-does-not-prove).

## The corpus plan

**Recorded camera frames show a child and a room. They are never committed**
(`.gitignore` already excludes `/tests/data/captures/` and `/docs/evidence/`;
[Licensing](../licensing.md#camera-input-and-letter-recognition)). What is in the
repository is **synthetic**: a generator in `tests/scan/synth` that draws a
board of dice from numbers and a seed, and nothing else.

### What the generator draws

A frame is a grey image drawn from a scene in the board's plane and seen through
a pinhole camera, by inverse mapping (every pixel asks the scene what it is):

| Part | What is drawn |
| --- | --- |
| The board | An N x N grid of cream dice in a dark tray, 4x4, 5x5 or 6x6 (the scanner has no 3x3, [Board design](board.md#sizes)); a gap between dice about a tenth of a die; rounded die corners; a die-to-die brightness spread |
| The letter | One glyph a die, upper case, about 55% of the die high ([Board design](board.md#the-tile)), as strokes of a width that varies by board; `Qu` is a large Q and a small u. Each board's glyphs are a **perturbed** copy of the reader's skeleton (stroke width, slant, width, every control point jittered), so the test is not the template matched to itself. It is **not a different typeface**, which is the largest thing the corpus cannot say (below) |
| The turn of a letter | A multiple of a quarter turn plus a few degrees (dice in cells), or any angle (`loose`) |
| The view | A tilt about each axis, a turn of the whole board in the image, a distance, a position. A pinhole homography, so the dice are quadrilaterals that shrink with distance |
| The room | A light gradient and a vignette, a table edge, one or two bright rectangles (a sheet of paper, a phone) that are not the board, sensor noise, blur |
| Glare | Soft elliptical highlights added over the dice and the tray, saturating, and a thin streak; letters under them wash out |
| Colour | None: the corpus is grey. A separate case draws RGB to test the conversion |

**What it deliberately leaves out**, so the numbers are not read as more than
they are: reflections of the room in the dice, the sides of the dice (their
visible edges under tilt), shadows of the hand, motion blur, a lid or a hand in
the frame, rolling shutter, a camera's own sharpening and compression, a die
that is not flat on its cell, and every real typeface.

### Splits and size

| Split | Used for | Seeds |
| --- | --- | --- |
| **Tuning** | Choosing the doubt thresholds and nothing else; never reported against the target | Seed base 1,000,000 |
| **Test** | The target. Seeds never used for tuning | Seed base 2,000,000 |

Each condition has 24 boards in the test split (12 of 4x4, 6 of 5x5, 6 of 6x6),
558 tiles, so a silent-error rate of 0.5% is three tiles: the resolution is
coarse, and a margin is left between what is measured and what is claimed. Board
`i` of a condition is drawn from `std::mt19937` seeded from the condition, the
split and `i`, with draws **taken modulo**, never `std::uniform_int_distribution`
(its algorithm is each standard library's own), so a seed is the same frame on
MSVC, libstdc++ and libc++ ([Testing](../testing.md#the-corpus)). Only the
arithmetic of the image (a float rendering) can differ in the last digit between
compilers, and the targets keep a margin for that.

### Where it runs

| Build | Runs |
| --- | --- |
| Release (`ctest --preset linux-gcc-release`, the `Performance` leg) | The whole test split against the target, and the frames with no board |
| Debug with the sanitizers (the build legs) | A **smoke** subset (one board of each condition): the pipeline runs clean under ASan and gets the plain cases right. The target is **skipped**, not failed, as the performance tier skips: the corpus is much longer there |

The generator also writes PNG files for looking at: `wjs_scan_render` (built with
the tests) writes the corpus, or a contact sheet of it, to a directory outside
the tree. **The app never writes a frame**; the generator is a test tool.

### Recorded frames, outside the tree

The real measurement is on real photographs, and the repository cannot hold
them. `tests/scan` has a hidden case, `[.recorded]`, that reads frames from the
directory in `WJS_SCAN_RECORDED_DIR` (binary PGM, grey, with a `<name>.txt`
beside each holding the true board in the board text format) and prints the same
five numbers. It skips when the variable is unset, as
[Licensing](../licensing.md#camera-input-and-letter-recognition) asks. **It has
not been run on anything**, because no frames were recorded; this is the first
thing to do on the iPad (below).

## The library

`wjs_scan` (`wjs::scan`), a static library of its own in `src/scan`. It links
`wjs::lib` for `Board` and nothing else; the solver library does not link it and
stays free of image code.

### No image dependency

**The pipeline adds nothing to the dependency diff**: `vcpkg.json` is unchanged
and the library is plain C++23 over a byte buffer. So there is no opt-in switch:
the default CI legs build and test it, and they stay fast (the library and its
tests compile in a few seconds, the full accuracy gate runs in the Release leg
only).

Why not OpenCV, the obvious choice and Cubeulator's: it would add a package
whose first vcpkg build on a clean runner is long (the practice is to pay it once
into the binary cache), a second set of per-platform packaging for iOS and
Android, and no capability this job needs. The work is *one board of one kind of
object in one known typeface*: find a bright quadrilateral lattice, warp it, and
compare a glyph with 27 templates. OpenCV's strengths (feature detection,
learned models, calibrated cameras) are not used by it. **Estimate:** the
pipeline is a few thousand lines because it avoids a general image library, and
that is what the next paragraph's cost is.

The price is that the image code is ours: a threshold, a connected-component
label, a convex hull, a homography and a bilinear warp, each tested here, and
none tuned on a camera. If the real dice or real frames show a limit that a
general library would cure, `board_reader` is the seam to put it behind (Vision
on iOS needs the same seam), and an OpenCV-backed reader would be an opt-in
vcpkg feature like `profiling`: it would add `opencv4` (Apache-2.0, compatible
with GPL-3.0-or-later; not added now).

### The pipeline

| Step | What it does | Notes |
| --- | --- | --- |
| Frame | A `Frame` is a view of a pixel buffer: gray, RGB, BGR, RGBA or BGRA, with a stride. It becomes luma (integer BT.601) at a working size of at most 480 px on the long side | A YUV camera's Y plane is a `gray8` frame at no cost |
| Find the board | A local threshold (the mean over a window of a quarter of the frame, plus a margin) marks the bright. Connected bright blobs that are die-shaped (a convex, four-sided hull; about square; a size window) are candidates; the largest group that agrees on size and sits at lattice spacing is the dice. The board is the **largest convex quadrilateral** of their outline: their convex hull, reduced to four corners | A bright sheet of paper is one big blob that has no neighbours at lattice spacing, so it is not a die. If the dice cannot be told apart (no gaps), the fallback is the largest convex bright quadrilateral |
| Warp | A homography from the four corners to a square, sampled bilinearly from the frame, 480 px a side. The corner order is the one nearest to upright in the image | |
| Count the cells | Comb-fit the warped square's row and column profiles for N from 3 to 8: the dice are bright and the gaps dark at N-th boundaries. The size is the best fit | The size is **shown first** on the check screen ([Game](game.md#the-check-screen)) and the adult can change it |
| Read a tile | The cell's central patch (about 64% of the cell) is sampled at 24 turns of 15°, each compared with the 27 templates by normalised correlation; the best few are refined to about 2.5° at fine scale. The score is the best correlation for the class | Absolute glyph size is kept, so `N` turned a quarter and `Z` differ by shape and by proportion, and `M` against a turned `W` by their stems |
| Confidence | The best class's score and its margin over the best *other* class. A tile is **doubtful** if the score is under a floor or the margin is under a gap, or the cell had no ink | The two thresholds are set on the tuning split only |
| Output | Per tile: letters, confidence in [0, 1], the turn found, up to two runners-up; per board: size, corners in the frame, a size confidence | |

The templates are the renders of the library's own stroke font
(`wjs/scan/stroke_font.hpp`): 26 letters and `Qu`, as polylines in a box 4 wide and
6 tall, drawn at any angle and weight by distance to the strokes. **Template
matching and a bitmap of the real dice font are interchangeable behind
`TemplateBank`**, which is how a font of the real dice (once its licence is read)
or a classifier trained on renders replaces the stand-in without touching the
rest.

The two thresholds and the grid-fit tolerances are **starting values**, tuned on
synthetic frames and expected to move on the first real ones.

### Confirm and correct

`BoardDraft` (`wjs/scan/confirmation.hpp`) is the model the check screen binds
to. It is Qt-free and holds no pixels.

| Operation | What it does |
| --- | --- |
| `BoardDraft::from_reading(reading)` | The grid as read: tiles with their doubt and runners-up |
| `BoardDraft::empty(rows, columns)` | The *Type in your board* grid; nothing is doubted, nothing is filled |
| `tile(position)` | Letters, `origin` (read, corrected, typed), confidence, `doubtful`, runners-up |
| `set_tile(position, letters)` | A grown-up's correction: one to three letters A to Z, upper-cased; the tile is settled |
| `accept_tile(position)` | "This one is right": settles a doubtful tile without changing it |
| `turn(direction)` | A quarter turn of the whole grid, for the *Turn* control |
| `resize(rows, columns)` | A grown-up corrects the counted size; tiles in the overlap are kept |
| `doubtful()` | The positions to ring, in reading order |
| `confirm()` | A `Board`, or `incomplete` if any tile has no letters |

`confirm()` does not refuse a board with ringed tiles left: the *Yes, play*
press is the grown-up's statement that they looked, and the check is for them,
not against them. The screen is free to ask first; `doubtful()` is how it knows.

## The platform interfaces

Two headers under `src/platforms/interfaces/wjs/platform/`, one definition each
per platform, chosen only by `cmake/Platform.cmake` through the platform
fragments:

| Interface | What it is | Windows, Linux, macOS (today) |
| --- | --- | --- |
| `camera.hpp` | `enumerate_devices`, `start`, `stop`, `is_capturing`, `try_pop_latest_frame`: Cubeulator's `camera.hpp` without OpenCV types (an `OwnedFrame` for its `cv::Mat`) and without its photometry lock, which is about colour sampling and not about letters | **No camera**: `enumerate_devices` says `unavailable`. A desktop webcam needs a per-OS capture (Media Foundation, V4L2, AVFoundation, or Qt Multimedia, which is not in the shared tree), and Mode A is an iPad feature of the MVP. *Type in your board* is the answer there ([Game](game.md#no-camera)) |
| `board_reader.hpp` | `read(frame, options)`: a `BoardReading` or a `ReadError`; `name()` says which reader this is | **The portable reader**: the three desktops forward to `wjs::scan::read_board`, from one file shared by all of them (`detail/`) |

The camera hands out frames the way Cubeulator's does, a single-slot mailbox:
the latest frame not yet taken, or none. The UI session decides when the camera
is on (only in `scan.ready`, [Game](game.md#the-mvp-one-capture)); the interface
only provides it.

`wjs_platform` now links `wjs_scan` (for the frame and reading types the two
headers use), so the layering is `wjs_lib` ← `wjs_scan` ← `wjs_platform` ← the
front ends; `wjs_scan` does not include a platform header.

### What is left for a Mac and an iPad

Written down so it is not taken as done. **None of this exists or has been
built, run or tested:**

| Piece | What it is | Needs |
| --- | --- | --- |
| iOS platform fragment | `src/platforms/ios/platform.cmake`, one branch in `cmake/Platform.cmake` (which today stops with an error for `IOS`), iOS presets and a CI leg | The Qt iOS kit (not in the shared tree), a Mac runner |
| `camera_ios.mm` | Objective-C++ over `AVCaptureSession` with a video-only input (no audio input is ever created: the privacy rule), `AVCaptureVideoDataOutput` delivering `CVPixelBuffer`s, the Y plane of a bi-planar 4:2:0 buffer wrapped as a `gray8` `Frame`, copied into the mailbox. Camera permission through Qt's `QCameraPermission` as Cubeulator does, and `open_permission_settings` for a refusal | An iPad, an ad-hoc profile, the Info.plist usage string (and **no** microphone string) |
| `board_reader_ios.mm` | Option 1: the portable pipeline finds, warps and splits; `VNRecognizeTextRequest` (or a Core ML classifier trained on renders of the dice font) classifies each cell's image at four quarter turns; the results fill `TileReading`. Option 2: Vision's rectangle detector finds the board. **Whether Vision reads a single rotated capital on a glossy die better than the template matcher is unknown, and is what the iPad decides** | An iPad and recorded frames |
| Recorded frames and their truth | A dozen boards of the author's own dice, photographed on the iPad at ordinary angles and in ordinary light, **never committed**, with a truth text beside each, read by `[.recorded]` | The author's dice and the iPad |
| The real typeface | The dice's letters measured against the stand-in stroke font, and the templates re-rendered from the real face (a licensed font or a scan of the dice, with the licence read) | The dice |
| Tuning | The doubt thresholds and the grid tolerances re-tuned on the recorded frames | The above |

The interface was designed so each fits: a platform's `board_reader` receives a
`Frame` (so the Y plane needs no copy) and returns the same `BoardReading`;
the portable pipeline is a library a native reader may call for the parts it
does not replace (find the board, warp, split the cells).

## Privacy, built in and checked

[Application design](application.md#privacy-built-in-and-checked) freezes the
MVP's rules; the scanner touches one of them closely, a camera pointed at a
child's room.

| Rule | In the build | Checked by |
| --- | --- | --- |
| Frames are processed on the device and **not kept** | `Frame` is a view; an `OwnedFrame` is a buffer in memory with no save. Nothing under `src/scan` or the two interfaces includes a file, stream or `<cstdio>` header or calls a file API | `scan.no_frame_files` (`tests/scan/check_no_frame_files.cmake`), run in every `ctest`: fails on a file-output facility in `src/scan` and the two interface headers and their implementations |
| No audio input | The camera interface has no audio and `start` takes a video device; the iOS notes above forbid an audio input | The existing rule on audio-capture classes when it is written |
| No network | The library includes no network header | The same scan; the platform scan for network classes |
| No export | No share, print or save | The same scan |
| No profiler in what ships | Two zones, `scan.find_board` and `scan.read_tiles`, through `wjs/profiling.hpp` (empty in a normal build) | `profiling.shipped_presets` (existing) |

A recorded frame can be read only by the test that asks for it, from outside
the tree, and only when the person who recorded it sets the variable.

## Performance

[Performance design](performance.md) proposes 3 s from capture to the check
screen. The library carries the usual counters and zones:

- `wjs/scan/work.hpp` tallies, once per read, the frames, the working pixels,
  the die candidates, the cells read, the template comparisons and the pixel
  buffers allocated, per thread, as `wjs/solver_work.hpp` does for the solver.
- `tests/perf` holds **work budgets** for a fixed set of frames (the counts are
  exact on every machine and compiler) and a hidden report, in the style of the
  other rows. A change that reads every cell at four times the angles, or
  allocates a buffer a pixel row, fails a count before a stopwatch notices.
- A wall time on this desktop is in [What was measured](#what-was-measured), as
  a figure to compare with, not a budget; a tablet is unmeasured.

## What this does not prove

- **Real dice.** The templates are the project's own stroke font; real dice have
  a different typeface, a moulded or printed face, wear, and a gloss the
  synthetic glare does not model. The accuracy on them is **unknown**. The
  corpus can say the geometry works (a board is found under tilt and turn, a
  glyph is read at any angle) and that doubt is raised when it should be; it
  cannot say the dice are readable.
- **Real cameras and light**: the tablet's noise, sharpening, auto-exposure and a
  ceiling light in the dice. Cubeulator's measured work on camera automatics
  (exposure that drifts 14 to 18 L points inside a scan) is a warning that the
  synthetic noise understates it.
- **A child in the frame**: hands, a lid, the shake.
- **Devices.** Every time in this page is a desktop; the iPad's is unmeasured.
- **The 5x5 and 6x6 real boards**: the MVP scanner is 4x4 ([Board](board.md#sizes));
  the larger sizes are read and measured here on synthetic frames only.
- **That the thresholds transfer.** They were tuned on one synthetic split.

## Unknowns

- Whether the template matcher or Vision reads a real die better, and with what
  doubt (the iPad decides).
- The real dice's typeface, and whether any of its letters is ambiguous under
  turn in a way the stroke font is not (`N` and `Z`, `M` and `W`, `H` and a
  serifed `I`, `C` and `U`).
- Whether the tray of a real set has gaps between the dice wide enough for the
  lattice step to see, or whether the fallback (the largest bright
  quadrilateral) is the common path.
- The camera's exposure and the glossy dice: whether the tablet needs an
  exposure lock (Cubeulator's photometry API was left out of this interface
  because nothing here samples colour; an exposure lock may come back).

## What was measured

*To be filled in once the corpus has run.*
