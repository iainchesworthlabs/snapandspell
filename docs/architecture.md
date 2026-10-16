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
                         grapheme_binder.hpp  shake.hpp  game_rules.hpp
                         game_session.hpp
                         calendar.hpp  settings.hpp  store.hpp  session_store.hpp
                         parent_gate.hpp  dashboard.hpp
                         solver_work.hpp  binder_work.hpp  profiling_types.hpp
                         version.hpp.in (configured into a generated header)
    src/                 board.cpp  dice.cpp  draw.hpp (internal)  dictionary.cpp
                         scoring.cpp  solver.cpp
                         path.cpp  phase_data.cpp  seeding.cpp
                         grapheme_binder.cpp  shake.cpp  game_rules.cpp
                         game_session.cpp
                         settings.cpp  store.cpp  store_format.cpp  session_store.cpp
                         parent_gate.cpp  dashboard.cpp
    variants/            profiling-enabled/wjs/profiling.hpp   (Tracy behind it)
                         profiling-disabled/wjs/profiling.hpp  (empty)
  scan/                wjs::scan: finds a board in a camera frame and reads its
                       letters. Portable and Qt-free; links wjs::lib for Board
    include/wjs/scan/    frame.hpp  image.hpp  geometry.hpp  segment.hpp
                         board_finder.hpp  stroke_font.hpp  glyph_reader.hpp
                         reading.hpp  reader.hpp  confirmation.hpp  work.hpp
    src/                 one .cpp for each
  cli/                 main.cpp: wjs-cli
  app/                 the Qt Quick application (built only with WJS_BUILD_APP)
    qml/Main.qml         the window: a shell that shows the screen Navigation names
    qml/Home.qml         home
    qml/kit/             Theme (the tokens), Navigation, Feedback, AppButton, HoldButton,
                         Icon, Jewel, TileFace
    qml/scene/           BoardScene (the 2D placeholder of the board scene), SceneScript.js
    qml/solo/            SoloScreen (solo.box, solo.playing, solo.resting), TraceLayer,
                         PlayingPanel, SoloBox, SoloResting, HistorySheet, BoardThumb
    solo/                solo_game.hpp (the port), real_solo_game.* (over the library's
                         session), solo_game_setup.* (what it is made from),
                         solo_game_factory.*, fake_solo_game.*, solo_model.*,
                         path_tracer.*, board_layout.*, solo_layout.*
    solver_model.*       the solver's model (no screen uses it now; it keeps its tests)
    system_colour_scheme.*  whether the system is in dark mode
    parent/              the parent area's controller and its QML module (CMake)
    qml/parent/          the parent area's screens and their wording
  platforms/           everything that differs between operating systems
    interfaces/wjs/platform/
                         console.hpp  paths.hpp  camera.hpp  board_reader.hpp
    detail/              what no platform differs in: camera_unavailable.cpp,
                         board_reader_portable.cpp
    windows/             console_windows.cpp  paths_windows.cpp  platform.cmake
    linux/               paths_linux.cpp  platform.cmake
    macos/               paths_macos.cpp  platform.cmake
    posix/               console_posix.cpp (shared by Linux and macOS)
cmake/                 Platform.cmake (selector), PlatformContract.cmake,
                       CompilerWarnings.cmake, Sanitizers.cmake, Coverage.cmake,
                       Tracy.cmake, FindQt6.cmake
tests/                 unit/  app/  app/parent_ui/  platform/  perf/  profiling/  scan/
                       privacy/  parent/
tools/checks/          check_platform_isolation.ps1
tools/scan/            render_corpus.cpp: wjs_scan_render, writes the synthetic corpus
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
   wjs::lib  <---- wjs::scan  <----    wjs::platform
   board, dictionary, scoring,    finds a board in a      one implementation of each
   solver, path, phase data,      frame and reads it      interfaces/ header, chosen
   seeding, grapheme binder,      (no dependency)         by CMake
   shake, game rules,
   game session
