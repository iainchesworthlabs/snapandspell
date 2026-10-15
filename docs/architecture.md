# Architecture

WordJewelsSolver is a small library of game logic with thin front ends on top
of it, and a separate layer for the few things that differ between operating
systems. This page says what is there. What is not built yet is listed at the
end and in the [roadmap](roadmap.md).

## Source layout

```text
src/
  lib/                 wjs:: the game logic. GUI-free and OS-free
    include/wjs/         board.hpp  dictionary.hpp  scoring.hpp  solver.hpp
                         solver_work.hpp  profiling_types.hpp
                         version.hpp.in (configured into a generated header)
    src/                 board.cpp  dictionary.cpp  scoring.cpp  solver.cpp
    variants/            profiling-enabled/wjs/profiling.hpp   (Tracy behind it)
                         profiling-disabled/wjs/profiling.hpp  (empty)
  cli/                 main.cpp: wjs-cli
  app/                 the Qt Quick window (built only with WJS_BUILD_APP)
    qml/Main.qml         the window
    solver_model.*       the model the window talks to
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
   board, dictionary,                  one implementation of each
   scoring, solver                     interfaces/ header, chosen by CMake
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

`src/app` is a small Qt Quick application and is optional (`WJS_BUILD_APP`,
off by default; it needs a prebuilt Qt 6.12). `SolverModel` is a `QObject` that
owns a `wjs::Dictionary`, loads it from `--dictionary <file>` or from
`dictionaries/en.txt` beside the executable, and turns a board string into a
`QVariantList` of words, scores and paths for QML. `Main.qml` is a grid of
text fields, a Solve button and a list. All the solving is the library's; the
model only reshapes its answer. The library knows nothing of Qt, and the model
has its own Catch2 tests.

What makes the window a program of the platform's desktop (a GUI-subsystem
executable on Windows, an application bundle on macOS) and what puts Qt's
runtime beside it are two hooks in each platform's `platform.cmake`, not tests
of the operating system in CMake.

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
- **Using the child-facing word lists.** `data/lexicon/` holds category and
  early-reader tier lists (animals, colours, shapes, and so on); nothing reads
  them yet. The [roadmap](roadmap.md) (M2) covers the lexicon and hints.
- **Packaging and installers**, and **phone and browser targets**.

The [roadmap](roadmap.md) has the plan and its order.
