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
build differed from the plan or settled something.

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
took 1.7 ms. **Loading the dictionary is** (about 60 ms and 18 MB), and in the
MVP it is not even on the critical path: seeding and the scanner verify against
the **phase index**, a few hundred words at most, and the full dictionary is
needed only to tell "real, not today's" from "not a word".

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
| Full dictionary load | 1 s, off the UI thread; `home` is usable meanwhile | Estimate 0.2 to 0.33 s from 55 to 65 ms |
| Phase index load | 10 ms | Hundreds of words |
| Seeding a board | 10 ms typical, 50 ms worst | Up to 20 verifications at about 0.5 ms (estimate) |
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
`data.load_dictionary` (the words), in `wjs::solve` and `Dictionary::load`, and
nothing else: the rest of the table is added with the code it times.

| Layer | Zones, with the value each carries |
| --- | --- |
| app | `app.startup`, `app.load_qml`, `app.scene_create` |
| data | `data.load_dictionary` (words), `data.load_phase` (words), `data.save_store` |
| board | `board.seed` (tries), `board.verify` (viable words) |
| solver | `solver.solve` (words found; visits) |
| phonics | `phonics.bind`, `phonics.segment`, `phonics.sequence` |
| input | `input.submit` (path length). Per-move work is a plot, `input.hit_test_us`, not a zone |
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

**Built:** every row that can be measured is in `tests/perf/budgets.hpp` with
the figure it is a quarter above; the rows about seeding and binding wait for
the code that makes them.

| Metric | Measured | Budget (a quarter above) |
| --- | --- | --- |
| `dictionary.words` (ENABLE) | 172,823 | Exactly this; changes with the data |
| `dictionary.nodes` | 387,889 | Exactly this |
| `solve.visits.max` 3x3 / 4x4 / 5x5 / 6x6 over 200 boards | 514 / 1,548 / 2,859 / 5,174 | 640 / 1,940 / 3,580 / 6,470 |
| `solve.visits` on the four built-to-be-rich boards | 2,832 / 5,496 / 9,443 / 12,350 | 3,550 / 6,900 / 11,800 / 15,500 |
| `solve.words` on each of those four | 1,126 / 1,464 / 1,788 / 327 | Exactly (it pins the corpus) |
| `board.seed.tries.max` over 28 seeds, each size and phase | Not measurable until a phase index exists | 20 (the algorithm's own cap), and `fallbacks` is 0 |
| `board.seed.viable` of every accepted board | | Within 3 to 5, always |
| `phonics.bind` comparisons for a path of 9 | | A count, set when written |

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
- The size of the phase index, and so the seeding's tries.
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