```

- **`wjs::lib`** (target `wjs_lib`, a static library) includes no operating
  system header and no Qt. It links nothing but the warning, sanitizer and
  coverage settings. Everything that decides what a board scores lives here.
- **`wjs::platform`** (target `wjs_platform`, a static library) is the only
  target that includes OS headers. Its public interface is the headers in
  `src/platforms/interfaces/`; its sources are whichever files the selected
  platform's `platform.cmake` names. The library does not use it.
- **`wjs::scan`** (target `wjs_scan`, a static library) finds a board in a camera
  frame and reads each die's letter. It includes no operating system header and no
  Qt, and **adds no third-party dependency** (`vcpkg.json` is unchanged). It links
  `wjs::lib` for `Board` and nothing else; `wjs::lib` does not link it, so the
  solver stays free of image code. `wjs::platform` links it, because the camera and
  `board_reader` interfaces are written in terms of its frame and reading types.
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
([Testing](testing.md#performance-tests)). The grapheme binder tallies its
comparisons the same way (`wjs/binder_work.hpp`), once per bind, and the seeding
reports its verifications and solves in its result. On the corpus of 3x3 to 6x6 boards
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
`load`, `load_file`). `roll_with_placements(dice, seed)` shakes them onto a square
board: the dice are put in a random order, a die to a tile, and each shows one
face. It returns a `RolledBoard`: the `Board` and, for each tile row-major, a
`Placement` with the index of the die on it and the index of its face that is
up, so `dice.die(p.die)[p.face]` is the tile's letters. A scene that draws the
dice, and tumbles them to show their other faces, takes the faces from
`dice.die(p.die)` and the face that is up from `p.face`; the game reads only the
board. `roll(dice, seed)` is the same roll returning the board alone, for a
caller that does not draw dice. A board from `deal_standard_board` is the roll of the
seed it reports (`DealtBoard::seed`), so `roll_with_placements(dice, dealt.seed)` gives
the dice of a dealt board. Both are pure functions of the set and the seed,
with the library's own `Draw` (`std::mt19937` taken modulo, and a shuffle that is
not `std::shuffle`), so a seed is the same board and the same placements on every
compiler, and a seed that gave a board before the placements were returned gives
it still. The set must hold a square number of dice, so the 3x3 is its own set
and not a window on the 4x4.

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

### Game rules

`game_rules.hpp` is what the touch handler and the tray call to ask a board in
play four questions. It draws nothing, plays nothing and keeps nothing between
boards; the application keeps the answers.

The questions are put to a **word set**, `WordSets{dictionary, active}`, because
the first release has two board modes and the rules do not know which they are
on (decided by the author, 2026-10-16; see [MVP library](design/mvp-library.md)).
On the **standard board**, a roll of dice, `active` is the full dictionary
itself (`WordSets::standard`). On the **phonics board**, the parent's switch,
`active` is the words of the active `PhaseIndex` (`WordSets::phonics`). Both are
held by reference and must outlive the `WordSets`.

**What a traced path is.** `classify_path(board, path, words)` checks the path
with `validate_path` and answers one of three states, never "wrong":

| `WordState` | When | `word` | The application |
| --- | --- | --- | --- |
| `viable` | the word has three letters or more and `active` has it | the word, upper-case | a jewel, stars, the sound-it-out sequence |
| `real_not_todays` | three letters or more, in `dictionary` and not in `active` | empty | the neutral cue: no star, no word shown or read |
| `not_a_word` | anything else: a word of two letters or fewer, a string that is no word, a path `validate_path` refuses | empty | the path fades |

On the standard board `active` and `dictionary` are one, so the middle state
cannot arise. A refused path is `not_a_word` with the `PathError` in `fault`.
The word is withheld from the two states that are not viable on purpose: the
design never shows or reads a word that is not one of today's, and the library
does not hand the application one to show. `viable_words(board, words)` is the
solve of the board against `active` at the minimum length, the list the other
two rules read; a path is viable exactly when its word is in it.

**Tier and stars** come from the letter count and are never stored:
`tier_for_letters(n)` (a `Qu` tile counts two, so the word's `size()` is the
count) and `reward_for(tier)`:

| Letters | `Tier` | Stars | Sticker |
| --- | --- | --- | --- |
| 3 | `spark` | 1 | no |
| 4 | `power` | 2 | yes, the first time the word is found on this device |
| 5 or more | `mega` | 5 | no (its accessory is after the MVP) |

Fewer than three letters have no tier. Whether this is the first time a word
is found on the device is the application's to know; `Reward::sticker` says
only that the tier carries one. The Mega accessory is not modelled.

**The words found on a board** are a `FoundWords`: each once, whatever its
case, in the order found. `insert` returns whether the word was new, which is
when the application awards stars; a word traced again on the same board is
`viable` and `insert` says false.

**The start-tile help.** `start_tile_help(viable, found)` is the start tile of
one viable word not yet found, chosen as a total order so that nothing is
remembered and the same press gives the same answer:

1. the shortest word, counted in letters (QUIT is four);
2. then the word whose first tile is nearest the top-left tile, by
   straight-line distance (row squared plus column squared, an integer), the
   upper tile winning a tie and then the left one;
3. then the word that sorts first.

The tile is the first of the path the solver reported, which is the first start
tile in reading order that can trace the word, so the word can be traced from
the tile that lights. Because the answer is the least of the words left in a
fixed order, finding any other word cannot move it: it stays until its word is
found, and then the same call names the next. It is `nullopt` when no viable
word is left.

**Cleared.** `is_cleared(viable, found)` is true when no viable word is left
unfound. A board with no viable word at all is cleared in that literal sense,
and none is meant to be dealt: the board is decided and checked before the
shake, so a child never lands on one (the author, 2026-10-16). A phonics board
always holds its planted targets, so `seed_board` already keeps to that (a case
pins it); the standard board's deal does it with `deal_playable`, below, and
`deal_standard_board`, which is that over `wjs::roll`.

**The deal.** `deal_playable(words, make_board, seed, options)` calls
`make_board(seed)`, solves what it made with `viable_words`, and keeps the first
board that holds `options.min_viable` words (default 1); otherwise it asks
`make_board` again with `next_deal_seed`, the first output of `std::mt19937`
seeded with the last, which the standard fixes, so a seed is the same deal on
every compiler. At most `options.max_rolls` boards (default 20) are made. It
answers a `DealtBoard`: the board, the viable list (what `start_tile_help` and
`is_cleared` take, so the application does not solve again), the boards made and
the seed of the one kept. Nothing is thrown: a zero `min_viable` or `max_rolls`,
or no maker, is `DealError::bad_options`; a maker that fails ends the deal as
`bad_board`; `max_rolls` boards with none holding enough words is `no_board`.

`deal_standard_board(dice, words, seed, options)` is that over `wjs::roll`, with
a `DiceError` read as `bad_board`. It is needed because `wjs::roll` does not
look at what it rolled: measured 2026-10-16 with the project's dice and the full
dictionary, 3,830 of 100,000 3x3 rolls (3.8%) and 30 of 20,000 4x4 rolls
(0.15%) hold no word of three letters or more, and none of 5,000 5x5 or 2,000
6x6 rolls do. The default twenty boards is there for a word list far thinner
than the dictionary: twenty empty 3x3 rolls in a row are about one in 10^28.

What is the library's reading of the design, and what the design left open, is
on [Testing](testing.md#game-rules-what-is-pinned).

### Game session

`game_session.hpp` is the state machine of Solo Play (Mode B, the phonics board)
that the screens bind to. One class, `wjs::GameSession`, deals a board, takes a
traced path and answers it, keeps the finds and says when the board is cleared.
It holds no Qt, plays no sound, draws nothing and **reads no clock**: what varies
is handed in, which is the seed of the deal (`SessionOptions::seed`), the moment
of a find (`Stamp`, an `int64` the caller gives to `trace` and gets back, never
read or ordered) and the pace of the presentation, which is the screens' (they
tell it when the shake, the drop and the celebration are over). The same options
and the same events do the same on every compiler. It persists nothing: it hands
the store plain values (below) and takes one back. It is single-threaded and
holds the phase data and the dictionary by reference, which must outlive it.

```cpp
auto session = wjs::GameSession::create(phase, english, {.size = wjs::BoardSize::three,
                                                         .seed = random_seed});
