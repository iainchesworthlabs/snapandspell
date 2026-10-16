# Performance design

Budgets, what to instrument with Tracy, and a performance-test tier. Written
from [product requirements](../product-requirements.md) section 8 and modelled
on Cubeulator's [performance](https://github.com/iainchesworthlabs/cubeulator/blob/main/docs/performance.md)
and [profiling](https://github.com/iainchesworthlabs/cubeulator/blob/main/docs/profiling.md)
documents and its
[`tests/perf`](https://github.com/iainchesworthlabs/cubeulator/tree/main/tests/perf).
Proposals unless marked **Fact**.

**Status.** The macro-free wrapper, the opt-in profiling build and the
performance tier are implemented, and what was checked where is marked below:
[Profiling](../profiling.md) says how to use them and
[Testing](../testing.md#performance-tests) how to read the tier. The sections
that were written as a plan keep their wording, with **Built** notes where the
build differed from the plan or settled something. The code that makes a board
(the roll of the dice, the seeding) and the grapheme binder have their zones
and work budgets too: see [The game rows](#the-game-rows).

**Board modes.** The standard board (dice, 3x3 to 6x6) is the MVP's **default**
and the seeded phonics board is a parent's switch (decided 2026-10-16). A
budget is for one mode, and every row below and every metric in the tier says
which.

**Every number below that is about a tablet or a phone is an estimate.** Nothing
has been measured on one, and the MVP's iPad is not named in this repository.

## What was measured

**Fact, measured for this document** on one Windows 11 desktop, a Release build
(MSVC 19.51, `/O2`), by a throw-away program that links the library's own
sources and is **not committed**. The performance tier below replaces it.

**Built, and re-measured by the tier on the same machine and compiler
(2026-10-16):** every work figure below reproduces exactly. The corpus draws
its letters in integer arithmetic where the throw-away program scaled a double,
and that changed no board of the 800. The maxima over 200 boards are 514 / 1,548
/ 2,859 / 5,174 visits, the four rich boards make 2,832 / 5,496 / 9,443 / 12,350
visits and find 1,126 / 1,464 / 1,788 / 327 words, and the two empty boards make
100 and 256. Loading ENABLE took 65 to 68 ms in the tier's runs (the
throw-away program saw 55 to 65 ms). The tier also runs, and passes, under GCC
15.2 on Ubuntu 26.04 with the same counts (a container of the CI image, and the
`Performance` CI leg).

| | Result |
| --- | --- |
| Loading ENABLE (172,823 words) | 55 to 65 ms; 387,889 trie nodes; +17.7 MB of working set |
| Whole `wjs-cli` run (roadmap) | About 76 ms, load dominating |
| Solving one board, mean / 95th percentile / worst | 3x3: 0.03 / 0.06 / 0.07 ms. 4x4: 0.12 / 0.19 / 0.26 ms. 5x5: 0.26 / 0.42 / 0.51 ms. 6x6: 0.48 / 0.69 / 0.85 ms |
| Visits of the search, mean / worst | 3x3: 181 / 514. 4x4: 707 / 1,548. 5x5: 1,477 / 2,859. 6x6: 2,692 / 5,174 |
| Boards built to hold many words (not dice sets) | 4x4 1,126 words, 2,832 visits, 0.9 ms; 5x5 1,464 words, 5,496 visits, 1.1 ms; 6x6 1,788 words, 9,443 visits, 1.7 ms; a 6x6 mix 12,350 visits, 1.5 ms |
| Boards with no words (all one letter) | 100 and 256 visits, 0.01 and 0.02 ms |

How the boards were drawn: 200 a size, `std::mt19937` seeded `7919 * i + size`,
each letter chosen by `rng() % 1,000,000` against ENABLE's letter counts. It
is a test corpus, not a dice set. A 4x4 with no word costs a hundred visits.

**What this says.** Solving is not the cost to budget: the worst board tried
took 1.7 ms. **Loading the dictionary is** (about 60 ms and 18 MB). Whether it is
on the critical path depends on the **board mode** (decided 2026-10-16: the
standard board is the default, the phonics board a parent's switch):

| Board mode | Solved against | Dictionary load | Seeding |
| --- | --- | --- | --- |
| **Standard** (default): dice, 3x3 to 6x6 | The child's word list, which is the full dictionary or a list cut from it | **On the path to the first board** | None: a roll of dice |
| **Phonics** (parent's switch): seeded, grapheme-bound | The phase index, a few hundred words at most | Not on the path: the full list is needed only to tell "real, not today's" from "not a word" | Yes, below |
| **Scanner** (Mode A) | The same as whichever board mode it is in; the phase index only in the phonics mode | As the board mode | None: it reads a real board |

The measurements above were taken on boards of random letters against the full
dictionary, which is the **standard** board's work; the seeding rows below are
the phonics board's only.

## Budgets

"The lowest device" is whatever the author names (a question on the
[overview](index.md#questions-for-the-author)). Multiples for a tablet are
Cubeulator's own **estimate** of 3 to 5 times a desktop core, which it says is
to be confirmed on the first device.

| Path | Budget | Basis |
| --- | --- | --- |
| One UI frame, steady, board on screen | 16.7 ms at 60 Hz | Cubeulator's UI budget. A 120 Hz display halves it; the design aims at 60 Hz and does not require more |
| During the shake, drop and confetti | No frame over 33 ms; 95% of frames under 16.7 ms | Proposal; measured on the device, not in CI |
| The resting screen | No frame is rendered after the fade (zero in five seconds) | Proposal. It is "calm", and a screen that draws nothing draws no power |
| A touch to a lit tile | 2 frames | Proposal |
| A touch to its letter's sound | 100 ms | Proposal; audio latency is unmeasured and differs by platform |
| Launch to `home` drawn | 2 s | Cubeulator proposes 2 s on a desktop; the same figure is proposed here |
| Full dictionary load (standard board: before its first board) | 1 s, off the UI thread; `home` is usable meanwhile | Estimate 0.2 to 0.33 s from 55 to 65 ms |
| Phase index load (phonics board) | 10 ms | Hundreds of words |
| Seeding a board (**phonics** only) | 10 ms typical, 50 ms worst | Up to 20 verifications at 0.02 to 0.5 ms (measured, desktop); the guided fill is up to 7 ms on a 6x6 (measured, desktop). Re-measured by the tier's corpus: [The game rows](#the-game-rows) |
| Making a standard board (**standard**, the default) | 50 ms | A roll of dice and one solve; the solve is the cost (worst measured 0.85 ms on a 6x6 of random letters; the dice's worst solves are in [The game rows](#the-game-rows)) |
| Solving any board | 50 ms | Worst measured 1.7 ms; a multiple of 5 gives 9 ms |
| Start of the sound-it-out sequence after the finger lifts | 100 ms | Proposal |
| Capture to the check screen (the single capture) | 3 s | Proposal; **unmeasured**, Vision's speed on dice is not known |
| Camera frame, after the MVP | 33 ms, all of it | Cubeulator's `DirectScan` budget |
| Memory for the dictionary | 32 MB | Measured 17.7 MB; a compact trie is an option if a device needs it |

Which are measured by a test is the next section.

## Tracy

### What exists to copy

**Fact, from Cubeulator's profiling document.** Tracy is in an opt-in build (a
`profiling` preset and a vcpkg feature `profiling`, `tracy[on-demand]` plus the
tools built for the host), runs in on-demand mode (an idle instrumented build
does no work), has its crash handler off, and is never in a shipped package
because its client **listens on a TCP port** and answers discovery broadcasts.
It also cannot run in a browser tab, so the web build has a recorder of its own
behind the same macros.

### The constraint here

This repository's rule is no `#define` and no `#if` in first-party C++
([Platform isolation](../platform-isolation.md)), and Tracy's API is macros.
Cubeulator resolves it with **macro-only variant headers** chosen by CMake;
those headers contain `#define`, so they would fail
`check_platform_isolation.ps1`. The roadmap lists a macro-free route as
unverified.

### A macro-free route

**Read (Tracy v0.13.1, `Tracy.hpp`).** The enabled `ZoneNamedN` expands to:

```cpp
static constexpr tracy::SourceLocationData loc{ name, TracyFunction, TracyFile, line, color };
tracy::ScopedZone zone( &loc, TRACY_CALLSTACK, active );
```

`SourceLocationData` is an aggregate of `name`, `function`, `file`, `line` and
`color`; `ScopedZone` takes a pointer to it. Likewise `FrameMark` calls
`tracy::Profiler::SendFrameMark( nullptr )` and the named form passes the name;
`TracyPlot` calls `tracy::Profiler::PlotData( name, val )`; `TracyMessage`
calls `tracy::Profiler::Message( txt, size, depth )`. In the disabled branch
every macro is empty. So **everything the macros do is ordinary C++ in
namespace `tracy`**, and the runtime source-location constructors (which copy
strings for each zone) are not needed.

**Checked here (a scratch program, MSVC 19.51, `/W4 /WX`, with stand-ins for
the two Tracy types):** a one-line zone with no macro compiles warning-free and
records the call site's function, file and line:

```cpp
// in a portable source file, no #define, no #if
const auto zone = wjs::profiling::zone<"solver.solve">();
```

where `zone` is a function template whose name is a class-type template
argument, a function-local `static` is initialised once from a
`std::source_location` default argument, and the returned type is not copyable
or movable (it is built in place by guaranteed elision, as Tracy's is).
**Not checked:** GCC and Apple clang; and against the real Tracy headers,
which were read from source and not compiled here.

The design, following Cubeulator's seam:

1. `src/lib/variants/profiling-enabled/wjs/profiling.hpp` includes Tracy and
   aliases the stand-ins to `tracy::SourceLocationData` and `tracy::ScopedZone`;
   `profiling-disabled/` defines an empty zone class. CMake puts one directory
   on the include path. **No first-party file has a `#define` or an `#if`.**
2. `TRACY_ENABLE` and `TRACY_ON_DEMAND` are compile definitions of the build,
   not source: the vcpkg port gives them to the target that links it.
3. The interface is a handful of functions: `zone<"...">()`,
   `frame_mark()`, `frame_mark("Name")`, `plot("name", n)`, `message("...")`,
   `thread_name("...")`.
4. A third directory, `profiling-recorder/`, is Cubeulator's recorder for the
   browser, behind the same functions.

Caveats. Tracy's own header defines macros (`ZoneScoped` and others) in every
file that includes the enabled variant, which is harmless unless a name
collides. The static is **per name**, so a name used at two sites shows the
first site's line; a test can scan for duplicate names. When no profiler is
connected a zone costs the guard of a function-local static and Tracy's
connection check.

**Built, and checked.** Compiled warning-free (`/W4 /WX`, `-Wall -Wextra
-Wpedantic -Werror ...`) against the **real Tracy v0.13.1 headers** (vcpkg
`tracy[on-demand]`, crash handler off) with MSVC 19.51, GCC 15.2 and Apple
clang 21 (Xcode), each running all 43 tests of its profiling preset: the
dispatch-only `profiling-*` legs of `ci.yml`, and the first two also locally.
The disabled variant is what every Debug leg compiles. Three things the plan
did not have:

- **A collision, found.** Tracy's header defines a macro `ZoneName`, which is
  what the first version of the wrapper called its name type, and it did not
  compile. It is `StaticName` now, and [Profiling](../profiling.md#adding-instrumentation)
  lists the names to keep clear of.
- **Names are checked by the compiler.** A zone's name is a template argument;
  the other names (`frame_mark("UI")`, `plot("x", n)`, `thread_name("main")`)
  convert from a string literal only, so a temporary string, whose pointer Tracy
  would keep, does not compile.
- **The variants are named `profiling-enabled/` and `profiling-disabled/`**,
  chosen by `WJS_ENABLE_TRACY` in `cmake/Tracy.cmake`. The `profiling-recorder/`
  of point 4 is not built: nothing targets the browser yet.

## What to instrument

Names are `<layer>.<thing>`, lower case, dotted, string literals, as in
Cubeulator. They are what the CSV and the GUI show and what the tier's
baseline reads, so a rename is deliberate. A zone goes where there is work, not
on an inner loop.

**Built:** `solver.solve` (the zone's value is the words found; the visits are
the plot `solver.visits`, because a zone carries one number) and
`data.load_dictionary` (the words), in `wjs::solve` and `Dictionary::load`; and,
with the code that makes a board, `board.seed` (the verifications it spent),
`board.verify` (the words of the phase found on one verified board),
`board.roll` (the dice rolled) and `phonics.bind` (the comparisons it made), in
`wjs::seed_board`, `wjs::roll` and `GraphemeBinder::bind`; and, with the game
session, `board.deal` (the seedings a deal spent), `input.submit` (the length of
a traced path) and `board.restore`, in `GameSession::shake`, `trace` and
`restore`. The rest of the table is added with the code it times.

`board.roll` is not in the plan's table: the dice did not exist when it was
written. It is the standard board's, where `board.seed` and `board.verify` are
the phonics board's, which is why the table's `board` row says so. A zone's
value is the one number a person reading the trace will want next to its time:
the comparisons for the bind (the figure the work budget holds, so a trace and
a budget say the same number), where the solver's value is its output and its
work is a plot. Seeding adds no plot of its own: its solves are `solver.solve`
zones inside `board.seed`, each with its `solver.visits`.

| Layer | Zones, with the value each carries |
| --- | --- |
| app | `app.startup`, `app.load_qml`, `app.scene_create` |
| data | `data.load_dictionary` (words), `data.load_phase` (words), `data.save_store` |
| board | `board.seed` (tries; **phonics**), `board.verify` (viable words; **phonics**), `board.deal` (seedings; **phonics**, the session), `board.restore` (**phonics**, the session), `board.roll` (dice; **standard**) |
| solver | `solver.solve` (words found; visits) |
| phonics (**phonics** board only) | `phonics.bind` (comparisons), `phonics.segment`, `phonics.sequence` |
| input | `input.submit` (path length; built, in `GameSession::trace`). Per-move work is a plot, `input.hit_test_us`, not a zone |
| audio | `audio.play_clip` (clip id); a plot `audio.start_latency_ms` |
| scene | `scene.shake`, `scene.drop`, `scene.settle`, `scene.celebrate` |
| scan | `capture.read`, `scan.ocr`, `scan.parse_grid` (after the MVP, with `vision.*` as Cubeulator names them) |

Frame sets: unnamed for the seeding loop's passes if useful, `UI` for the Qt
Quick frame (`frameSwapped`, as Cubeulator), `Capture` for the camera.
Threads: `main`, `audio`, `data-load`, `camera-capture`. Messages:
`solo.board_started`, `solo.word_found`, `solo.cleared`, `scan.captured`.
The trace's information window carries the build (version, commit, dirty).

## The performance tier

Modelled on Cubeulator's `tests/perf`: its own binary, `wjs_perf_tests`, linked
against `wjs::lib` alone (no Qt, no window), every threshold in one file,
`tests/perf/budgets.hpp`. **Two halves, and the split is the design.**

| | Holds | Exact? | Runs |
| --- | --- | --- | --- |
| **Work budgets** | What the code *does*: visits, words, tries, trie nodes | Yes, on every machine and compiler | In every plain `ctest` |
| **Wall-time checks** | What it *takes*, as a multiple of a calibration workload run in the same process | No, and loose on purpose | Only when asked: `WJS_PERF_TIMING=1`, a `*-perf` test preset, or a CI leg; run serially |

Cubeulator's reasons carry over: a count fails with a number and a seed rather
than "slower"; a time is the one check that can fail with nothing changed, so it
is a ratio of the fastest of N runs to the calibration's fastest, alternating,
and a budget is several times the ratio.

### Mechanics to copy

- **A corpus that is the same everywhere.** `std::mt19937` taken modulo, never
  `std::uniform_int_distribution` (its algorithm is each standard library's
  own), so a seed is the same board on MSVC, libstdc++ and libc++.
- **A work counter in the library**: a thread-local tally the search adds to
  once as it ends, nothing per visit, and nothing in the application reads it.
  (Cubeulator's `search_work.hpp`.) **Built:** `wjs/solver_work.hpp`; the search
  counts into a member and the solve adds the total.
- **The build decides whether a clock is meaningful**, as `constexpr bool`s from
  compile definitions CMake passes (`WJS_PERF_OPTIMISED`,
  `WJS_PERF_INSTRUMENTED`), read with no preprocessor. Debug and sanitizer
  builds *skip* the tier, never fail it.
- **A digest** of every solution over the corpus, so "same results" is one
  number.
- **Budgets a quarter above the measurement**, and moved only when the work
  genuinely changed, by a person, never to make a red run green.

### Work budgets

**Built:** every row is in `tests/perf/budgets.hpp` with the figure it is a
quarter above (rounded up to three significant figures for the game rows).
Every metric names the **board mode** it is for. The standard board is the
default, so its work comes first: the solve, the dictionary and the dice are the
**standard board's**; the `seed.*` and `bind.*` rows are the **phonics board's**,
a parent's switch, and are measured on a test phase (below), because the
repository has none.

| Metric | Mode | Measured | Budget (a quarter above) |
| --- | --- | --- | --- |
| `dictionary.words` (ENABLE) | standard | 172,823 | Exactly this; changes with the data |
| `dictionary.nodes` | standard | 387,889 | Exactly this |
| `solve.visits.max` 3x3 / 4x4 / 5x5 / 6x6 over 200 boards | standard | 514 / 1,548 / 2,859 / 5,174 | 640 / 1,940 / 3,580 / 6,470 |
| `solve.visits` on the four built-to-be-rich boards | standard | 2,832 / 5,496 / 9,443 / 12,350 | 3,550 / 6,900 / 11,800 / 15,500 |
| `solve.words` on each of those four | standard | 1,126 / 1,464 / 1,788 / 327 | Exactly (it pins the corpus) |
| `roll.visits.max` 3x3 / 4x4 / 5x5 / 6x6: a roll of the dice, then a solve, over 200 seeds | standard | 471 / 1,636 / 3,748 / 6,457 | 589 / 2,050 / 4,690 / 8,080 |
| digest of every solution over those boards | standard | 9710656603150208005 | Exactly |
| `seed.phase.words`: the test phase | phonics | 249 | Exactly (it pins the phase) |
| `seed.tries.total` over 100 seeds, 3x3 / 4x4 / 5x5 / 6x6 | phonics | 1,739 / 664 / 194 / 187 | 2,000 (the cap: see below) / 830 / 243 / 234 |
| `seed.solves.total`, the same | phonics | 5,503 / 21,653 / 40,663 / 68,049 | 6,880 / 27,100 / 50,900 / 85,100 |
| `seed.visits.total`, the same | phonics | 487,209 / 1,036,442 / 2,525,621 / 5,578,096 | 610,000 / 1,300,000 / 3,160,000 / 6,980,000 |
| `seed.visits.max`, one seeding | phonics | 10,460 / 25,292 / 96,479 / 133,802 | 13,100 / 31,700 / 121,000 / 168,000 |
| `seed.tries.max` | phonics | 20 / 20 / 7 / 7 | 20, the algorithm's own cap (`SeedOptions::max_tries`) |
| `seed.fallbacks` of 100 | phonics | 77 / 5 / 0 / 0 | 97 / 7 / 0 / 0 (**not** the plan's 0: see below) |
| `seed.out_of_range`: a board called a landing that is outside 3 to 5 words | phonics | 0 | Exactly 0, always |
| digest of every board seeded | phonics | 15489434241397487766 | Exactly |
| `bind.comparisons`, a path of 9: none / all / mixed | phonics | 279 / 50 / 187 | 349 / 63 / 234 |

The plan's "viable words within 3 to 5, always" row (for `board.verify`) is
`seed.out_of_range`, held as a count of boards outside the range that are not
marked fallbacks.

### The game rows

The code that makes a board and shows its blocks exists now, and this is what
was added for it and what it measured. **Built, and measured 2026-10-16** (a
Release build, MSVC 19.51, the Windows 11 desktop above; the counts are exact on
every machine, the milliseconds are this machine's, taken by a throw-away
program that is not committed). The cases are in
`tests/perf/game_work_test.cpp` and carry the tags `[standard]` or `[phonics]`.

**Standard board (the default).** `wjs::roll` of the project's dice
(`data/dice/<n>x<n>.txt`, the shipped files) with the seeds of the random
boards (`7919 * i + n`, 200 a size), then `wjs::solve` against the full word
list: what making a standard board costs. The zone is `board.roll` (the dice
rolled).

| Size | Most visits of a solve | Budget | Roll alone | Roll and solve, mean / worst |
| --- | --- | --- | --- | --- |
| 3x3 | 471 | 589 | 2.5 us | 0.03 / 0.08 ms |
| 4x4 | 1,636 | 2,050 | 3.4 us | 0.11 / 0.26 ms |
| 5x5 | 3,748 | 4,690 | 4.6 us | 0.29 / 0.59 ms |
| 6x6 | 6,457 | 8,080 | 5.8 us | 0.53 / 1.02 ms |

The roll is not the cost, as the plan said: a shuffle and a parse, a few
microseconds, and the solve is the rest. The dice's worst solves are below the
random boards' (514 / 1,548 / 2,859 / 5,174) on a 3x3 and up to a third above
them from 4x4 up: a die has a vowel on it, so these boards hold more words than
a draw by letter counts. The 50 ms budget for making a board is met by a factor
of fifty on this desktop. The digest pins the dice files and the roll together;
it moves when the files do (they are made from the dictionary:
`data/README.md`).

**Phonics board (a parent's switch): seeding.** The corpus is the proxy index
of [Testing](../testing.md#seeding-what-was-measured): the 249 words of three
or more letters of ENABLE that are spelled only with S, A, T, P, I and N, each
letter a grapheme, built by walking the trie. It is test data, as every phase
here is; it stands for "a few hundred words", which is the size the design
guessed. The options are those of `phonics_board_options` for the size (the
guided fill, targets of three letters at most, every letter but `B` as filler),
the range 3 to 5 and 20 tries; 100 seeds a size. The zones are `board.seed` (the
verifications it spent) and `board.verify` (the words of the phase found on one
board), with the solver's own zones inside.

| Size | Verifications (max one) | Solves | Visits, total / worst seeding | Fallbacks of 100 | Time a seeding, mean / worst |
| --- | --- | --- | --- | --- | --- |
| 3x3 | 1,739 (20) | 5,503 | 487,209 / 10,460 | 77 | 0.7 / 1.2 ms |
| 4x4 | 664 (20) | 21,653 | 1,036,442 / 25,292 | 5 | 1.3 / 2.3 ms |
| 5x5 | 194 (7) | 40,663 | 2,525,621 / 96,479 | 0 | 3.0 / 7.4 ms |
| 6x6 | 187 (7) | 68,049 | 5,578,096 / 133,802 | 0 | 6.6 / 12.1 ms |

What it says:

- **The plan's "fallbacks is 0" does not hold on the small boards, and the
  budgets are what was measured.** 77 of 100 seeds of a 3x3 and 5 of 100 of a
  4x4 end as the nearest board, not one in range. This agrees with
  [Testing](../testing.md#fuller-boards-a-guided-fill-and-short-targets)
  (23% and 92% landed with targets of three letters): a phase of this size
  leaves a 3x3 only a few words to choose from. The 5x5 and 6x6 land every
  time. Nothing here says the 3x3 is wrong for the phonics board, only that the
  seeding mostly gives up on it with this phase; whether that board is wanted
  is the author's.
- **The budget on a 3x3's verifications is the cap.** A quarter above 1,739
  is 2,174, which is more than 100 seeds of 20 tries can make, so it is set to
  that 2,000 and cannot fail. What holds the 3x3 is its solves and visits,
  which the guided fill's trials leave unbounded.
- **Verifications are not the cost; the guided fill is.** A 6x6 spends under
  two verifications a seeding and 680 solves, because the fill tries each of up
  to 25 letters on each tile left. On this desktop a seeding is within the 10
  ms typical and 50 ms worst of the budgets table; at the estimated 3 to 5
  times a desktop core a 6x6 would be 20 to 33 ms typical, which is over the 10
  ms. That is the plan's own worry about the lowest device, now with a figure.
  These times are two to three times the guided-fill table in
  [Testing](../testing.md#fuller-boards-a-guided-fill-and-short-targets) for
  the same words, which was taken before `phonics_board_options` fixed the
  options (its filler is 25 letters).
- **No accepted board was outside the range**, on any size.

**Phonics board: binding.** `GraphemeBinder::bind` of a path of nine tiles
against a test set of 39 graphemes, of which the binder holds the 31 that have
more than one letter and are not split. The zone is `phonics.bind`, carrying the
comparisons; the work counter is `wjs/binder_work.hpp`, written once as a bind
ends, as the solver's is.

| Path | Blocks | Comparisons | Budget | Time a bind |
| --- | --- | --- | --- | --- |
| none: `S A T P I N S A T` | 9 | 279 (9 x 31) | 349 | 1.5 us |
| all: `C H A I R S H E E` | 4 | 50 | 63 | 0.9 us |
| mixed: `S H I P R A I N S` | 7 | 187 | 234 | 1.0 us |

A tile that starts no grapheme is held against every grapheme the binder
holds, so the work is tiles times graphemes at worst, and that is the binder's
only growth: a phase of 100 multi-letter graphemes would cost about 900
comparisons a path. A case recounts the comparisons from the blocks that came
back and the order the binder documents, with nothing the library counts.

**Phonics board: the game session.** `wjs::GameSession` deals a board with
`seed_board` and seeds again, with the next seed of its stream, until a board
holds three to five words, up to five seedings (`SessionOptions::max_seedings`).
The corpus is the same test phase and the sizes the first release plays: 100
sessions each of 3x3 and 4x4, seeded `7919 * i + n`, each dealt, played to the
resting screen and restored once (`tests/perf/game_work_test.cpp`, tag
`[session]`). The zones are `board.deal` (the seedings it spent),
`input.submit` (the length of the path traced) and `board.restore`.

| Size | Seedings (max one) | Verifications | Solves | Visits, total / worst deal | Boards out of range of 100 | Time a deal, mean / worst |
| --- | --- | --- | --- | --- | --- | --- |
| 3x3 | 322 (5) | 5,792 | 17,534 | 1,544,699 / 26,251 | 35 | 3 / 7 ms |
| 4x4 | 106 (3) | 717 | 22,809 | 1,101,833 / 25,292 | 0 | 1.4 / 2.4 ms |

What it says:

- **Seeding again is what makes three to five the rule rather than the hope.**
  A single seeding leaves 77 of 100 3x3 boards out of range (the fallbacks of
  the seeding rows above); five seedings leave 35, and the 4x4's 5 are all
  landed by the third. By seedings allowed, the 3x3's misses are 77 / 58 / 49 /
  38 / 35 / 25 for one to six (measured with a throw-away loop, not committed).
  What a miss is: the nearest board, which still holds a word, and the session
  reports it (`DealReport::in_range`); the screens do not show a count.
- **The price is the 3x3 deal's solves and nothing else.** A 3x3 deal is 175
  solves and a 4x4 deal 228, almost all of them the guided fill's trials of a
  letter on a tile. The times are within the budget of 10 ms typical and 50 ms
  worst on this desktop (measured while other sessions were building, so these
  are indicative); at the estimated 3 to 5 times a desktop core the 3x3's worst is
  about 35 ms, still within it. A deal runs before the shake begins, so it can be
  done on a worker thread while the box is on screen; the library has no thread
  in it.
- **Playing makes no solve.** Tracing, asking for the help, `record()`,
  `snapshot()` and `rest()` read what the deal kept: 0 solves and 0 visits over all
  200 boards, held exactly. A **restore** makes one solve (100 of 100), the cost
  of the board again: 5,122 visits for 100 3x3 boards and 6,605 for 100 4x4.
- **A board with no word was never dealt**, on any seed, and the words of the
  phase on the 200 boards are exactly 522 and 476, which pins the corpus, the
  phase and the seeding together.
- The proxy phase is a size, not a proposed index. A real Phase A of 68 to 122
  words lands a 3x3 more often (56% to 75% of single seedings against 23%), so the
  3x3's misses here are an upper bound, and the 4x4's 0 is not a promise for a
  smaller index.

**What was not added.** No wall-time check was added for the roll, the seeding or
the binder: the counts above see the order-of-magnitude slowdown a ratio would,
and a seeding is dominated by the solver, which has them. The game session has
one, opt-in like the others (below), because a deal is the one path of the game
that a child waits for. The standard board's word list is still the full
dictionary until the author chooses a list; the dice rows are against it.

### Time ratios

**Built, and measured** (the tier's own calibration, in a Release build, three
runs, MSVC 19.51 on the Windows 11 desktop above; the heavy calibration drifted
from 84 to 125 ms between runs, which is what a ratio is for). A solve is one
solve, against the light calibration (13 to 17 ms); the load is against the
heavy one (a working set of tens of megabytes):

| Task | Alone | Ratio measured | Budget |
| --- | --- | --- | --- |
| Loading ENABLE | 66 to 68 ms | 0.65 to 0.78 | 5.0 |
| Solving a 4x4 built to be rich | 0.57 to 0.59 ms | 0.042 to 0.043 | 0.3 |
| Solving a 6x6 built to be rich | 1.6 ms | 0.115 to 0.120 | 0.8 |
| Dealing a session board, 3x3 and 4x4 (**phonics**; one deal, the first five corpus seeds) | 1.2 and 1.3 ms | 0.087 and 0.086 | 0.6 and 0.6 |

Fastest of 5 runs (the load) and of 15 runs of 50 solves, alternating with the
calibration. The proposal's starting values (10, 1.0, 1.0) were against a
stand-in calibration that this replaces; each budget is now six to eight times
what it measures. A load that costs a copy per node, or a solve that allocates
per visit, is a factor of several, which these absorb.

### What the tier does not hold

It is not the frame time of the 3D scene, the start of a sound, the speed of
the OCR or a launch. Those are measured on the device by a person with Tracy
connected (or Qt's own `QSG_RENDER_TIMING`, as Cubeulator's page does), and
written down as Cubeulator's `performance.md` does: the build, the machine, what
was and was not measured. A system test that counts frames on the resting
screen is the one automatable exception, and it needs the window and Qt.

## Unknowns

- Every device figure above.
- The size of the phase index, and so the seeding's tries. **Measured** on a
  proxy of 249 words ([The game rows](#the-game-rows)); the real phases'
  sizes are not known, and a smaller or larger index moves every `seed.*` row.
- The standard board's word list: the full dictionary or a list cut from it
  decides its load time and memory, and is not yet chosen
  ([MVP library](mvp-library.md)).
- ~~Whether the macro-free zone behaves on GCC and Apple clang and against the
  real Tracy headers.~~ **Settled** on MSVC, GCC and Apple clang, against the
  real headers (see "A macro-free route").
- **A profiler needs about a second to find a program that has just started**,
  and in on-demand mode what happens before it connects is not recorded, so a
  program that is over in a tenth of a second (`wjs-cli`) cannot be captured as
  it is. `wjs-cli --repeat <n>` exists for this; the zones of the application
  will not have the problem. See [Profiling](../profiling.md#capturing-a-trace).
- Whether Qt Quick 3D's first frames on the iPad need a baked light probe, as
  they did on Cubeulator's software renderer.
