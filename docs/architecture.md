# Architecture

WordJewelsSolver is a small library of game logic with thin front ends on top
of it, and a separate layer for the few things that differ between operating
systems. This page says what is there. What is not built yet is listed at the
end and in the [roadmap](roadmap.md).

## Source layout

```text
src/
  lib/                 wjs:: the game logic. GUI-free and OS-free
    include/wjs/         board.hpp  dice.hpp  dictionary.hpp  scoring.hpp  solver.hpp
                         path.hpp  phase_data.hpp  seeding.hpp
                         grapheme_binder.hpp  shake.hpp
                         solver_work.hpp  profiling_types.hpp
                         version.hpp.in (configured into a generated header)
    src/                 board.cpp  dice.cpp  draw.hpp (internal)  dictionary.cpp
                         scoring.cpp  solver.cpp
                         path.cpp  phase_data.cpp  seeding.cpp
                         grapheme_binder.cpp  shake.cpp
    variants/            profiling-enabled/wjs/profiling.hpp   (Tracy behind it)
                         profiling-disabled/wjs/profiling.hpp  (empty)
  cli/                 main.cpp: wjs-cli
  app/                 the Qt Quick application (built only with WJS_BUILD_APP)
    qml/Main.qml         the window: a shell that shows the screen Navigation names
    qml/Home.qml         home
    qml/kit/             Theme (the tokens), Navigation, Feedback, AppButton, HoldButton,
                         Icon, Jewel, TileFace
    qml/scene/           BoardScene (the 2D placeholder of the board scene), SceneScript.js
    qml/solo/            SoloScreen (solo.box, solo.playing, solo.resting), TraceLayer,
                         PlayingPanel, SoloBox, SoloResting, HistorySheet, BoardThumb
    solo/                solo_game.hpp (the port), fake_solo_game.*, solo_model.*,
                         path_tracer.*, board_layout.*, solo_layout.*
    solver_model.*       the solver's model (no screen uses it now; it keeps its tests)
    system_colour_scheme.*  whether the system is in dark mode
  platforms/           everything that differs between operating systems
    interfaces/wjs/platform/
                         console.hpp  paths.hpp
    windows/             console_windows.cpp  paths_windows.cpp  platform.cmake
    linux/               paths_linux.cpp  platform.cmake
    macos/               paths_macos.cpp  platform.cmake
    posix/               console_posix.cpp (shared by Linux and macOS)
cmake/                 Platform.cmake (selector), PlatformContract.cmake,
                       CompilerWarnings.cmake, Sanitizers.cmake, Coverage.cmake,
                       Tracy.cmake, FindQt6.cmake
tests/                 unit/  app/  platform/  perf/  profiling/
tools/checks/          check_platform_isolation.ps1
tools/data/            check_wordlists.ps1, check_phonics.ps1 and its self-test,
                       make_dice.ps1
tools/hooks/, policy/  git hooks and the commit and PR text rule
```

## How the pieces depend on each other

```text
 wjs-cli      WordJewelsSolver (window)      wjs_unit_tests
      \                |                     /
       +---------------+---------------------+
       |                                     |
       v                                     v
   wjs::lib                            wjs::platform
   board, dictionary, scoring,         one implementation of each
   solver, path, phase data,           interfaces/ header, chosen by CMake
   seeding, grapheme binder, shake
```

- **`wjs::lib`** (target `wjs_lib`, a static library) includes no operating
  system header and no Qt. It links nothing but the warning, sanitizer and
  coverage settings. Everything that decides what a board scores lives here.
- **`wjs::platform`** (target `wjs_platform`, a static library) is the only
  target that includes OS headers. Its public interface is the headers in
  `src/platforms/interfaces/`; its sources are whichever files the selected
  platform's `platform.cmake` names. The library does not use it.
- **The front ends** link both. `wjs-cli` and the window ask the platform layer
  where the executable is (to find a word list beside it) and, in the CLI, to
  put the console in UTF-8; they hand everything else to the library.