session->shake();                         // box -> shaking: the board is dealt here
session->still(); session->landed();      // shaking -> dropping -> playing
auto outcome = session->trace(path, today);  // viable / real, not today's / not a word
auto tile = session->request_help();      // the start tile, never the word
```

**The states** are those of [Game](design/game.md#the-flow), in its ids
(`to_string(SessionState)` gives `"solo.box"` and so on). `solo.word-found` is
not one: it is the few moments of presentation after a find, when the session is
already `playing` (or `cleared` after the last word), so the session accepts a
trace through it. Every event returns whether it applied, and one that does not
leaves everything as it was, so a screen may send it late or twice.

| Event | From | To |
| --- | --- | --- |
| `choose_size(BoardSize)` | `box` | `box` (3x3 or 4x4; only here) |
| `shake()` | `box` | `shaking` (deals the board; an error leaves the box) |
| `still()` | `shaking` | `dropping` (how long it shook is the screens') |
| `landed()` | `dropping` | `playing` |
| `trace(path, stamp)` | `playing` | `playing`, or `cleared` after the last viable word |
| `request_help()` | `playing` | `playing` |
| `rest()` | `cleared` | `resting` (about 3 s after the last jewel, the screens' timer) |
| `again()` | `resting` | `box` |
| `back()` | `shaking`, `dropping`, `playing` | `box` (the lid goes back on) |

From `cleared` and `resting` the screens leave Solo Play without the session
(`Back` there is `home`, not the box), so `back()` is refused.

**What a trace is.** `trace` answers with a `TraceOutcome`:

| `TraceKind` | When | `find` | Stars |
| --- | --- | --- | --- |
| `found` | a word of the phase's index, on the board, not found yet | the `Find` just recorded | those of its tier (`stars()`; `sticker()` if the tier carries one) |
| `found_again` | the same, already in the tray | the earlier `Find` | none |
| `real_not_todays` | a real word that is not one of today's, **of any length** | none | none |
| `not_a_word` | anything else, including a path that is not a path | none | none |
| `ignored` | not in `playing`, or a path of fewer than two tiles | none | none |

The classification is `classify_path` of [Game rules](#game-rules) against
`WordSets::phonics`, with two readings of the session's own:

- **A real word is never `not_a_word`.** The rules call a word of two letters
  `not_a_word`, because it cannot count and has no tier. The session reads the
  spelling again and answers a real two-letter word (`at`, `in`) as
  `real_not_todays`: the soft tick, no word. A child who is told a real word is
  not one has been taught something false, and the answer names nothing either
  way.
- **A path of one tile, or none, is `ignored`**, not a word attempt. A touch on
  a tile is to hear its letter ([Game](design/game.md#tracing-and-tapping)), and
  a trace cancelled by lifting outside the board is no attempt; the dashboard
  design drops "emerging" for the same reason. The cue for a touch is its
  letter's sound, which is the screens'.

`Find` is `{word, assisted, when}`, with `tier()` and `reward()` derived from the
letter count and never stored. `word` is upper-case and is a word of the phase,
because a word is named only when it is: `real_not_todays` and `not_a_word` carry
none, and nothing else the session hands out (the record, the snapshot, the
tray, the help) holds a word outside the phase index.

**The help.** `request_help()` is the start-tile help of the rules: the tile of
the shortest viable word not yet found, nearest the top left, the same one until
that word is found. It returns a position, not the word (the child does the
finding). The tile stays lit (`lit_tile()`) until its word is found, and a word
found while its tile is lit is an **assisted** find (`Find::assisted`), which is
what the dashboard counts as not unassisted. `SessionOptions::help` is the
parent's switch; off, it returns nothing.

**The deal.** `shake()` seeds the board before anything moves, with
`seed_board` and the options of `phonics_board_options` for the size (targets of
three letters, the guided fill, no dead jewels on a board that lands), the
session's range (`min_viable` 3, `max_viable` 5) and the parent's focus sounds.
Seeding lands three to five words only some of the time (23% of 3x3 seeds and
92% of 4x4 seeds on the 249-word proxy of
[Testing](testing.md#seeding-what-was-measured)), so the session **seeds again,
with the next seed of its stream, up to `max_seedings` (5) times** and keeps the
board nearest the range, which is the first that lands. A board that holds no
word is never dealt: the seeding plants its targets, and a board that held none
would be `no_board`. `last_deal()` reports how it went (`DealReport`: seedings,
verifications, solves, the words held, `in_range`). The stream is
`std::mt19937`'s first output, so a seed is one run of boards on every compiler;
the targets of recent boards are remembered within a session
(`RecentTargets`) so the next board is not made of the same words.

**What the store keeps.** Two plain values, strings, numbers and flags only, so
any file format will do:

- `record()` is a `BoardRecord`: the phase, the size, the tiles as
  `Board::parse` reads them (`"P A T/B I N/B B B"`; the dead letter is the first
  of A-Z that no word of the phase uses), **every word of the phase on the board**
  (`viable`, upper-case, alphabetical) and the finds with their `when` and
  `assisted`. It is what the history and the dashboard are made from
  ([Dashboard](design/dashboard.md#the-store)): "met" and "found" are counted
  from `viable` and `finds`. It is there from the moment the dice land, and stays
  after `back()` or `again()` until the next shake, so the store saves it when a
  board ends however it ends. Nothing but a board that reached `playing` has one.
- `snapshot()` is a `SessionSnapshot`: the state, the size, the seed the next
  board is dealt from, the board in play (a `BoardRecord`) and the word whose
  start tile is lit. `GameSession::restore(snapshot, phase, dictionary, options)`
  brings the session back. Only `box`, `playing` and `resting` are saved:
  `shaking` and `dropping` save as the box (the board was never seen, and the
  seed has moved on), `cleared` as `resting`. A restore **solves the board again**
  against the phase, so it refuses a snapshot of another phase (`wrong_phase`),
  a board that no longer holds the words recorded (`board_changed`) and one that
  is not a snapshot of a session (`bad_snapshot`: a find that is not on the
  board, a word twice, a state that does not fit its board). It does not keep
  the recent targets, so the board after a restored one may be made of words the
  original would have avoided.

**Not the session's.** Which words are stickered on the device and the best
stars for a size are the store's to know (the session has the board's `stars()`
and the tier's `sticker()`, and nothing across boards). History is the store's
list of records. The grapheme blocks of a path are `GraphemeBinder`'s, called
by the screens. Time, sound and animation are theirs. The standard board
(the default) is a roll of dice and is not dealt by this class: the session is
the phonics board's, and the deal is the one function in it that knows which
board it is.

### The store

`Store` is what the application keeps between runs (docs/design/dashboard.md,
"The store"): a small versioned text file in one place and nothing else. It holds
each find (word, board size, phase, **day**, whether the start tile was shown, which
board), the last twenty boards, the stars, the best of each size, the words found at
least once (the vault), a tally for each of the last 35 days, the settings, and the
game in progress; no duration, time of day, identity, location, image or sound. Every
list has a bound and the file never exceeds 128 KiB. The first line is the version
and the last a checksum; an older file is migrated a step at a time (`MigrationStep`)
and kept beside the new one, a damaged file is moved aside whole and never
overwritten, and a newer file is left alone and not saved over. A save is written
whole and renamed into place. The game session hands it plain values
(`BoardSnapshot`, `SavedSession`), and a find's moment is a `Day` from
`wjs/calendar.hpp` (days since 1970-01-01, the device's calendar, with the week by
the locale's first day). Where the file goes is `wjs::platform::user_data_directory()`.
`session_store.hpp` is the copy between the game session's `BoardRecord` and
`SessionSnapshot` and the store's values, and does what the session's `restore()` needs on
the way back (upper-case words in alphabetical order, the session's own tile text).
See [The parent area as built](design/parent-area.md#the-store).

### The parent gate

`Gate` (`wjs/parent_gate.hpp`) is the rule of the parental gate as logic: two touches,
one in each of two circles a diagonal apart, held two seconds; any other touch, a lift
or a slide out starts the hold over. It has no clock of its own: it is told the time
with each touch. An accessibility action opens it with no hold. It keeps a child out
of the settings by accident and is not security.

### The dashboard

`build_dashboard(store data, phase index, today, first day of the week)` computes what
a parent sees, as counts: this week (boards cleared, stars, new words, the longest
word), a row for each sound of the phase (boards it was met on, boards it was found
on, distinct words found, found without the start tile, often found), the vault and
the best. It words nothing: the screens do, from one file. With no phase data there are
no sounds.

### The scanner

`wjs::scan` (`src/scan`) reads a real board for Mode A: `read_board(frame, options)`
finds the board in a camera frame (the largest convex quadrilateral of a lattice of
dice), draws it square, counts and centres the cells, and reads each cell's one
rotated glyph by template matching against the project's own stroke font, answering a
`BoardReading`: letters, a confidence and runners-up for each tile, and which tiles
the reader doubts. A reading is not a `Board`; the one way to a `Board` is
`BoardDraft::confirm()`, the confirm-and-correct model the check screen binds to, so
the solver sees only a board a person has looked at. It is a pure function of the
frame, writes nothing, and is held to a numeric accuracy target on a synthetic
corpus. [Scanner design](design/scanner.md) is the page for all of it, including
what is **not** known (real dice, real light, the iPad).

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
| `paths.hpp`: `user_data_directory()` | The per-user, per-device directory a program keeps its own data in | `%LOCALAPPDATA%` | `$XDG_DATA_HOME`, else `~/.local/share` | `~/Library/Application Support` |
| `console.hpp`: `use_utf8_console()` | Make the console read and write UTF-8 | `SetConsoleCP`, `SetConsoleOutputCP` | nothing to do (`posix/`) | nothing to do (`posix/`) |
| `camera.hpp`: `enumerate_devices`, `start`, `stop`, `try_pop_latest_frame`, ... | Capture frames for Mode A | no camera backend yet (`detail/`): `unavailable` | the same | the same |
| `board_reader.hpp`: `read(frame, options)` | Read a board from a frame | the portable reader (`detail/`) | the same | the same |

Linux and macOS share `posix/console_posix.cpp`, named by both
`platform.cmake` files; all three name the two files in `detail/`, which are what no
platform differs in yet. An iOS camera and a Vision-based reader are Objective-C++ under
`src/platforms/ios`, which does not exist ([Scanner design](design/scanner.md#what-is-left-for-a-mac-and-an-ipad)).

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
| The real game | `solo/real_solo_game.*` | `RealSoloGame`, the port over `wjs::GameSession`: it turns the session's names into the port's and is the one place that hands the session's records to the store. It decides nothing. See *Wiring the library's session* below |
| The setup | `solo/solo_game_setup.*` | `SoloGameSetup`: loads the word list, the phases of a data directory and the store, owns them for the run, and makes a `RealSoloGame` from the parent's settings. Qt-free |
| The fake | `solo/fake_solo_game.*` | A stand-in for the library's session, for the tests and for the program when it is given no phase. Its words are test data: today's are the five early-reader words the design names, a few more are real, and the boards are two fixed grids turned by the seed. It publishes no phonics list |
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
and `Navigation.parentAvailable` are set, by whoever adds those screens. No
phase is played unless the program is given one (below), so by default the
screens run on the fake.

**Wiring the library's session.** Done: `RealSoloGame` is the port over
`wjs::GameSession`, and `make_solo_game()` in `solo/solo_game_factory.cpp` is still
the one place that decides which game is played. It answers with the maker the
program installed (`ScopedSoloGameMaker`, in `main.cpp`), and with the fake when
there is none. The mapping, which `tests/app/real_solo_game_test.cpp` holds down:

| Port | Session | And the store |
| --- | --- | --- |
| `chooseSize(side)` | `choose_size()`, in the box only | `Settings::board_size` (the last used), by `Store::update_settings` |
| `shake()` | `shake()`; an error (`no_board`) stays in the box and answers `false` | |
| `settled()` | `still()`, then `landed()`: each applies in its own state and ignores the other | |
| `submit(path)` | `trace(path, today)`: `found` and `found_again` are viable (`alreadyFound` for the second), `real_not_todays` and `not_a_word` keep their names, `ignored` is `notAWord`; the word only for a viable one, lower-case | `Store::record` after each find: `newBest` is `RecordOutcome::new_best` on the board that cleared, `sticker` is a Power word in `new_words` |
| `celebrated()`, `again()` | `rest()`, `again()` | |
| `lidOn()` | `back()` | `Store::record` of the board left, found anything or not |
| `startTile()`, `litTile()` | `request_help()`, `lit_tile()` | the find made while it is lit is recorded as assisted |
| `helpAllowed()` | what `SessionOptions::help` was given | |
| `board()` | `board()`, a dead jewel (`dead_tiles()`) as `""`, the seed of `last_deal()` | |
| `blocks(path)` | `GraphemeBinder` over the phase | |
| `history()` | | the store's boards, newest first, and their finds; the phase's dead letter is a dead jewel on a board of that phase |

**Where a run begins.** After every call that can change the game, `RealSoloGame`
brings the store's game in progress (`Store::save_session`) to what
`GameSession::snapshot()` says: a board on the table with its finds and the lit
tile (`playing`), or the resting screen, which is also how a board that has just
cleared is kept. A game in the box is **not** saved: the store holds none and what
it held is cleared. There is nothing on the table to bring back; the size the box
offers is the settings' (the child's last choice, written as it is made, so a
change the parent makes in the dashboard is not overruled by an old box); and the
next deal's seed is a new one each run, or the one `--seed` gives. `shaking` and
`dropping` are the box, as the session saves them. A game the store cannot keep
whole is cleared rather than left as the game before it. The session is saved
before a find is recorded, so the history is never ahead of the game; the board
left with the lid is recorded before the session is cleared, so it is never behind.

`SoloGameSetup::make()` is where the game comes back. If the store holds a game in
progress and `GameSession::restore()` accepts it (the same phase, a board the
phase's words still fit, a record a session makes) the game is that one, as it was
left, with the parent's help switch and sounds as they are now: a lit tile the
parent has since switched off is dropped. If it does not (`wrong_phase`,
`board_changed`, `bad_snapshot`, or a snapshot the store's values cannot make) a new
game begins in the box and the old record is cleared by it, and the child is told
nothing. The one thing a restore does not bring back is the seed of the scene's
shake and drop, which a board already on the table does not need.

The screens open on it. `SoloModel` over a game that is not in the box sets up what
a shake would have (the touch geometry, the lit tile) in its constructor, so it is
`playing` or `resting` from the start. `SoloScreen` gives the scene `show(board)`,
the board already down with no motion, when it opens on `playing`, and the closed
box otherwise; `play(board, seed)` is still only for a shake. So the scene's
interface, which `app.scene_contract` holds, did not change. The run itself still
begins on `home`: Solo Play, pressed there, opens on the board that was left.

The unit of the stamp is the **day**: `wjs::Day`, days since 1970-01-01 on the
device's calendar, which is what the store keeps of when and the most it will
ever keep. The program gives the game a function for it (Qt's `QDate`, in
`main.cpp`).

**Which game is played.** The repository has no phase: the files have to be
written and reviewed by a person qualified to do it, and none may be published
before that. So a run plays the real session only when it is told where a phase
is, and otherwise the fake:

```text
WordJewelsSolver --phase-directory <dir> [--phase <name>] [--dictionary <file>]
                 [--store <file>] [--seed <number>]
