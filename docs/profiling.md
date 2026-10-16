# Profiling with Tracy

WordJewelsSolver can be profiled with [Tracy](https://github.com/wolfpld/tracy),
the profiler its sibling projects use ([Cubeulator](https://github.com/iainchesworthlabs/cubeulator)
has the same document and the same workflow), so a trace from any of them is
captured, exported and read the same way.

It is **off by default and free when off**. A normal build compiles the
instrumentation to empty functions and does not need Tracy installed. A build
with it on is a development build: **it listens on a TCP port, and is never
shipped** ([below](#the-profiler-listens-on-the-network)).

What counts as fast enough, and the tests that hold it, are in
[Testing](testing.md#performance-tests) and the
[performance design](design/performance.md).

## Building a profiling binary

```sh
cmake --preset windows-msvc-profiling      # or linux-gcc-profiling / macos-clang-profiling
cmake --build --preset windows-msvc-profiling
```

The preset is RelWithDebInfo (optimised, with symbols: the right shape for a
profiler) with `WJS_ENABLE_TRACY=ON` and the sanitizers off. That one option does
everything:

- It selects the real `wjs/profiling.hpp` (below) instead of the empty one.
- It adds the `profiling` feature to the vcpkg manifest, which supplies the Tracy
  **client** (`tracy[on-demand]`, with its crash handler off) that the programs
  link, and Tracy's **command-line tools** (`tracy-capture`, `tracy-csvexport`),
  built for the *host* so that a cross-build does not try to build them for the
  device.

On Windows the client is a DLL: vcpkg puts `TracyClient.dll` beside each program
it was linked into. **On Linux** vcpkg builds the tools from `dbus` upward
(`libxcrypt`, `libsystemd`), which needs `autoconf`, `autoconf-archive`,
`automake`, `libtool`, `libltdl-dev` and `python3-venv` installed first; the first configure
takes several minutes (6 in a clean container), and vcpkg's binary cache makes
the next one quick. The tools are in
`build/<preset>/vcpkg_installed/<triplet>/tools/tracy/`.

To turn it on for another configuration, pass `-DWJS_ENABLE_TRACY=ON`. Do not
profile a Debug build: the sanitizers' cost dominates every number, and
`cmake` warns if you try. The perf tests treat a Tracy build as not fit for
wall-time checks (they skip), and run their work budgets as in any optimised
build.

### The profiler listens on the network

A Tracy client opens a TCP data port (8086) on every interface and answers UDP
discovery broadcasts, **independently of anything the program does**. That is
what makes it a neutral profiler, and it is why no preset that ships has it:

- The option defaults to `OFF`; only the `*-profiling` presets turn it on.
- Tracy is a vcpkg **feature**, not a dependency, and not a default feature.
- The `profiling.shipped_presets` test (`tests/profiling/check_presets.cmake`)
  runs in every `ctest`, and so in every CI leg: it fails if any other preset,
  a release preset above all, enables the option, if the default moved, if
  `tracy` became a base dependency, or if a workflow turns it on by hand.

`TRACY_ONLY_LOCALHOST` and `TRACY_NO_BROADCAST` are compile definitions of the
Tracy client *library*, so this project cannot set them from its own sources;
they would have to be built into the `tracy` port (a vcpkg triplet or overlay).
Nothing here does that. Until it does, treat the open port as exposed: profile
on a trusted network, or reach a remote machine through an SSH tunnel.

## Capturing a trace

The client runs in **on-demand** mode: it records nothing until a profiler
connects, so an instrumented build left idle does no work beyond a branch per
zone. Two consequences:

- Events that happen before the profiler connects are not recorded.
- **A profiler takes about a second to find a freshly started program**
  (measured: 0.7 s). `wjs-cli` solves a board in a tenth of a second, so on its
  own it is over before a capture can attach. Give it work to do for the capture
  with `--repeat <n>` (it solves the board `n` times and prints one result).

Use the tools at the **same version as the client** (the wire protocol changes
between releases). The pin is whatever `tracy` resolves to in the vcpkg
baseline: 0.13.1.

Headless, from the tools vcpkg built into the build tree:

```sh
TOOLS=build/windows-msvc-profiling/vcpkg_installed/x64-windows/tools/tracy   # x64-linux, arm64-osx elsewhere

# start the capture first; it waits for the program to appear
$TOOLS/tracy-capture -o solve.tracy -a 127.0.0.1 -f &
build/windows-msvc-profiling/bin/wjs-cli --repeat 4000 "S E R S/P A T G/L I N E/S E R S"

# the capture ends when the program exits; then export it
$TOOLS/tracy-csvexport solve.tracy                 # one row per zone: count, total, mean, min, max
$TOOLS/tracy-csvexport -u solve.tracy              # every zone event, with its value
$TOOLS/tracy-csvexport -u -p solve.tracy           # the plot samples (needs -u)
$TOOLS/tracy-csvexport -m solve.tracy              # the messages
```

What that prints for the zones (a 4x4 built to hold 1,126 words, MSVC, on one
desktop; the first 160 or so solves were over before the capture attached):

```text
name,src_file,src_line,total_ns,total_perc,counts,mean_ns,min_ns,max_ns,std_ns
solver.solve,...\src\lib\src\solver.cpp,134,2920383891,81.73,3839,760714,564434,1904351,238972
```

`tracy-capture`: `-o` output file, `-a` the program's address, `-f` overwrite
an existing file, `-s <seconds>` stop after that long. `tracy-csvexport`: `-f
<text>` filters zone names (so `-f solver.` is the solver), `-e` reports self
time rather than inclusive.

The dictionary load is over before the capture attaches in a `wjs-cli` run, so
its zone is missing from the capture above. The performance-test binary does
the load several times, which is how to see it:

```sh
$TOOLS/tracy-capture -o perf.tracy -a 127.0.0.1 -f &
build/windows-msvc-profiling/bin/wjs_perf_tests "[.perf-report-timing]"
$TOOLS/tracy-csvexport perf.tracy
```

```text
data.load_dictionary,...\src\lib\src\dictionary.cpp,90,363791093,7.02,5,72758218,67037960,81391971,...
solver.solve,...\src\lib\src\solver.cpp,134,2103315138,40.59,1600,1314571,588494,3259660,...
```

(Each load holds `172823` as the zone's value: the words.)

Interactively, run the `tracy-profiler` GUI of the same version (a release from
<https://github.com/wolfpld/tracy/releases>, or built from that tag), press
**Connect** and enter `127.0.0.1`, or pick the program from the discovery list.
The GUI is not part of the build.

## Reading a trace

A trace holds the zones that ran while the profiler was connected, with their
times, each zone's value and its source location, the plots, and the threads.

**The CSV, one row a zone** (`tracy-csvexport trace.tracy`):

| Column | Meaning |
| --- | --- |
| `name`, `src_file`, `src_line` | The zone and the place it was first run from (one name is one place) |
| `total_ns`, `total_perc` | Time inside the zone, summed, and its share of the trace's span |
| `counts` | How many times it ran while connected |
| `mean_ns`, `min_ns`, `max_ns`, `std_ns` | The distribution of one run. `min` is the cost of the code; `max` and `std` are what the machine added |

Inclusive times: a zone's time contains the zones inside it (`-e` for self
time). `-f` filters by name.

**Every event** (`-u`) adds `ns_since_start`, `exec_time_ns`, the `thread` and the
zone's `value`: here the words found by a solve (`solver.solve`) or loaded
(`data.load_dictionary`). With `-p` it lists the plot samples instead, which is
where `solver.visits` is, one sample a solve.

Reading the capture above: 3,839 solves of a board with 1,126 words ran in 2.9
s, a mean of 0.76 ms against a minimum of 0.56 ms. The search is the same every
time (a visit count does not vary), so the spread is the machine. A **change**
in `min_ns` or in the visits is the code's; a change in `max_ns` alone is not.

**The GUI** shows the same on a timeline per thread: zones as bars nested by
call, the plots under them, and the statistics window with the same table. The
**information** window will carry the build (version, commit); nothing sets it
yet.

## What is instrumented

Names are `<layer>.<thing>`, lower case, dotted, string literals. They are what
the CSV and the GUI show, so a rename is deliberate; `profiling.zone_names`
checks the shape and that no name is used twice.

Only code that exists is instrumented. The [performance design](design/performance.md#what-to-instrument)
lists the zones the rest of the application will carry, and they are added with
the code they time.

The **board mode** matters to how a trace reads. The standard board (dice, 3x3
to 6x6) is the default and the seeded phonics board is a parent's switch, so a
standard board's trace has `board.roll` and a `solver.solve`, and a phonics
board's has `board.seed` with many `solver.solve` inside it and `phonics.bind`
as the finger moves. The column below says which.

| Name | Kind | Mode | Where | Carries |
| --- | --- | --- | --- | --- |
| `solver.solve` | zone | both | `wjs::solve`, once per call | the words found |
| `solver.visits` | plot | both | the same, once per call | the visits of that search |
| `data.load_dictionary` | zone | both (on the path to the first board of the standard one) | `wjs::Dictionary::load`, so every file or stream load | the words loaded |
| `board.roll` | zone | standard | `wjs::roll`, once per board | the dice rolled (the tiles) |
| `board.seed` | zone | phonics | `wjs::seed_board`, once per board | the verifications it spent (1 to `max_tries`) |
| `board.verify` | zone | phonics | each verification inside `board.seed` | the words of the phase found on that board |
| `phonics.bind` | zone | phonics | `GraphemeBinder::bind`, so every call that binds a path | the comparisons it made: a grapheme held against the tiles at a block |
| `board.deal` | zone | both | `GameSession::shake`, once per board | the seedings it spent on the phonics board (1 to `max_seedings`), the rolls on the standard board (1 to `max_rolls`) |
| `board.restore` | zone | both | `GameSession::restore` | none |
| `input.submit` | zone | both | `GameSession::trace` of two tiles or more in `playing`; not for an event that is refused or a touch of one tile | the number of tiles in the path |

`board.seed` has no plot of its own. The solves it makes, the verifications and
the guided fill's trials, are `solver.solve` zones inside it, each with its
`solver.visits` sample, so the CSV's `solver.solve` row inside a seeding trace
is the fill's cost and `board.verify`'s is the verifications'. A seeding of a
6x6 on the test phase is about 680 solves of which two are verifications
(the [performance design](design/performance.md#the-game-rows) has the
figures).

What a capture of the report shows (`wjs_perf_tests "[.perf-report]"` in the
profiling build, headless as above; MSVC, the same desktop, the profiler
attached for the run, which costs tens of nanoseconds a zone):

```text
name,total_ns,counts,mean_ns,min_ns,max_ns
board.seed,1298149292,400,3245373,334247,15298926
board.verify,34324224,2784,12329,3570,57459
board.roll,6897909,1600,4311,2400,10570
phonics.bind,6050,3,2016,1220,2500
```

The counts are the budgets' own numbers: `board.seed` is the 100 seeds of each
of four sizes; `board.verify` is 2,784, which is the verifications of the
[work budgets](design/performance.md#the-game-rows) summed (1,739 + 664 + 194
+ 187); `board.roll` is two passes of the 800 rolls (the metrics and the
digest); and `phonics.bind` is the three paths. A mean of a zone under a
profiler is a little above the same zone's cost without one, and a bind is a
microsecond, so the zone is a few percent of it.

`wjs-cli` names its thread `main`.

Visits are the tile steps that stayed inside the dictionary's trie: the measure
of a search that does not depend on the machine, and the one the
[work budgets](testing.md#performance-tests) are held on. The seeding's are the
verifications and the solves it made, which it reports in its result, and the
visits of those solves; the binder's are the comparisons, which `phonics.bind`
carries as its value. Each is held in the same tier, and each has a budget for
the mode it is part of.

There are no frame marks, messages or other threads yet; the application will
add them with the window.

## Adding instrumentation

Include `"wjs/profiling.hpp"`. It is on `wjs::lib`'s public include path, so
every target that links the library already has it. Nothing else of Tracy's
API is reachable from the sources, on purpose: this is what compiles to
nothing.

| Call | Use |
| --- | --- |
| `auto zone = wjs::profiling::zone<"layer.name">();` | A zone for the rest of the C++ scope. The name is a template argument. `zone.Value(n)` attaches a number (a count, a size) and `zone.Text(ptr, size)` some text; the names are Tracy's own |
| `wjs::profiling::frame_mark()` / `frame_mark("Name")` | A frame boundary on the default set / a named one |
| `wjs::profiling::plot("name", n)` | A number on a timeline. Integer, floating point or `bool` |
| `wjs::profiling::message("text")` | A discrete event. The text is copied |
| `wjs::profiling::thread_name("name")` | Names the calling thread, **once** per thread: every call adds a table entry, so never in a per-frame or per-task path |

Rules that keep it honest:

- **Names are literals, and the compiler holds that.** Tracy keeps the pointer.
  A zone's name is a template argument; the other names are a type that only a
  string literal (or a constexpr array) converts to, so a temporary string does
  not compile.
- **One name is one place.** The source location is a function-local static of
  the template, built the first time the zone runs from the call site, so a
  name used at two places shows the first one's file and line. Use a different
  name for each place.
- **Arguments are evaluated when profiling is off.** Unlike Tracy's macros, which
  vanish, the empty functions are still called. Keep what goes into `plot()` and
  `Value()` cheap and free of side effects.
- **Put a zone where there is work, not on a tight inner loop.** A zone costs
  tens of nanoseconds while a profiler is attached, and a guard variable and a
  branch otherwise. A search is a few thousand visits; a zone per visit is noise
  and slower than the search.
- **Do not name anything after one of Tracy's macros.** Its header defines
  `ZoneName`, `ZoneText`, `ZoneValue`, `ZoneColor`, `FrameMark`, `FrameImage`,
  `LockMark` and everything that begins `Tracy`, in every file that includes the
  real `wjs/profiling.hpp`. This was found, not guessed: the first version of the
  wrapper had a type called `ZoneName`.

## Why there are no macros

The repository holds no `#define` and no `#if` in first-party C++
([Platform isolation](platform-isolation.md)), and Tracy's API is macros. What
its macros do is ordinary C++ in namespace `tracy`, so the wrapper calls that:

- `src/lib/variants/profiling-enabled/wjs/profiling.hpp` includes Tracy and
  aliases `tracy::SourceLocationData` and `tracy::ScopedZone`. `zone<"name">()`
  makes a function-local `static` `SourceLocationData` from the call site's
  `std::source_location` (a default argument is evaluated at the call) and
  returns a `ScopedZone` built in place, as Tracy's `ZoneNamedN` does with a
  `static constexpr` and a local.
- `src/lib/variants/profiling-disabled/wjs/profiling.hpp` has the same
  interface with an empty zone and empty functions.
- CMake puts exactly one of the two directories on `wjs::lib`'s include path
  (`cmake/Tracy.cmake`, `src/lib/CMakeLists.txt`). No source file asks which.
- `TRACY_ENABLE` and `TRACY_ON_DEMAND` are compile definitions of Tracy's own
  CMake target, which `wjs::tracy` links; they are not written in any source.

`wjs::profiling::enabled` says which variant a build has, and a unit test pins
it against the CMake option.

## In the browser and elsewhere

Tracy cannot run in a browser tab (its client opens a listener). Nothing here
targets the browser yet; when something does it will need a recorder of its own
behind the same functions, as Cubeulator has.