How CMake picks the platform and keeps the OS out of the source is in
[Platform isolation](platform-isolation.md).

## The library

All of it is in `namespace wjs`.

### Board

`Board::parse` turns text into a rectangular grid of tiles. A tile holds one to
three letters, A to Z, stored upper-case; the Boggle "Qu" face is the
two-letter tile `QU`. Rows are separated by newlines or `/`; within a row, tiles
are separated by whitespace or commas, and a row with no separators is one
letter per tile. A board is at most 16 by 16, and every row has the same number
of tiles. Failures come back as a `BoardError` (`empty`, `ragged`, `bad_tile`,
`too_large`); there is no exception.

### Dictionary

A `Dictionary` is a trie of upper-case words: a node per letter, each with its
outgoing edges kept sorted by letter and looked up by binary search, and a flag
for "a word ends here". Besides `contains` and `has_prefix`, it exposes the trie
itself as a position in it (`root`, `step`, `is_word`), so a search can ask "is this a
word?" and "could this still become one?" one letter at a time without
rebuilding strings.

`insert` refuses an empty word or one with anything but ASCII letters, leaving
the dictionary unchanged. `load` and `load_file` read one word per line,
trimming whitespace and skipping any line `insert` refuses, so an apostrophe or
a hyphen drops the line rather than failing the load. A file that cannot be
opened is `DictionaryError::cannot_open`.

### Scoring

`score_word` has two schemes (`ScoringScheme`):

- `standard_boggle`: 3 or 4 letters score 1, 5 score 2, 6 score 3, 7 score 5,
  8 or more score 11, and anything shorter scores 0.
- `letter_rarity`: the scheme of the original program, kept for continuity. The
  mean over the word's letters of 100 divided by the letter's percentage
  frequency in English, truncated to an `int`.

### Solver

`solve(board, dictionary, options)` returns every dictionary word that can be
traced through neighbouring tiles without using a tile twice. It is a
depth-first walk from every tile that carries a position in the trie alongside
the path:

1. Step the trie by the tile's letters (two for `Qu`). If the dictionary has no
   word with that prefix, give up this branch at once.
2. Mark the tile used and extend the word. If the trie position is a word and the
   word has at least `min_length` letters, record it, unless that word has
   already been recorded: each word appears once, with the first path found.
3. Recurse into the eight neighbours that are on the board and unused, then
   unmark the tile.

The result is sorted by score (highest first), then length (longest first),
then alphabetically, and carries each word's path as a list of `Position`s.
`min_length` counts letters, so a `Qu` tile is two. The score of a word does
not depend on the path.