```

`<dir>` holds `phase-<x>.graphemes.txt` and `phase-<x>.words.txt` (the
[format](#phase-data)). `--phase` defaults to the phase in
the store's settings, `--dictionary` to `dictionaries/en.txt` beside the program,
`--store` to the per-user file (`wjs::default_store_path`), and `--seed` to a random
one; a fixed seed makes a run repeat. Files without a complete review header are
test data, and the program says so on its error output. A directory that cannot be
loaded, or a phase that cannot make a board, stops the program with a message and
exit code 2: it does not fall back to the fake, which would hide the mistake. Run
against test data, give `--store` a file of its own, or the finds go to the
store of the person's real profile.

What the game does with the parent's settings: the size to offer first, the help
switch and the sounds in focus are read when the game is made, once per run, so a
change made in the parent area is seen by the next run. The size the child last
chose on the box is written back to `Settings::board_size`, and a game in progress
is saved and brought back (above), so a run closed in the middle of a board shows
the same board with the same finds. What it does not do: read a change to the
settings made while the game is being played. The parent area's screens are not routed from the window yet; when they are, the one `Store`
is `SoloGameSetup::store()`, to hand to `ParentController::useStore()`.

The parent area (the gate, the dashboard, the settings and the about page) is a QML
module of its own, `WordJewelsSolver.Parent`, in `src/app/parent` (C++) and
`src/app/qml/parent` (screens), linked by one line of `src/app/CMakeLists.txt`.
`ParentController` is the shape Qt Quick wants over the library's rules, plus the
clock; `ParentArea.qml` is one screen with a controller and a `closed()` signal. The
program makes one `Store` and shares it with the game. Its tests are in
`wjs_app_tests`, and a stub shell (`tests/app/parent_ui`) draws the screens without a
display. See [The parent area as built](design/parent-area.md).

## Build targets

| Target | Kind | Notes |
| --- | --- | --- |
| `wjs_lib` (`wjs::lib`) | static library | The game logic |
| `wjs_platform` (`wjs::platform`) | static library | The platform's implementations; links `wjs_scan` |
| `wjs_scan` (`wjs::scan`) | static library | The scanner: finds a board in a frame and reads it. No third-party dependency |
| `wjs_cli` | executable `wjs-cli` | `WJS_BUILD_CLI` |
| `wjs_app` | executable `WordJewelsSolver` | `WJS_BUILD_APP` |
| `wjs_unit_tests` | executable | Catch2 |
| `wjs_perf_tests` | executable | Catch2; work budgets and opt-in wall-time checks, linked to `wjs::lib` and the scanner with its synthetic corpus |
| `wjs_scan_tests` | executable | Catch2; the scanner, the camera and `board_reader` interfaces, and the corpus that holds it to its target |
| `wjs_scan_synth` | static library | The synthetic corpus (frames of a board of dice drawn from a seed) and the counting of the target. Test tool; nothing in `src` links it |
| `wjs_scan_render` | executable | Writes the corpus to a directory as PNG files, to look at |
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

- **A camera.** The `camera` interface exists, and the three desktops answer
  `unavailable` (no backend); the iOS camera (`AVCaptureSession`), a desktop
  webcam and Android's are not written. Nothing has been run on a camera.
- **Reading letters from a real picture.** The reader exists and is measured on
  synthetic frames only ([Scanner design](design/scanner.md#what-was-measured)).
  The iOS reader (Vision), recorded frames of real dice, the dice's own typeface and
  the tuning that follows are not done, and need an iPad.
- **Phonics phase data.** The reader, the checks, the seeding, the binder, the
  shake detector, the game rules and the game session exist and are tested, and
  no front end uses them yet. No phase
  file exists, and none is written here: the content has to be written and
  reviewed by someone qualified to do it before a child uses it. The audio
  clips and the sound-it-out sequence are not built. The parent dashboard is built
  and shows nothing of the sounds until a phase exists.
- **Using the child-facing word lists.** `data/lexicon/` holds category and
  early-reader tier lists (animals, colours, shapes, and so on); nothing reads
  them yet. The [roadmap](roadmap.md) (M2) covers the lexicon and hints.
- **Packaging and installers**, and **phone and browser targets**.

The [roadmap](roadmap.md) has the plan and its order.
