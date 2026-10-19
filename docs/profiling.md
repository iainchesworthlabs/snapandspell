# Profiling with Tracy

WordJewelsSolver can be profiled with [Tracy](https://github.com/wolfpld/tracy),
the profiler its sibling projects use ([Cubeulator](https://github.com/iainchesworthlabs/cubeulator)
has the same document and the same workflow), so a trace from any of them is
captured, exported and read the same way.

It is **on by default in Debug builds, and free when off**. A build without it
compiles the instrumentation to empty functions and does not need Tracy
installed. A regular Debug build is instrumented, so a running development
session can be connected to by the profiler at any moment, and a Release build
can be instrumented by hand, for data from a real session. A build with it on
is a development build: **it listens on a TCP port, and is never shipped**
([below](#the-profiler-listens-on-the-network)).

What counts as fast enough, and the tests that hold it, are in
[Testing](testing.md#performance-tests) and the
[performance design](design/performance.md).

## Building a profiling binary

```sh
cmake --preset windows-msvc-profiling      # or linux-gcc-profiling / macos-clang-profiling
cmake --build --preset windows-msvc-profiling
```

A **Debug build is instrumented by default** (`windows-msvc-debug` and the
others): connect the profiler to it for development, without any option. Its
timings are a shape, not a number — the sanitizers and no optimisation dominate
them. The `*-profiling` presets are the same instrumentation in RelWithDebInfo
(optimised, with symbols: the right shape for a profiler), with
`WJS_ENABLE_TRACY=ON` and the sanitizers off, so their timings are numbers about
the code. That one option does everything:

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

To instrument a Release build, pass `-DWJS_ENABLE_TRACY=ON` (the default is
already `ON` for a Debug build type). Read a Debug build's trace for structure
and connection, not for timings: the sanitizers' cost dominates every number
there. The perf tests treat a Tracy build as not fit for wall-time checks (they
skip), and run their work budgets as in any optimised build.

### The profiler listens on the network

A Tracy client opens a TCP data port (8086) on every interface and answers UDP
discovery broadcasts, **independently of anything the program does**. That is
what makes it a neutral profiler, and it is why no preset that ships has it:

- The option's default is `ON` for a Debug build type and `OFF` otherwise; the
  `*-debug` and `*-profiling` presets are the instrumented ones, and every
  preset a package is made from keeps it off.
- Tracy is a vcpkg **feature**, not a dependency, and not a default feature.
- The `profiling.shipped_presets` test (`tests/profiling/check_presets.cmake`)
  runs in every `ctest`, and so in every CI leg: it fails if any preset other
  than a `*-debug` or `*-profiling` one enables the option, if a debug preset
  does not, if the default moved, if `tracy` became a base dependency, or if a
  workflow sets the option by hand.

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
between releases). The pin is an explicit `overrides` entry in `vcpkg.json`:
0.14.1 (port-version 1).

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

The application is captured the same way, with its smoke run for a workload
([below](#the-smoke-mode)):

```sh
$TOOLS/tracy-capture -o app.tracy -a 127.0.0.1 -f &
build/windows-msvc-profiling/bin/WordJewelsSolver.exe --smoke 600 --store build/smoke-store.wjs > build/smoke.txt
$TOOLS/tracy-csvexport app.tracy
$TOOLS/tracy-csvexport -p app.tracy    # the plots: ui.frame_ms, render.frame_ms, ui.sync_ms
```

A build with Tracy's `--smoke` gives the profiler that was started first up to
five seconds to attach *before* it loads anything, so `app.startup`,
`app.load_qml` and the store's load are in the capture; the wait is not part of
any number. The report the run prints (`build/smoke.txt` here) is the window's
frames read without a profiler, which is what the frame budget is held on
(tests/app/frame_timing_test.cpp).

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
| `app.startup` | zone | both | `start()` in `src/app/main.cpp`, once: the store and everything the game is made from | none |
| `app.load_qml` | zone | both | the same file, once, around the engine's properties and `loadFromModule` | none |
| `app.scene_create` | zone (QML) | both | `SoloScreen.qml`, once, the scene's first layout | none |
| `data.load_phase` | zone | phonics | `PhaseSet::load_directory`, so a `--phase-directory` load | the words of every phase |
| `scene.step` | zone | both | `DiceSim::advance`, once a device sample (one or two a frame) | the simulation steps that call took |
| `scene.prepare` | zone (QML) | both | `BoardScene.prepare`, only when the board's faces are not in an atlas already | the cells of the atlas built |
| `scene.play` | zone (QML) | both | `BoardScene.play`, once a play | none |
| `scene.show` | zone (QML) | both | `BoardScene.show`, once a board is shown without motion | none |
| `scene.shake`, `scene.drop`, `scene.settle` | zone (QML) | both | `BoardScene.step`, one a frame, by the stage the script is in | none |
| `scene.celebrate` | zone (QML) | both | `SoloScreen.qml`, when the last word lands: the glow and its hold | none |
| `ui.trace` | zone (QML) | both | `TraceLayer.qml`, one for each pointer or key event that traces | none |
| `ui.screen` | zone (QML) | both | `Main.qml`, the loader's `onLoaded`: the screen transition's work | none |
| `scene2d.play`, `scene2d.step`, `scene2d.show` | zone (QML) | the 2D fallback (**not built**) | `qml/scene/fallback/BoardScene.qml`, its script calls | none |
| `ui.frame_ms` | plot | both | the frame clock, once a frame | the interval between swaps |
| `ui.first_frame_ms` | plot | both | the frame clock, once | process start to the first frame |
| `render.frame_ms` | plot | both | the frame clock, once a frame | the render thread's own time for the frame |
| `ui.sync_ms` | plot | both | the frame clock, once a frame | the render thread waiting for the GUI thread |

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

### The frame

The **application** adds the window to that list. Its frame clock is
`src/app/frame_profiler.cpp`: a frame mark named `UI` at every
`QQuickWindow::frameSwapped`, the interval between swaps as the plot
`ui.frame_ms`, the render thread's own time for the frame as `render.frame_ms`
and its wait for the GUI thread as `ui.sync_ms`, and `ui.first_frame_ms` once,
from the start of the process to the first frame. The swap is connected
directly, so the mark and the plot are sent where the frame ended: the render
thread of Qt's threaded render loop (the GUI thread of the basic one). The mark
is the process's, not a thread's, and the named frame set is `UI`. The
application names its threads `main` (in `main`) and `render` (the first time
the scene graph is initialised).

The frame clock keeps a rolling window of the last 4,096 intervals and their
report: count, mean, p50, p95, p99, max, the frames over 16.7 and over 33 ms,
and the first frame's time. `--smoke` prints it (below); the frame budget is
held on it (tests/app/frame_timing_test.cpp,
[performance design](design/performance.md#budgets)). A trace has the same
numbers under `ui.frame_ms` and `render.frame_ms`.

`app.startup`, `app.load_qml`, `app.scene_create`, `data.load_phase`, the
scene's zones and the seam that opens them from QML are in the table above and
in [QML and the scene](#qml-and-the-scene) below. There are no messages yet:
nothing in the application is a moment without a duration.

## QML and the scene

Tracy's API is macros, QML has no preprocessor, and the rule here is that only
`wjs/profiling.hpp` speaks to Tracy. So the QML seam is a QML **singleton**,
`Profiler`, in the scene's own module (`WordJewelsSolver.Scene`,
`src/app/scene/profiler.cpp`), over a zone the wrapper can name at run time:

```qml
Profiler.begin("scene.prepare")
fillAtlas(spareAtlas, shown)
Profiler.value(96)                    // the number beside the innermost zone
Profiler.end()

Profiler.mark("scene.celebrate")      // a moment with no work: a zero-length zone
```

**The discipline**, because QML has no scopes of its own:

- One `begin()` per synchronous moment, and its `end()` in the same function
  after the work. Nesting is allowed and must be last-in, first-out, as the
  calls are.
- One thread: a `begin()` and its `end()` are on the QML thread. Tracy's zones
  belong to the thread that opened them, and an `end()` on another thread would
  not be the same zone.
- A `begin()` without its `end()` leaves the seam one deep. It holds a stack of
  eight, drops a `begin()` past that rather than nesting without bound, and an
  `end()` with nothing open does nothing. A scope that has to span frames (a
  whole stage) is not a moment and is not what this is for: the scene opens one
  a frame, by stage, around that frame's work.
- In a build without Tracy every call is empty: the name is not converted, and
  nothing is allocated.

**The names come from QML**, so the compile-time rule that a zone's name is a
string literal cannot hold there. The wrapper's
`wjs::profiling::RuntimeScope` opens a zone the way Tracy's own transient zones
do (its `ZoneTransient` macro): the name goes with the zone, and the profiler
registers the location itself. The location is therefore the seam's own —
function and file `QML`, line 0 — and a name used again is the same name in the
CSV, so one name is still one place in a trace. The allocation a transient zone
makes is per zone, and only while a profiler is connected; the seam is used a
few times a frame, not in a loop. `tests/profiling/check_zone_names.cmake`
finds the QML names (`Profiler.begin("...")`, `Profiler.mark("...")`) in the
`.qml` and `.js` files and checks their shape and that none is also a C++
zone's.

**Where it is used**, and only there: the scene's `prepare`/`play`/`show` and
its per-frame stage; the trace layer's path work (one per pointer or key
event); a screen's transition; the celebration's start; the 2D fallback's
script calls. The names are in the table above. `Profiler.value()` is the QML
side of a zone's `Value()`: the atlas's cells are the number beside
`scene.prepare`.

## The smoke mode

```sh
build/windows-msvc-profiling/bin/WordJewelsSolver.exe --smoke 300 --store build/smoke-store.wjs
```

`--smoke [<frames>]` runs the window as a capture workload and as the frame
check's program. It shows Solo Play, shakes the box, puts the settled board
away, shakes again, and after `<frames>` measured frames (300 when no number is
given, after 60 warm-up frames) prints the frame-time report and leaves with 0.
The frames are counted by the frame clock, so the report is of the application's
own frames; the seed is fixed (7919) when `--seed` is not given, so the boards
are the same on every run; and a `--store` of its own keeps a run out of the
person's store.

**The report**, to standard output (a windowed program's output is visible when
it is redirected, as above):

```text
smoke: frames=60 warmup=60 span_ms=1583.08 p50_ms=15.69 p95_ms=17.74 p99_ms=309.13 max_ms=722.15 over16_7=12 over33=1 first_frame_ms=144.14 render_mean_ms=0.77 render_max_ms=8.32 sync_mean_ms=0.15 sync_max_ms=1.82
ui.frame_ms: 60 frames over 1583.08 ms: mean 26.68, p50 15.69, p95 17.74, p99 309.13, max 722.15 ms
ui.frame_ms over 16.7 ms: 12 (20.00%)   over 33 ms: 1
ui.first_frame_ms: 144.14 ms (process start to the first frame)
render.frame_ms: 121 frames, mean 0.77, p95 1.29, max 8.32 ms (the render thread's own time for a frame)
ui.sync_ms: 121 frames, mean 0.15, p95 0.92, max 1.82 ms (the render thread waiting for the GUI thread)
```

The first line is the one a machine reads: `key=value`, no spaces in a value.
`tests/app/frame_timing_test.cpp` holds the frame budget on it
([performance design](design/performance.md#budgets)):
`p50_ms <= 17.0`, `max_ms <= 33.0` and `over33 == 0`.

**Those numbers are a real offscreen run, and they are not the 3D scene's.**
Under `QT_QPA_PLATFORM=offscreen` with `QSG_RHI_BACKEND=opengl`, Qt Quick runs
on a renderer that is not QRhi-based, and Quick 3D says so on stderr:

```text
Unexpected condition met: "QSGRendererInterface::isApiRhiBased(rif->graphicsApi())" ...
The Qt Quick scene is using a rendering method that is not based on QRhi and a 3D graphics API.
Qt Quick 3D is not functional in such an environment. The View3D item is not going to display anything.
```

The 2D screens still draw and the frame clock still counts, which is why the
smoke leaves with a report there, but the p99 and max above are the first
frames' setup and the 3D scene is not in them. **Quick 3D cannot render
offscreen**, the frame budget needs a real window, and the smoke is supported
on a desktop with a display. The frame check skips the `offscreen` platform for
that reason.

**Two runs of the application** were captured for this document on 2026-10-19
(MSVC 19.51, RelWithDebInfo, Tracy 0.14.1, on the Windows 11 desktop the
[shake prototype](design/shake-prototype.md) measured; the profiler connected
before the program started, so the start-up zones are in them):

- a windowed run with the profiler attached, killed after 15 s; the session did
  not present the window (the frame marks stop after a few frames), so its
  *frame* numbers are not usable, but the QML animation clock ran and its
  zones are the scene's per-frame work. It was run with `QSG_RENDER_LOOP=basic`
  because the threaded loop in that session slept after the first frames (the
  prototype's window did too);
- an offscreen run for the report above.

| Zone, from the windowed capture | Count | Mean | Min | Max |
| --- | --- | --- | --- | --- |
| `app.startup` | 1 | 71.6 ms | | |
| `app.load_qml` | 1 | 251.2 ms | | |
| `app.scene_create` (QML) | 1 | 0.47 ms | | |
| `data.load_dictionary` | 1 | 70.5 ms (172,823 words) | | |
| `data.load_phase` | 1 | 0.41 ms (7 words, a test directory) | | |
| `data.load_store` | 1 | 0.08 ms | | |
| `data.save_store` | 24 | 2.30 ms | 1.63 | 3.21 |
| `board.deal` | 9 | 98.5 µs | 48.3 | 152.8 |
| `board.roll` | 19 | 10.5 µs | 3.8 | 28.2 |
| `solver.solve` | 10 | 61.7 µs | 20.4 | 124.7 |
| `scene.prepare` (QML) | 9 | 1.01 ms | 0.50 | 3.77 |
| `scene.play` (QML) | 9 | 2.14 ms | 1.19 | 7.73 |
| `scene.show` (QML) | 10 | 0.81 ms | 0.16 | 5.99 |
| `scene.shake` (QML) | 234 | 0.69 ms | 0.45 | 2.21 |
| `scene.drop` (QML) | 144 | 0.72 ms | 0.44 | 1.48 |
| `scene.settle` (QML) | 330 | 0.98 ms | 0.41 | 4.58 |
| `scene.step` (C++) | 708 | 0.51 ms | 0.12 | 1.63 |
| `ui.screen` (QML) | 1 | 64.1 µs | | |

The run played nine deals of the default 3x3 standard board and put each away;
1,517 zones and 3 frame marks. `scene.step` is one call a frame (the stand-in
shakes, so the device's samples are made inside the script), and its 0.51 ms is
the dice world's step for a 3x3, against the prototype's 0.59 ms a frame for the
same size: the zone is the simulation's own time, as it should be. The QML
stage zones (`scene.shake` etc.) are the whole frame's scene work and sit above
it, as the nesting says. `scene.celebrate` is not in the table: a smoke run
does not clear a board. Neither is `ui.trace`: a smoke run sends no pointer
input. `scene2d.*` never appears: the 2D fallback is not built.

The frame percentiles this document can give are the offscreen line above (60
frames, a p50 at 15.7 ms) and **not** a desktop 3D measurement; on a desktop
with a display the same line is the one to read, and `--smoke 300` prints it.

**A profiler gets its moment.** A build with Tracy's `--smoke` waits up to five
seconds for a profiler to connect before it loads anything, then calls
`connected()` until one does or the time is up. That is what puts
`app.startup`, `app.load_qml` and `data.load_phase` in a capture: they run once,
before a profiler started alongside the program could otherwise have found it
(on-demand mode records nothing until one is connected). The wait is not part of
any number: the process clock starts after it. A run with no profiler waits the
five seconds once and then plays; a run of a build without Tracy does not wait
at all.

**A display is needed.** Qt Quick 3D renders through the RHI into a window and
needs a window system; the `offscreen` platform plugin does not give it one.
`--smoke` is supported on a desktop with a display. Under
`QT_QPA_PLATFORM=offscreen` (with `QSG_RHI_BACKEND=opengl`) the 2D screens do
draw and a report is printed, but Quick 3D says on stderr that it is not
functional there and the report is not the 3D scene's; the frame check skips
that platform for the same reason. If no frame at all can be presented, the run
says so on stderr and leaves with 3 after 20 seconds rather than hanging: it
never reports an empty measurement as a result. The exit codes are 0 for a
report, 1 when the QML made no window, 2 when the game could not be made, and 3
when no frame was presented.

**A window that is not presented.** On the machine this package was built on
(the session's window was not composited), the animation clock ran and the
scene's zones were recorded, but `frameSwapped` fired three times in fifteen
seconds: the frame plots are then a handful of samples and the frame budget has
nothing to say. The smoke run's target is not reached and it does not quit by
itself; `tracy-capture` ends when the program is stopped, and the trace still
holds the zones. On a desktop that presents its window the frames are what the
report counts, as they were for the prototype's tables.

**As a capture workload** it is the recipe [above](#capturing-a-trace): start
`tracy-capture`, run `--smoke` with enough frames for the scene's shake and drop
(a shake cycle is about three seconds at 60 Hz, so 600 frames is three of them),
and read the zones and the frame plots out of the trace. The 2D fallback's
`scene2d.*` zones never appear: it is not built.

## Adding instrumentation

Include `"wjs/profiling.hpp"`. It is on `wjs::lib`'s public include path, so
every target that links the library already has it. Nothing else of Tracy's
API is reachable from the sources, on purpose: this is what compiles to
nothing.

| Call | Use |
| --- | --- |
| `auto zone = wjs::profiling::zone<"layer.name">();` | A zone for the rest of the C++ scope. The name is a template argument. `zone.Value(n)` attaches a number (a count, a size) and `zone.Text(ptr, size)` some text; the names are Tracy's own |
| `wjs::profiling::RuntimeScope scope{"layer.name"};` | The same for a name known only at run time, held in a `std::optional` where the scope ends later than its declaration. What the QML seam is built on: a transient zone, so the location is `QML`, line 0, and the name carries it |
| `wjs::profiling::frame_mark()` / `frame_mark("Name")` | A frame boundary on the default set / a named one |
| `wjs::profiling::plot("name", n)` | A number on a timeline. Integer, floating point or `bool` |
| `wjs::profiling::message("text")` | A discrete event. The text is copied |
| `wjs::profiling::thread_name("name")` | Names the calling thread, **once** per thread: every call adds a table entry, so never in a per-frame or per-task path |
| `wjs::profiling::connected()` | Whether a profiler is attached now (Tracy's on-demand state). For a workload that has to wait for one, as `--smoke` does |

Rules that keep it honest:

- **Names are literals, and the compiler holds that.** Tracy keeps the pointer.
  A zone's name is a template argument; the other names are a type that only a
  string literal (or a constexpr array) converts to, so a temporary string does
  not compile. The QML seam's names are the one exception, and they are copied
  with the zone rather than kept ([above](#qml-and-the-scene)).
- **One name is one place.** The source location is a function-local static of
  the template, built the first time the zone runs from the call site, so a
  name used at two places shows the first one's file and line. Use a different
  name for each place. A run-time name is one place by being the same name (its
  location says `QML`), so the seam may open it at more than one call site but
  it may not also be a compile-time zone's.
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