Pruning by the trie is what keeps the search small; there is no other
optimisation. The search counts its **visits**, the tile steps that stayed
inside the trie, and `wjs/solver_work.hpp` tallies them per thread, once per
solve; the performance tests hold budgets on that count
([Testing](testing.md#performance-tests)). On the corpus of 3x3 to 6x6 boards
the worst solve is about 5,200 visits and 0.9 ms; a 6x6 built to hold 1,788
words is about 9,400 visits and 1.6 ms. Nothing larger than 6x6 has been
measured.

### Profiling

`wjs/profiling.hpp` is the one place the code asks to be timed: `zone<"name">()`,
`frame_mark()`, `plot()`, `message()` and `thread_name()`, as functions and not
macros. There are two files of that name under `src/lib/variants/`, one with
Tracy behind it and one empty, and CMake puts exactly one directory on the
include path, so no source asks which it got. A normal build has the empty one.
See [Profiling](profiling.md).

### Path

`validate_path(board, path)` checks a path a finger or the solver traced against
a board: every position on the board, no tile used twice, each tile touching the
one before it (diagonals included, edges not wrapping). On success it is the word
the tiles spell, upper-case, a two-letter tile such as `Qu` adding both letters;
otherwise a `PathError` (`empty`, `off_board`, `repeated_tile`, `not_adjacent`)
for the first fault. `are_neighbours` is the one-step question, for a touch
handler that asks it a tile at a time.

### Dice

`DiceSet` is a set of dice, each with faces of one to three letters (the Boggle
`Qu` face is the tile `QU`), read from a text file, one die a line (`parse`,
`load`, `load_file`). `roll(dice, seed)` shakes them onto a square board: the
dice are put in a random order, a die to a tile, and each shows one face. It is a
pure function of the set and the seed, with the library's own `Draw`
(`std::mt19937` taken modulo, and a shuffle that is not `std::shuffle`), so a seed
is the same board on every compiler. The set must hold a square number of dice, so
the 3x3 is its own set and not a window on the 4x4.

The library holds no set. The project's are `data/dice/<n>x<n>.txt` for 3x3 to
6x6, made by `tools/data/make_dice.ps1` from the dictionary's letter counts: six
different letters to a die dealt in proportion to the counts, a vowel on each, a
Q shown as `QU`, a seed to each size, and the source's hash and the method in the
header ([Data](https://github.com/iainchesworthlabs/wordjewelssolver/blob/develop/data/README.md)).
`data.dice` holds the files to what the tool makes. The faces of a printed set are
not used: in the scanner mode the board is read from the table.

### Phase data

A phonics phase is data: which graphemes it teaches and which words it offers,
each word split into graphemes. The library holds no phase of its own and knows
no letter, sound or word of one; the files are written and reviewed by a person
qualified to do it, and **none exists in the repository** (the tests carry a
dozen words of test data of their own). `phase_data.hpp` reads and checks them.

Two kinds of file, in `data/phonics/`, one line each, `#` lines being the header
or a comment:

```text
# phase: A
# reviewed-by: <name>   reviewed-on: <date>   source: <where it came from>

phase-<x>.graphemes.txt     ai  /sound/  clip:ai
phase-<x>.words.txt         pain  p ai n
                            <word>  tricky
```

- A grapheme is lowercase letters, its sound is written between slashes (any
  bytes, so a phonetic alphabet passes through), its clip id is `clip:<id>`.
  Two graphemes may share a clip.
- A word is three or more lowercase letters (nothing shorter can be counted)
  followed by its graphemes, or by `tricky` and nothing else: a word that does
  not decode, spoken whole and never sounded out. The graphemes, joined, spell
  the word.
- **A split digraph** is written as its two halves around the letters between,
  `cake  c a_ k _e`, and is the grapheme `a_e` in the graphemes file. The halves
  are one grapheme: `WordGrapheme` carries where each half sits in the word
  (`first` and `second`). It is not found while tracing (below).
- **A grapheme that makes more than one sound** is listed once for each sound,
  and a word line says which, counting from 1 in the order of that grapheme's
  lines: `snow  s n ow:2`, `a_:2` for a split digraph. A grapheme with several
  sounds has to be told which; with one, the number is optional. A later phase
  may add a sound to a grapheme, and an earlier phase's words cannot use it.
- Both files are sorted in byte order, and hold no word twice, and no grapheme
  with the same sound twice.
- A header is `key: value` pairs, several to a line. A file with no
  `reviewed-by`, `reviewed-on` (a date, `YYYY-MM-DD`) and `source` is **test
  data**: `PhaseHeader::reviewed()` and `PhaseSet::reviewed()` say so, and a
  release build refuses it
  ([Testing](testing.md#phonics-data-the-checker-and-the-review-gate)). A value
  still in `<angle brackets>` has not been given. Test data need not say where
  it came from; a file that claims a review has to.

`parse_words` and `parse_graphemes` read one file and report every problem, with
its line, not the first. `PhaseSet::build` (or `load_directory`, which reads
`phase-<x>.*.txt` from a directory) holds the files together to the checks of
the design: each word is in the English dictionary, each grapheme is in the
word's phase or an earlier one and a grapheme with several sounds there is told
which, nothing is listed twice across phases. (That the graphemes spell the word
is held at parsing, where the line is.) Phases are **cumulative** and ordered by name: a `PhaseIndex` holds the
words and graphemes of its phase and every earlier one, with a `Dictionary` of
the words, which is what a board is solved against, and the phase's `alphabet()`.
`tools/data/check_phonics.ps1` applies the same checks outside the build.

### Seeding

`seed_board(index, options, recent, seed)` makes the board of Mode B. It is a
pure function of the phase data, the options, the recent targets and a seed:

1. **Choose** two or three target words of the index. Words with a grapheme that
   makes a focus sound come first, and within each of those and the rest, words
   that are not among the recent targets come first. A third target is taken
   only from the focus words while the targets are all focus words.
2. **Plant** each as a path of touching tiles, a tile shared where the letter
   agrees, leaving room for the dead jewels.
3. **Fill** the other tiles: `dead_cells` of them with the dead letter, the rest
   from the filler (the phase's alphabet by default).
4. **Verify** with `wjs::solve` against the phase's dictionary. Three to five
   words is kept. Otherwise another board is made, up to `max_tries` (20)
   verifications in all, and the nearest is returned with `fallback` set.

Two options make fuller boards (measured in
[Testing](testing.md#fuller-boards-a-guided-fill-and-short-targets)):
`max_target_letters` limits the targets to short words, and `guided_fill` gives
each tile left a letter the solver says keeps the count in range, making a tile
dead only if no letter fits, with the planted words kept apart where there is
room. Both are off by default, and `phonics_board_options(index, size)` is the
two together with every letter but one as filler, which is the phonics board as
decided: the phase's letters first, others only where a letter of the phase
would make too many words, and no dead jewel on a board that lands.

Dead jewels are a letter no word of the phase uses, chosen by the caller and
checked (`bad_dead_letter`); the library's `Board` stays A-Z and the application
draws that letter as a jewel. Draws are `std::mt19937` taken modulo and shuffles
are the library's own, never `std::uniform_int_distribution` or `std::shuffle`,
so a seed is the same board on every compiler. `RecentTargets` is the ring
buffer of the last ten boards' targets, which the caller keeps. What was
measured of its share and cost is in
[Testing](testing.md#seeding-what-was-measured).

### Grapheme blocks

`GraphemeBinder` binds a traced path into blocks as the finger moves: the
letters of the tiles are matched greedily, longest first, against the
multi-letter graphemes of the phase, and each run of whole tiles that spells one
is a block. A tile that starts no grapheme is a block of its own, and a grapheme
cannot start or end inside a tile. The brief's example: R, A, I, N is `R [AI] N`.
A split digraph is not found in flight (it cannot be known until its last
letter); the design draws it from the word's authored split, which the data
holds as the two halves and where each sits (`WordGrapheme::first` and
`second`), and the binder leaves such a grapheme out.

### Shake detection

`detect_shakes(samples, options)` is a pure function over `(t, ax, ay, az)`
accelerometer samples. Gravity is followed by a low-pass filter and taken away;
a jolt is the remainder going over a threshold (1.2 g), jolts must alternate
direction and come within 0.4 s of each other, and four make a shake. The result
is each shake's start, end, jolt count and peak, and whether the trace ended while
it may still be going, which is what a live caller asks of a short window of the
latest samples. A knock, turning the device over, tilting it slowly, walking and a
gentle wave are not shakes. The numbers are starting values, tuned on synthetic
traces and not on a device. Reading the sensor is an operating-system service and
belongs behind an interface in `src/platforms`, which is not written.

### Errors

An expected failure (a board that cannot be read, a word list that cannot be
opened) is a `std::expected<T, E>` value with a small enum for `E` and a
`to_string` for it. The library does not throw for them.

### The version header

`src/lib/include/wjs/version.hpp.in` is configured by CMake into a generated
header that holds `wjs::project_version` and `wjs::target_platform` as
`constexpr` values. That is how the version and the name of the platform reach
the code without a macro, and why `wjs-cli --version` can print them.

## The platform layer

An OS service the program needs is declared in a header under
`src/platforms/interfaces/wjs/platform/`, and defined once per platform:

| Interface | What it does | Windows | Linux | macOS |
| --- | --- | --- | --- | --- |
| `paths.hpp`: `executable_path()` | The absolute path of the running program, so data beside it can be found | `GetModuleFileNameW` | `/proc/self/exe` | `_NSGetExecutablePath` |
| `console.hpp`: `use_utf8_console()` | Make the console read and write UTF-8 | `SetConsoleCP`, `SetConsoleOutputCP` | nothing to do (`posix/`) | nothing to do (`posix/`) |

Linux and macOS share `posix/console_posix.cpp`, named by both
`platform.cmake` files.

## The Qt Quick window

`src/app` is the Qt Quick application and is optional (`WJS_BUILD_APP`, off by
default; it needs a prebuilt Qt 6.12). It is Solo Play, Mode B, of
[Application design](design/application.md) and [Game design](design/game.md):
`home`, and `solo.box`, `solo.playing` and `solo.resting` with the moments
between them (shaking, dropping, word-found, cleared) on one screen, because the
board scene is one component that stays while the lid lifts and the dice fall.
The library knows nothing of any of it, and no game rule is in the QML.

| Layer | Where | What it is |
| --- | --- | --- |
| The port | `solo/solo_game.hpp` | `SoloGame`, a Qt-free abstract class in the names of the game design: the states of the flow (box, shaking, dropping, playing, cleared, resting), `shake`, `settled`, `submit(path)` answered as viable, real-but-not-today's or not-a-word (the last two carry no word), `startTile`, `lidOn`, `again`, the tray and the history. It decides nothing |
| The fake | `solo/fake_solo_game.*` | A stand-in for the library's session until it is wired, and for the tests. Its words are test data: today's are the five early-reader words the design names, a few more are real, and the boards are two fixed grids turned by the seed. It publishes no phonics list |
| The model | `solo/solo_model.*` | `SoloModel`, the QObject the screens bind to: the state as a string (the design's ids without `solo.`, including `word-found`, which is presentation and so the model's and not the game's), the board, the path being made and its grapheme blocks, the tray, the start tile, the history. It turns pointer input into moves with `PathTracer` and says every moment that wants a sound or a tap as a `cue` |
| The touch logic | `solo/path_tracer.*` | Trace and tap in order, both always on. A hit circle smaller than the tile and a larger one it must leave, so a diagonal slide takes the diagonal tile and a wobble near an edge does nothing; a fast move is walked in small steps so no tile is jumped; sliding back takes the last tile off; lifting outside the board cancels; a tapped path ends with the last tile tapped again or the Done jewel; a trace may carry on from a tapped path; a dead jewel is never in a path. The radii are starting values for a small finger and are the one thing the tests cannot settle |
| The layout rule | `solo/board_layout.*`, `solo/solo_layout.*` | [Board design](design/board.md#layouts-and-sizes) as a pure function, tested on the page's reference windows, and one copy for the screens and the scene. A size is offered only if its tile is 48 dp or more |
| The board scene | `qml/scene/BoardScene.qml` | One component: given a finished board and a seed, play the shake, the lid and the drop and say when the dice are settled. The 2D placeholder, with the members of [Shake prototype](design/shake-prototype.md#the-interface); the 3D scene being built apart plugs in at the same seam. `app.scene_contract` holds both files to the list. An empty board is the box with its lid on |
| The kit | `qml/kit/` | `Theme` has every colour, size, time and type size a screen uses (light and dark, reduced motion, text scale); a screen writes none of its own, so a school font or a world is a token swap. `AppButton` has the three targets (48, 64 and 96 dp), acts on release inside it and takes the keyboard. Nothing drawn has text in it |

The two-thirds and one-third split is applied to the window below a 72 dp bar
that holds the way back and the quiet corner buttons, so neither lies over the
board; so the tiles are a little smaller than the reference table's (a 4x4 on
a 1280 x 800 window is 164 dp and not 182). Back goes where the person came from,
with the design's exceptions, and Esc and the system back do what the visible
control does.

**What is not there.** No sound or touch feedback: every moment is a
`Feedback.cue`, and nothing listens, because Qt Multimedia, Sensors and
TextToSpeech are not in the tree and the recordings wait for the literacy review.
No device shake: `wjs::detect_shakes` exists and the sensor interface in
`src/platforms` does not, so Shake is a button (and Space) everywhere. No bundled
type: Andika is named in `Theme` and Qt falls back to the platform's face until
its licence is read and the glyph check is done. The family game's picture and the
parent's button are on `home` and are shown only when `Navigation.scanAvailable`
and `Navigation.parentAvailable` are set, by whoever adds those screens. The
history is the fake's until the store exists.

**Wiring the library's session.** `make_solo_game()` in `solo/solo_game_factory.cpp`
is the one place that decides which game is played. The session
(`wjs::GameSession`) maps to the port: `shake()`, `still()` and `landed()` (the
port's `settled()` calls the last two), `trace(path, stamp)`, `rest()`, `back()` for
`lidOn()`, `again()`, `request_help()` and `lit_tile()`; the dead jewels are its
`dead_tiles()`; the grapheme blocks are `GraphemeBinder` with the phase; the best
for each size, the first-time sticker and the history come from the parent area's
store.

## Build targets

| Target | Kind | Notes |
| --- | --- | --- |
| `wjs_lib` (`wjs::lib`) | static library | The game logic |
| `wjs_platform` (`wjs::platform`) | static library | The platform's implementations |
| `wjs_cli` | executable `wjs-cli` | `WJS_BUILD_CLI` |
| `wjs_app` | executable `WordJewelsSolver` | `WJS_BUILD_APP` |
| `wjs_unit_tests` | executable | Catch2 |
| `wjs_perf_tests` | executable | Catch2; work budgets and opt-in wall-time checks, linked to `wjs::lib` only |
| `wjs::tracy` | interface library | Tracy's client when `WJS_ENABLE_TRACY` is on, otherwise nothing |
| `wjs_app_tests` | executable | Catch2, with `WJS_BUILD_APP` |
| `wjs::warnings`, `wjs::sanitizers`, `wjs::coverage` | interface libraries | Flags, linked into every first-party target |

## Limits of what exists

- Words and tiles are the letters A to Z. There is no support for accented
  letters or other scripts, and a word list line with one is skipped.
- One word list is loaded for a run; there is no per-language handling.
- The board is at most 16 by 16 and a tile holds at most three letters.
- The only front ends are the CLI and the small window.

## Not built yet

These do not exist, and nothing in the tree stands in for them:

- **Camera input.** Cameras are an OS service, so by the rule in
  [Platform isolation](platform-isolation.md) any camera API will sit behind an
  interface in `src/platforms/interfaces/` with an implementation per platform.
  How is not designed.
- **Reading letters from a picture.** The seam is the board: whatever reads
  the picture has to produce the text `Board::parse` accepts, or a `Board`.
  The recognition stack is not chosen, and its choice carries licensing
  questions (see [Licensing](licensing.md)).
- **Phonics phase data.** The reader, the checks, the seeding, the binder and the
  shake detector exist and are tested, and no front end uses them yet. No phase
  file exists, and none is written here: the content has to be written and
  reviewed by someone qualified to do it before a child uses it. The audio
  clips, the sound-it-out sequence and the dashboard are not built.
- **Using the child-facing word lists.** `data/lexicon/` holds category and
  early-reader tier lists (animals, colours, shapes, and so on); nothing reads
  them yet. The [roadmap](roadmap.md) (M2) covers the lexicon and hints.
- **Packaging and installers**, and **phone and browser targets**.

The [roadmap](roadmap.md) has the plan and its order.
