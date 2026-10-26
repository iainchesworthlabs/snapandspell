# Performance design

Budgets, what to instrument with Tracy, and a performance-test tier. Written
from [product requirements](../product-requirements.md) section 8 and modelled
on Cubeulator's [performance](https://github.com/iainchesworthlabs/cubeulator/blob/main/docs/performance.md)
and [profiling](https://github.com/iainchesworthlabs/cubeulator/blob/main/docs/profiling.md)
documents and its
[`libs/core/bench`](https://github.com/iainchesworthlabs/cubeulator/tree/main/tests/perf).
Proposals unless marked **Fact**.

**Status.** The macro-free wrapper, the profiling build (on by default in
Debug builds) and the performance tier are implemented, and what was checked
where is marked below: [Profiling](../profiling.md) says how to use them and
[Testing](../testing.md#performance-tests) how to read the tier. The sections
that were written as a plan keep their wording, with **Built** notes where the
build differed from the plan or settled something. The code that makes a board
(the roll of the dice, the seeding) and the grapheme binder have their zones
and work budgets too: see [The game rows](#the-game-rows).

**What is not built or not held** (checked against the merged code, 2026-10-22):

- **No required CI leg holds the work budgets.** They skip in a Debug build, and
  the three required build legs are Debug. The one automatic Release leg
  (`Performance (Linux, GCC)`) is not a required check, no workflow sets
  `WJS_REQUIRE_PERF_BUDGETS`, and the wall-time checks and the window's frame
  check run in no automatic leg ([The performance tier](#the-performance-tier)).
- **The dictionary load is synchronous.** It runs on the main thread before the
  QML engine exists; "off the UI thread" is not built ([Budgets](#budgets)).
- **The frame check fails on one screen of the author's desktop, and the cause is
  open** ([The window's frames](#the-windows-frames)).
- **There is no resting screen and no confetti or celebration scene.** The
  resting screen was not built and a cleared board waits on a summary sheet
  ([D12](../roadmap.md#4-decisions-that-gate-the-plan)); the zeros those rows
  asked for are not measured by anything.
- Nothing has been measured on a tablet or a phone.

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
100 and 256. Loading ENABLE took 66 to 68 ms in the tier's runs (the
throw-away program saw 55 to 65 ms); 66 to 68 ms is the figure the budgets, the
[time ratios](#time-ratios) and [Profiling](../profiling.md#capturing-a-trace)
(67 ms at the fastest under a profiler) use, and the one below. The tier also runs, and passes, under GCC
15.2 on Ubuntu 26.04 with the same counts (a container of the CI image, and the
`Performance` CI leg).

| | Result |
| --- | --- |
| Loading ENABLE (172,823 words) | 66 to 68 ms in the tier (55 to 65 ms in the throw-away program); 387,889 trie nodes; +17.7 MB of working set |
| Whole `wjs-cli` run (roadmap's figure, not re-measured) | About 76 ms, load dominating |
| Solving one board, mean / 95th percentile / worst | 3x3: 0.03 / 0.06 / 0.07 ms. 4x4: 0.12 / 0.19 / 0.26 ms. 5x5: 0.26 / 0.42 / 0.51 ms. 6x6: 0.48 / 0.69 / 0.85 ms |
| Visits of the search, mean / worst | 3x3: 181 / 514. 4x4: 707 / 1,548. 5x5: 1,477 / 2,859. 6x6: 2,692 / 5,174 |
| Boards built to hold many words (not dice sets) | 4x4 1,126 words, 2,832 visits, 0.9 ms; 5x5 1,464 words, 5,496 visits, 1.1 ms; 6x6 1,788 words, 9,443 visits, 1.7 ms; a 6x6 mix 12,350 visits, 1.5 ms |
| Boards with no words (all one letter) | 100 and 256 visits, 0.01 and 0.02 ms |

How the boards were drawn: 200 a size, `std::mt19937` seeded `7919 * i + size`,
each letter chosen by `rng() % 1,000,000` against ENABLE's letter counts. It
is a test corpus, not a dice set. A 4x4 with no word costs a hundred visits.

**What this says.** Solving is not the cost to budget: the worst board tried
took 1.7 ms. **Loading the dictionary is** (about 66 to 68 ms and 18 MB). Whether it is
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
| One UI frame, steady, board on screen | 16.7 ms at 60 Hz | Cubeulator's UI budget. A 120 Hz display halves it; the design aims at 60 Hz and does not require more. Built as the display's own period (the median within one period and 0.3 ms), because a fixed 17 ms is wrong for a 100 Hz screen ([The window's frames](#the-windows-frames)) |
| During the shake and the drop | No frame over 33 ms; 95% of frames within one display period and 2 ms | Proposal, and checked by the opt-in frame check on a desktop with a display; never in CI. The row used to name the confetti too, which was never built. On a tablet or phone: unmeasured |
| The resting screen | ~~No frame is rendered after the fade (zero in five seconds)~~ **Dropped with the screen** | The resting screen was not built; a cleared board waits on a summary sheet ([D12](../roadmap.md#4-decisions-that-gate-the-plan)). Nothing measures what that sheet draws |
| A touch to a lit tile | 2 frames | Proposal |
| A touch to its letter's sound | 100 ms | Proposal; audio latency is unmeasured and differs by platform |
| Launch to `home` drawn | 2 s | Cubeulator proposes 2 s on a desktop; the same figure is proposed here. Reported by every smoke run (`ui.first_frame_ms`, 576 to 852 ms in optimised builds on the desktop), **not held by a test** |
| Full dictionary load (standard board: before its first board) | 1 s. **Not built: off the UI thread, with `home` usable meanwhile.** The load is synchronous, in `start()` in `apps/app/src/main.cpp` before the QML engine exists, so nothing is drawn until it ends | Estimate 0.2 to 0.34 s on a tablet from 66 to 68 ms at 3 to 5 times a desktop core. **Reported only:** the time is held as a ratio to a calibration workload (5.0), opt-in and run in no CI leg; no test holds the 1 s |
| Phase index load (phonics board) | 10 ms | Hundreds of words. Measured at about 1 ms for the 249-word test phase as a directory (and 0.4 ms for 7 words); held as work (bytes, words, graphemes) and a ratio, [below](#what-the-screens-wait-on) |
| Seeding a board (**phonics** only) | 10 ms typical, 50 ms worst | Up to 20 verifications at 0.02 to 0.5 ms (measured, desktop); the guided fill is up to 7 ms on a 6x6 (measured, desktop). Re-measured by the tier's corpus: [The game rows](#the-game-rows) |
| Making a standard board (**standard**, the default) | 50 ms | A roll of dice and one solve; the solve is the cost (worst measured 0.85 ms on a 6x6 of random letters; the dice's worst solves are in [The game rows](#the-game-rows)). The session's whole deal, measured: 0.05 ms on a 3x3 to 0.67 ms on a 6x6 |
| Solving any board | 50 ms | Worst measured 1.7 ms; a multiple of 5 gives 9 ms |
| Start of the sound-it-out sequence after the finger lifts | 100 ms | Proposal |
| Capture to the check screen (the single capture) | 3 s | Proposal. **Measured, desktop only:** the portable reader reads a 4x4 frame in 45 to 50 ms and a 6x6 in 61 to 69 ms (MSVC, Release, [Scanner design](scanner.md#what-was-measured)). The read runs on a worker thread named `scan`, not on the GUI thread. A tablet and Vision's speed on dice are **unmeasured** |
| Camera frame, after the MVP | 33 ms, all of it | Cubeulator's `DirectScan` budget |
| Memory for the dictionary | 32 MB | Measured 17.7 MB; a compact trie is an option if a device needs it |

Which are measured by a test is the next section. **Built (2026-10-19):** the
two frame rows above are measured by the application's own frame clock and
`--smoke`, and held by an opt-in check that no CI leg runs ([The window's
frames](#the-windows-frames)); the rest is where this document said it was.

## Tracy

### What exists to copy

**Fact, from Cubeulator's profiling document.** Tracy is in an opt-in build (a
`profiling` preset and a vcpkg feature `profiling`, `tracy[on-demand]` plus the
tools built for the host), runs in on-demand mode (an idle instrumented build
does no work), has its crash handler off, and is never in a shipped package
because its client **listens on a TCP port** and answers discovery broadcasts.
It also cannot run in a browser tab, so the web build has a recorder of its own
behind the same macros.

**Built here, and what it exposes.** A Debug build and the `*-profiling` presets
are instrumented, and in them the Tracy client listens on **TCP port 8086 on
every interface** and answers discovery broadcasts for as long as the program
runs, whatever the program does ([Profiling](../profiling.md#the-profiler-listens-on-the-network);
`SECURITY.md` says the same to a reporter). The release presets, and the
packages made from them, exclude the client: the preset names are held by
`profiling.shipped_presets`, `cpack` refuses a Tracy tree unless a development
package is asked for by name, and the package check rejects a Release package
that carries `TracyClient*`. CI's Windows Debug package is a development package
on purpose. The capture tools are built from an overlay of the vcpkg port without
ISA extensions, so a tool built on one hosted runner runs on another
([Profiling](../profiling.md#building-a-profiling-binary)).

### The constraint here

This repository's rule is no `#define` and no `#if` in first-party C++
([Platform isolation](../platform-isolation.md)), and Tracy's API is macros.
Cubeulator resolves it with **macro-only variant headers** chosen by CMake;
those headers contain `#define`, so they would fail
`check_platform_isolation.ps1`. The roadmap lists a macro-free route as
unverified.

### A macro-free route

**Read (Tracy v0.14.1, `Tracy.hpp`).** The enabled `ZoneNamedN` expands to:

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

1. `libs/core/variants/profiling-enabled/wjs/profiling.hpp` includes Tracy and
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
-Wpedantic -Werror ...`) against the **real Tracy v0.14.1 headers** (vcpkg
`tracy[on-demand]`, crash handler off) with MSVC 19.51, GCC 15.2 and Apple
clang 21 (Xcode), each running the tests of its profiling preset (43 of them
then): the dispatch-only `profiling-*` legs of `ci.yml`, and the first two also locally.
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
session, `board.deal` (the seedings a deal spent on the phonics board, the rolls on
the standard), `input.submit` (the length of a traced path) and `board.restore`, in
`GameSession::shake`, `trace` and `restore`; and, with the scanner, `scan.find_board` and `scan.read_tiles`, in
`wjs::scan::find_board` and `read_board`. Since then the rest of the table below
that is marked built was added with the code it times, and the whole set is
pinned: `profiling.zone_names` fails when a required zone, plot, QML scope, frame
set or thread name is gone, and [Profiling](../profiling.md#what-is-instrumented)
has the complete list with where each is. `scene.shake`, `scene.drop` and
`scene.settle` are chosen at run time by `BoardScene.stageZone()`, so the check
pins them as string literals in the QML.

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
| app | **Built:** `app.startup`, `app.fonts` (six font files read before the first screen), `app.load_qml` (all C++). `app.scene_create` (the scene's first layout, from QML) was built once and is **gone** (removed in the Jewel Box design overhaul): nothing times the scene's creation, and `scene.prepare` times a board's atlas |
| data | **Built:** `data.load_dictionary` (words), `data.load_phase` (words), `data.save_store` (bytes written), `data.load_store` (bytes read), `data.build_dashboard` (sounds), `data.load_catalogue` (a translation catalogue installed) |
| board | `board.seed` (tries; **phonics**), `board.verify` (viable words; **phonics**), `board.deal` (the session: seedings on the **phonics** board, rolls on the **standard**), `board.restore` (the session, either board), `board.roll` (dice; **standard**); also built, `board.deal_playable` (the standard deal's roll-until-it-holds-a-word loop) and `board.parse` |
| solver | `solver.solve` (words found; visits). The former `solver.model` went with the solver's app model, which was removed |
| phonics (**phonics** board only) | **Built:** `phonics.bind` (comparisons) and `phonics.sound_it_out` (the sequence of sounds for a word). `phonics.segment` and `phonics.sequence` were planned and are **not built** (the playback is `sound.sequence`) |
| input | `input.submit` (path length; built, in `GameSession::trace`) and `input.trace` (the C++ path tracer). Per-move work as a plot, `input.hit_test_us`, is **not built** |
| ui | **Built:** `ui.trace` (the trace layer's path work, from QML), `ui.screen` (a screen's transition, from QML); the plots `ui.frame_ms`, `ui.first_frame_ms` and the frame mark `UI` are the frame clock's, and `render.frame_ms`, `render.record_ms`, `ui.sync_ms` and `ui.animate_ms` are the two threads' shares |
| feedback, sound, speech | **Built:** `feedback.plan`, `feedback.play`, `sound.play`, `sound.sequence`, `speech.say`. They time the call that hands a cue to the device, not the moment it is heard. `audio.play_clip` (clip id) and the plot `audio.start_latency_ms` were planned and are **not built**: no sound latency is measured |
| scene | **Built:** `scene.step` (the dice world's step; the steps it took, C++), `scene.prepare` (the atlas; its cells), `scene.play`, `scene.show`, `scene.shake`, `scene.drop`, `scene.settle` (all from QML). `scene.celebrate` was built once and is **gone** with the celebration scene: there is none now. The 2D fallback's `scene2d.play`, `scene2d.step` and `scene2d.show` are in the file kept for D2 and not built |
| scan, assist, camera | **Built:** `scan.find_board`, `scan.read_tiles`, the stages inside them (`scan.bright_mask`, `scan.box_blur`, `scan.find_blobs`, `scan.decode_frame`, `scan.convert_working`, `scan.warp_board`, `scan.read_glyphs`), `scan.process_frame` (the whole read, on the `scan` thread), `camera.convert_frame`, `assist.confirm` and `assist.solve`. Not built: `capture.read` (the camera's capture has the frame mark `Capture` but no zone), and `vision.*` as Cubeulator names them, which the iOS reader will have |
| solo, session, parent | **Built:** `solo.path`, `solo.blocks`, `solo.history`, `solo.submit`, `solo.help`, `solo.words_with_start`, `solo.again`, `session.finish`, `parent.edit` |
| the library's other calls | **Built:** `path.validate`, `rules.classify`, `settings.normalize`, `shake.detect`, `motion.push`, `gate.refresh`, `dice.load_sets`, `store.parse`, `store.serialize`, `store.session_snapshot`, `store.session_record`, `cli.run` |

Frame sets: unnamed for the seeding loop's passes if useful (none), **`UI`** for
the Qt Quick frame (built, `frameSwapped`, as Cubeulator), and **`Capture`** for the
camera (built: `CameraSource::capture` marks one for each frame it takes).
Threads: `main`, `render` and `scan` are built (the frame clock names the render
thread; the scanner's read runs on a worker thread of one named `scan`, so it is
not mistaken for the window's), and `profiling.zone_names` requires all three.
`audio`, `data-load` and `camera-capture` are **not built**: the dictionary load is on `main`
(see the budgets). Messages: `solo.board_started`,
`solo.word_found`, `solo.cleared`, `scan.captured` are **not built** (the
application has no moment without a duration). The trace's information window
would carry the build (version, commit, dirty); nothing sets it yet.

## The performance tier

Modelled on Cubeulator's `libs/core/bench`: its own binary, `wjs_perf_tests`, linked
against `wjs::lib` and the scanner with its synthetic corpus (no Qt, no window), every
threshold in one file,
`libs/core/bench/budgets.hpp`. **Two halves, and the split is the design.**
| | Holds | Exact? | Runs |
| --- | --- | --- | --- |
| **Work budgets** | What the code *does*: visits, words, tries, trie nodes | Yes, on every machine and compiler | In every plain `ctest` of an optimised build; **skipped in a Debug build**, and so in the three required CI legs |
| **Wall-time checks** | What it *takes*, as a multiple of a calibration workload run in the same process | No, and loose on purpose | Only when asked: `WJS_PERF_TIMING=1` or a `*-perf` test preset; run serially. No automatic CI leg asks |

Cubeulator's reasons carry over: a count fails with a number and a seed rather
than "slower"; a time is the one check that can fail with nothing changed, so it
is a ratio of the fastest of N runs to the calibration's fastest, alternating,
and a budget is several times the ratio.

**What runs where, and what holds the budgets (2026-10-22).** The work budgets
stand aside (SKIP, which is not a failure) in a build that is not optimised, and
all three required CI legs (Windows, Linux, macOS) are Debug, so **no required
leg holds a work budget**: a regression in a visit count, a digest or the
dictionary's size passes the required checks. The places that do run them are
the `Performance (Linux, GCC)` leg of `ci.yml` (the `linux-gcc-release` preset
with the application off, then all of `ctest`), which is **not a required
check**, and the release workflow's Release build. Three more pieces:

- `profiling.perf_budgets_ran` (`tests/profiling/check_perf_ran.cmake`) runs the
  work cases and **fails** when a Release, RelWithDebInfo or MinSizeRel build,
  or any build run with `WJS_REQUIRE_PERF_BUDGETS=1` in its environment, ran
  none of them or found one that skipped; in any other build it reports itself
  skipped, with the reason, so it is never a pass that checked nothing. No
  workflow sets `WJS_REQUIRE_PERF_BUDGETS` yet, so a Debug leg does not use it.
- `WJS_PERF_TIMING=1` turns the wall-time checks on. The `*-perf` test presets
  set it and select the `perf` label; the Performance leg runs that preset on a
  manual dispatch only.
- `WJS_PERF_FRAME_TIMING=1` turns on the window's frame check
  ([The window's frames](#the-windows-frames)); no preset and no workflow sets it.

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

**Built:** every row is in `libs/core/bench/budgets.hpp` with the figure it is a
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
| digest of every solution over the random, rich and empty boards | standard | 228066831253711239 | Exactly |
| `roll.visits.max` 3x3 / 4x4 / 5x5 / 6x6: a roll of the dice, then a solve, over 200 seeds | standard | 471 / 1,636 / 3,748 / 6,457 (the earlier dice: see below) | 589 / 2,050 / 4,690 / 8,080 |
| digest of every solution over those boards | standard | 8038812626269854397 (moved with the dice, 2026-10-19) | Exactly |
| `seed.phase.words`: the test phase | phonics | 249 | Exactly (it pins the phase) |
| `seed.tries.total` over 100 seeds, 3x3 / 4x4 / 5x5 / 6x6 | phonics | 1,739 / 664 / 194 / 187 | 2,000 (the cap: see below) / 830 / 243 / 234 |
| `seed.solves.total`, the same | phonics | 5,503 / 21,653 / 40,663 / 68,049 | 6,880 / 27,100 / 50,900 / 85,100 |
| `seed.visits.total`, the same | phonics | 487,209 / 1,036,442 / 2,525,621 / 5,578,096 | 610,000 / 1,300,000 / 3,160,000 / 6,980,000 |
| `seed.visits.max`, one seeding | phonics | 10,460 / 25,292 / 96,479 / 133,802 | 13,100 / 31,700 / 121,000 / 168,000 |
| `seed.tries.max` | phonics | 20 / 20 / 7 / 7 | 20, the algorithm's own cap (`SeedOptions::max_tries`) |
| `seed.fallbacks` of 100 | phonics | 77 / 5 / 0 / 0 | 97 / 7 / 0 / 0 (**not** the plan's 0: see below) |
| `seed.out_of_range`: a board called a landing that is outside 3 to 5 words | phonics | 0 | Exactly 0, always |
| digest of every board seeded | phonics | 6255957985524133718 (moved when the fallback boards began to list their dead jewels in row order) | Exactly |
| `bind.comparisons`, a path of 9: none / all / mixed | phonics | 279 / 50 / 187 | 349 / 63 / 234 |

The plan's "viable words within 3 to 5, always" row (for `board.verify`) is
`seed.out_of_range`, held as a count of boards outside the range that are not
marked fallbacks.

**The dice were replaced on 2026-10-19.** `data/dice` is now generated from a
Scrabble-style English tile table and not from ENABLE's letter counts
(`data/README.md`), so every board the dice make is a different board. What moved
with them is exact in the test: the dice digest, the standard-session digest and
the standard-session word totals (below). The visit, roll and time figures that
were measured on the dice (the `roll.visits.max` row, the dice table and the
standard-session and time-ratio rows below) are the earlier set's, taken on
2026-10-16, and **have not been repeated**; what the test asserts today is their
budgets, which are ceilings a quarter above them. Treat those figures as the
size of the work and not as what the current dice measure.

### The game rows

The code that makes a board and shows its blocks exists now, and this is what
was added for it and what it measured. **Built, and measured 2026-10-16** (a
Release build, MSVC 19.51, the Windows 11 desktop above; the counts are exact on
every machine, the milliseconds are this machine's, taken by a throw-away
program that is not committed). The cases are in
`libs/core/bench/game_work_test.cpp` and carry the tags `[standard]` or `[phonics]`.

**Standard board (the default).** `wjs::roll` of the project's dice
(`data/dice/<n>x<n>.txt`, the shipped files) with the seeds of the random
boards (`7919 * i + n`, 200 a size), then `wjs::solve` against the full word
list: what making a standard board costs. The zone is `board.roll` (the dice
rolled). The three measured columns are the earlier dice's (see above); the
budget is what the test asserts.

| Size | Most visits of a solve (earlier dice) | Budget | Roll alone | Roll and solve, mean / worst |
| --- | --- | --- | --- | --- |
| 3x3 | 471 | 589 | 2.5 us | 0.03 / 0.08 ms |
| 4x4 | 1,636 | 2,050 | 3.4 us | 0.11 / 0.26 ms |
| 5x5 | 3,748 | 4,690 | 4.6 us | 0.29 / 0.59 ms |
| 6x6 | 6,457 | 8,080 | 5.8 us | 0.53 / 1.02 ms |

The roll is not the cost, as the plan said: a shuffle and a parse, a few
microseconds, and the solve is the rest. With the earlier dice, the worst solves were below
the random boards' (514 / 1,548 / 2,859 / 5,174) on a 3x3 and up to a third above
them from 4x4 up: a die has a vowel on it, so these boards held more words than
a draw by letter counts. The 50 ms budget for making a board is met by a factor
of fifty on this desktop. The digest pins the dice files and the roll together;
it moves when the files do (they are made from the fixed Scrabble-style table:
`data/README.md`), and it moved when they were regenerated.

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
`resting` state (the C++ state; the resting screen was not built) and restored once (`libs/core/bench/game_work_test.cpp`, tag
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

**Standard board: the game session.** The default mode. `wjs::GameSession` over
the project's dice (`wjs::DiceSets`, `data/dice`) and the full dictionary deals a
board with `wjs::deal_standard_board`: it rolls the dice of the chosen size and
solves the roll, and rolls again, with the next seed of its stream, until the board
holds a word, up to 20 rolls (`SessionOptions::max_rolls`). It lands no range of
words. The corpus is the dice rows' seeds, 100 sessions each of 3x3, 4x4, 5x5 and 6x6,
seeded `7919 * i + n`, each dealt, played to the `resting` state (the C++ state; a
find for every word of the dictionary on the board) and restored once (`libs/core/bench/game_work_test.cpp`,
tags `[session][standard]`). The zones are `board.deal` (the rolls it made),
`input.submit` and `board.restore`, as for the phonics board.

The columns are what `libs/core/bench/budgets.hpp` asserts today (`kStandardSessionBudgets`):
ceilings for the rolls and the visits, and the **exact** words of the dictionary on
the 100 boards, which are the current dice's.

| Size | Rolls, total (most for one board): at most | Visits, total / worst deal: at most | Words on the 100 boards (a board, on average): exactly | Restore, visits: at most |
| --- | --- | --- | --- | --- |
| 3x3 | 135 (4) | 23,500 / 558 | 3,273 (33) | 23,300 |
| 4x4 | 125 (2) | 71,400 / 1,390 | 9,948 (99) | 71,400 |
| 5x5 | 125 (2) | 202,000 / 4,690 | 19,165 (192) | 202,000 |
| 6x6 | 125 (2) | 373,000 / 8,080 | 31,396 (314) | 373,000 |

The ceilings are a quarter above what the **earlier dice** measured on
2026-10-16 (108 rolls for the 3x3, the worst board needing three, and 100 for each
larger size; 18,755 / 446, 57,089 / 1,109, 161,035 / 3,748 and 298,302 / 6,457
visits; a deal of 0.05 ms on a 3x3 to 0.67 ms on a 6x6), and have not been
re-measured on the current dice. The word totals moved from 3,432 / 10,897 /
26,311 / 48,121 with the dice and are the test's figures now.

What it says:

- **The standard deal costs one solve, and a 3x3 sometimes two.** With the earlier
  dice, 3.8% of 3x3 rolls hold no word of three letters or more, so 8 of the 100 3x3
  boards were rolled again (the worst three times); 0.15% of 4x4 rolls did, and none
  of those 300 boards on a 4x4 or larger. A deal is a roll (a shuffle of the
  dice) and a solve, 0.05 to 0.67 ms on this desktop with them: two orders of magnitude under
  the 50 ms of the budget table, and under the phonics board's 1.2 to 1.3 ms, which
  seeds and solves up to five times. The worst deal of a 5x5 and a 6x6 was the very
  board that was worst in the dice rows (3,748 and 6,457 visits), because the first
  roll of a corpus seed is the dice rows' board.
- **Playing makes no solve, however many words there are.** Tracing, asking for the
  help, `record()`, `snapshot()` and `rest()` read what the deal kept: 0 solves and 0
  visits over all 400 boards, which hold 63,782 words between them, held exactly. A **restore** makes one solve (100 of 100 for each size), the
  cost of the board again, which is the cost of its deal when it landed first
  time.
- **A board with no word was never dealt**, on any seed, and the words of the
  dictionary on the 400 boards are exactly the figures above, which pins the dice,
  the roll, the dictionary and the deal together. They move with the dice files and
  the dictionary (`data/README.md`), by a person's decision.
- **The record of a standard board lists no words** (`viable_total` counts them), so
  saving it after each find costs what the finds cost and not what the board holds;
  the store keeps no list either.
- The phonics rows above are unchanged by the standard board: the phonics digest of
  the tier is the number it was before the session knew a second mode.

**What was not added.** No wall-time check was added for the roll, the seeding or
the binder: the counts above see the order-of-magnitude slowdown a ratio would,
and a seeding is dominated by the solver, which has them. The game session has
one for each board mode, opt-in like the others (below), because a deal is the one
path of the game that a child waits for. The standard board's word list is the full
ENABLE list with no block list: the accepted MVP choice, pinned by a test
([decisions](../roadmap.md#4-decisions-that-gate-the-plan)); the dice rows are
against it.

### What the screens wait on

**Built (2026-10-20).** Besides the solver, the screens wait on four
things that are not a search: the store that is rewritten whole after every find,
the dashboard a parent opens, the phase directory read at start-up, and the path a
finger hands in. The library keeps no counter for them as it does for the solver,
so the work held is what can be counted from outside and does not depend on the
compiler: bytes, records, rows and words, in `libs/core/bench/data_work_test.cpp` and
`libs/core/bench/data_metrics.cpp`, with the figures in `libs/core/bench/budgets.hpp`
(`kStoreBudgets`, `kStoreHeaviestBytes`, `kDashboardBudget`, `kPhaseLoadBudget`).
Every case carries its board mode, and skips in a build that is not optimised.

| What | Mode | Held | Recorded measurement | Budget |
| --- | --- | --- | --- | --- |
| The store after twenty played boards: boards / finds / vault words (exact), the file, and the bytes the twenty saves wrote | standard | the store's own limits, so a save writes whole | 20 / 300 / 1,000 and a file of 31,013 B (517,233 B written), with the earlier dice | 38,800 B file, 647,000 B written |
| The same | phonics | as above | 20 / 95 / 48; 4,756 B (53,550 B written) | 5,950 B, 67,000 B written |
| The heaviest store the format allows | both | the file a save writes at worst | 88,521 B, 68% of the format's cap | 111,000 B |
| The dashboard built from the phonics board's twenty boards | phonics | sounds / boards met / words found / vault / inputs read | 6 / 81 / 133 / 48 / 163 | 6 / 81 / 133 / 48 exactly, 204 inputs at most |
| The test phase loaded as a directory | phonics | phases / words / graphemes (exact), bytes | 1 / 249 / 6, 4,345 B | the same exactly, 5,440 B |
| A path submitted to a session | both | a miss and a repeated find change nothing but the finds | no solve | exact |

The store is rewritten whole after each change, on the GUI thread, which is why
the written bytes are held and not only the file: on the standard board the
twenty changes write 517 KB for a 31 KB file, because the board's finds and the
vault are at both of their limits. Their times are ratios to a calibration
workload, opt-in and run in no CI leg: serialising the heaviest store 0.026 to
0.040 (budget 0.4), parsing it 0.069 to 0.082 (0.8), saving it to a file 0.018 to
0.022 (1.0), building the dashboard 0.012 to 0.014 (0.15), loading the phase
directory 0.008 to 0.010 (0.5), 200 submits of each kind 0.004 to 0.008 (0.08) and
200 validations of 12 tiles 0.001 to 0.002 (0.02). The save and the phase
directory touch files, so they are held about fifty times the measurement, to catch
a read or a write per word and not a busy disk. The phase directory's 1 ms is the
measurement behind the 10 ms row of the [budgets](#budgets).

### The scene's work

**Built (2026-10-26; the 3D scene's step and atlas budgets are gone with it).** The pixel scene's
work is not a simulation, so what can be counted without a window is the **length of a play**: the
shake's least time, the lid, the drop and the settle that `SceneScript.js` computes from the board's
size. It is held the same way as the solver's: a number, exact, in `libs/core/bench/budgets.hpp`,
measured by `apps/app/tests/unit/scene_script_test.cpp` over the scene's own script. The case
carries the `perf` tag and runs with the scene's script tests, in every build.

| Metric | Computed | Budget |
| --- | --- | --- |
| The longest play (6x6): shake 600, lid 250, drop 1,060 (35 starts 19.4 ms apart and a die's 380), settle 150 | 2,061 ms | 2,200 ms |

The scene's frame cost is a few dozen items moved by whole pixels: no shaders, no textures drawn
ahead, nothing to measure in a window that a count would miss. (The 3D scene's simulation cost 62 µs
a step and 2,560 steps a play, and its atlas 96 cells a 4x4 board; the history is in
[Profiling](../profiling.md#the-smoke-mode).)

### The first show

**Built, and measured 2026-10-20** (the Debug build's smoke, on the same
desktop; the Debug figures are a shape - sanitizers and Tracy dominate - and
the Release figure is the frame check's machine). A screen's or window's first
showing cost what it did because every QML type was instantiated interpreted
and the scene's items were made on the first shake. Three
mechanisms now spread that cost, and the plan is to leave it there rather than
to budget a launch in CI:

- **Ahead-of-time compilation.** The application's QML module builds with the
  type compiler (`ENABLE_TYPE_COMPILER`, qmltc): the kit's frequent types - the
  buttons, icons, jewels, the box, the thumbnails - are C++
  classes with compiled bindings, so first instantiation is no longer
  interpreted. The files qmltc of the shared aqtinstall kit cannot take are
  skipped per file and keep bytecode-compiled bindings (cross-module screens,
  `LayoutMirroring` users, `Timer`/`Connections` users, the translated wording
  singletons); the parent, scan and scene modules stay dynamic for the same
  reasons. The kit's private headers and the qmltc runtime are linked by hand in
  `cmake/FindQt6.cmake` (the kit ships them without their CMake packages).
- **Deferred loading.** The navigation loader is asynchronous, so a screen
  switch never stalls a frame; the board scene sits behind an asynchronous Loader
  that makes it when Solo Play is first shown - the closed box, while the child
  reads it - and keeps it for the run, so the first shake pays no scene-graph
  cost. (Its `active` is a monotonic flag: gating it on the derived state
  booleans destroyed the scene mid-shake once, because those re-evaluate
  non-atomically - measured in the box suites.)

Measured, the Debug smoke's `ui.first_frame_ms` (process start to the first
frame): **14 676 ms to 7 848 ms**; the `*-profiling` build (RelWithDebInfo)
smoke reports **576 ms** for the same launch, inside the budgets table's 2 s
"launch to `home` drawn" with the margin the estimate wanted, and its 2D frames
hold a p50 of 15.9 ms offscreen. A launch budget is still not held by a test;
the frame check ([The window's frames](#the-windows-frames)) holds the
steady-state frames, and a person measures launch on the device as before.

### The window's frames

**Built, and measured 2026-10-19 to 21** (a Release build of the application, on
the Windows 11 desktop above, which has two screens at different rates: see
[the frame on a desktop with two screens](../profiling.md#the-frame-on-a-desktop-with-two-screens)). The frame clock
(`apps/app/src/frame_profiler.cpp`, [Profiling](../profiling.md#the-frame)) keeps a
rolling window of the frames the window swapped, and `--smoke` prints it. The
budgets are the design's table's own, held against **the refresh rate of the
screen the window is on** (the report carries `refresh_hz` and `screen`), and the
check is a wall-time check held like the tier's others: opt-in, and it skips a
debug, Tracy or coverage build and the `offscreen` platform.

| Metric | Budget | What it holds |
| --- | --- | --- |
| `frame.p50` | One display period and 0.3 ms (16.97 ms at 60 Hz, 10.3 ms at 100 Hz) | The typical frame is a refresh: 0.3 ms is room for the display's own period (16.68 ms measured on a 60 Hz one) and a timer's granularity. A fixed 17 ms was right at 60 Hz, failed a 100 Hz screen for a reason that was the display's, and could not see a window that held only half its frames there |
| `frame.p95` | One display period and 2 ms | One frame in twenty may be late, by a little. A busy machine moved a 100 Hz screen's from 10.5 to 14.2 ms, which this catches |
| `frame.max` | 33 ms | No frame over 33 ms during the shake and the drop: one missed refresh is a late frame, two are a stall |
| `frame.over33` | 0 | The same, counted: no frame in the measured window is over it |

A report with no refresh rate is held to 60 Hz. The check is
`apps/app/tests/unit/frame_timing_test.cpp`: it runs `WordJewelsSolver --smoke 240` in a
child process, reads the report's `smoke: key=value` line, requires 240 frames and
a first frame, and compares the four numbers above. A second case, always run,
checks the arithmetic of the budget for 60, 100 and 144 Hz and for a refresh rate
that is not one. The frame case needs a display and a quiet machine; it carries
the `perf` label, so a `*-perf` test preset selects it, but it **skips** unless
`WJS_PERF_FRAME_TIMING=1` is set, which no preset and no workflow sets. So it
**runs in no CI leg**: it is a desktop check, run alone (`RUN_SERIAL`). A window
that cannot present a frame (exit code 3) is reported as a skip with the
application's own sentence, not as a pass.

**The first windowed frame report, 2026-10-20.** The same desktop, a Release
build (`windows-msvc-release`), the check run as designed (`WJS_PERF_FRAME_TIMING=1`
with a real window), when the budget was still a fixed 17.0 ms median and the
report did not yet say which screen the window was on: `frames=240, mean 16.69 ms,
p50 19.72, p95 20.30, p99 20.37, max 22.36, over 33 ms 0, first frame 582 ms`. The
check's 12 assertions passed 11; the one that failed was the median.

**What this page first said about it was wrong.** It read the numbers as a 60 Hz
display whose median landed on "Windows' timer quantisation". That was a guess,
made before the screen was recorded, and the measurements that followed do not
support it. The desktop has two screens, and the window had landed on the
59.95 Hz one:

- **Measured 2026-10-20 and 21, and open:** the same binary holds **p50 10.0 / p95
  10.5 ms** on the 100 Hz screen and shows **p50 19.9 / p95 20.3 ms, with the
  render thread spending about 7 ms a frame in the present,** on the 59.95 Hz one.
  A plain Qt Quick window held 17 ms there, and so did the 3D scene's prototype.
  The 3D scene's own cost was 0.6 ms of GUI thread, 0.6 ms of render thread and
  0.03 ms of recording, at 3 to 6% of the GPU: the scene was not what was slow.
- On the 59.95 Hz screen, switching the Quick Controls style to Basic, the basic or
  windows render loop, or an OpenGL or Direct3D 12 backend puts the application back
  at the screen's rate; Vulkan does not. **The cause is not found.** The Basic style
  is not set in `apps/app/src/main.cpp` (the shake prototype's `main.cpp` sets it), and
  nothing has been changed. The table, the method and the other rows are in
  [Profiling](../profiling.md#the-frame-on-a-desktop-with-two-screens).
- The budget was re-expressed after that, as the display's period plus room (above)
  and not made looser: the check **fails on the 59.95 Hz screen, truthfully,** and
  holds on the 100 Hz one in the runs above. That the budget is an opt-in desktop check and not a
  gate is why this does not stop anything; whether a window that misses its
  screen's refresh there is acceptable is the author's decision, not this page's.
- **Launch is within budget with margin.** First frame 582 ms against the 2 s
  "launch to `home` drawn" row (see [The first show](#the-first-show)); not held by
  a test.

Nothing was measured on a tablet or a phone.

The scene's work, on the same capture: `scene.step` (the dice world's step) is
0.51 ms a frame at a 3x3, and a frame of the shake 0.69 ms, of the drop 0.72 ms
and of the settle 0.98 ms, with `scene.prepare` 1.0 ms a board and `scene.play`
2.1 ms a play (the state change and the first pose of every die). The
[profiling document](../profiling.md#the-smoke-mode) has the whole table and the
machine.

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
| Dealing a session board, 3x3 to 6x6 (**standard**; one deal, the first five corpus seeds; the earlier dice) | 0.05, 0.15, 0.35 and 0.67 ms | 0.003, 0.010, 0.023 and 0.046 | 0.03, 0.08, 0.15 and 0.30 |

Fastest of 5 runs (the load) and of 15 runs of 50 solves, alternating with the
calibration. The proposal's starting values (10, 1.0, 1.0) were against a
stand-in calibration that this replaces; each budget is now six to eight times
what it measures. A load that costs a copy per node, or a solve that allocates
per visit, is a factor of several, which these absorb.

### What the tier does not hold

Not every budget is in `wjs_perf_tests`. It is not the frame time of the
application's window (held opt-in, with the window and Qt, by
`apps/app/tests/unit/frame_timing_test.cpp`: [The window's frames](#the-windows-frames)),
the start of a sound, the speed of the OCR (it is held, as work and as ratios), the dictionary load being off the UI thread (it is not),
or a launch (the frame clock's `ui.first_frame_ms` reports it, but no budget
holds it yet). The load's time is held as a ratio (5.0, opt-in, in no CI leg),
not the 1 s of the budgets table. What is
left is measured on the device by a person with Tracy connected (or Qt's own
`QSG_RENDER_TIMING`, as Cubeulator's page does), and written down as
Cubeulator's `performance.md` does: the build, the machine, what was and was not
measured. The frame check is the one automatable exception: it needs a display,
a quiet machine and a Release build, so it is opt-in and a shared runner never
runs it. The resting screen's zero-frames-in-five-seconds proposal was dropped
with the screen.

## Unknowns

- Every device figure above.
- **Why the application's window presents every 19.9 ms on the 59.95 Hz screen
  of the author's desktop** (and nowhere else it was tried): not found; see
  [The window's frames](#the-windows-frames).
- Whether the dictionary load should leave the UI thread: the 1 s row assumes it
  does, and the load is synchronous at 66 to 68 ms on a desktop.
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
