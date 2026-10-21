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
[performance design](design/performance.md). The work budgets skip in a build that
is not optimised, and a skip is not a failure, so `profiling.perf_budgets_ran`
fails a Release leg (or any leg with `WJS_REQUIRE_PERF_BUDGETS=1` in its
environment) that ran none of them, and reports itself skipped, with the
reason, in the others.

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

### Which program the capture attaches to

**Every Tracy client listens on port 8086**, whatever program it is in, and a
capture started with only an address (`tracy-capture -a 127.0.0.1`) attaches to
whichever client answers first. With two Tracy-built programs on one machine
(a sibling project's, or this one's CLI and its window) the capture can be of
the wrong one: this was found when `tracy-capture` attached to another project
that happened to be running, and the application's own `--smoke` wait for "a
profiler" can be satisfied by a capture of something else.

Give the program a port of its own, and the capture the same one:

```sh
TRACY_PORT=18086 build/windows-msvc-profiling/bin/wjs-cli --repeat 4000 "S E R S/P A T G/L I N E/S E R S" &
$TOOLS/tracy-capture -o solve.tracy -a 127.0.0.1 -p 18086 -f
```

`TRACY_PORT` is read by the client when it starts (an environment variable of
the program, not a build option), and `-p` is `tracy-capture`'s. The profiler
GUI takes the port after the address in its Connect box (`127.0.0.1:18086`).
The tests that capture (`profiling.trace_cli` on 18086, `profiling.trace_app`
on 18087) do this, so they cannot attach to a program that is not theirs.

Use the tools at the **same version as the client** (the wire protocol changes
between releases). The pin is an explicit `overrides` entry in `vcpkg.json`:
0.14.1 (port-version 1).

Headless, from the tools vcpkg built into the build tree:

```sh
TOOLS=build/windows-msvc-profiling/vcpkg_installed/x64-windows/tools/tracy   # x64-linux, arm64-osx elsewhere

# start the capture first; it waits for the program to appear (use TRACY_PORT and -p
# below if anything else with Tracy in it is running)
$TOOLS/tracy-capture -o solve.tracy -a 127.0.0.1 -f &
build/windows-msvc-profiling/bin/wjs-cli --repeat 4000 "S E R S/P A T G/L I N E/S E R S"

# the capture ends when the program exits; then export it
$TOOLS/tracy-csvexport solve.tracy                 # one row per zone: count, total, mean, min, max
$TOOLS/tracy-csvexport -u solve.tracy              # every zone event, with its value
$TOOLS/tracy-csvexport -u -p -f "no zone" solve.tracy   # the plot samples (-p needs -u; the filter
                                                       # leaves out the millions of zone events -u lists)
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
$TOOLS/tracy-csvexport -u -p -f "no zone" app.tracy    # the plots: ui.frame_ms, render.frame_ms, ...
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
the CSV and the GUI show, so a rename is deliberate. Two checks hold them:

- `profiling.zone_names` reads the sources. It checks the shape; that the first
  part of a name is a **layer in the registry** at the top of
  `tests/profiling/check_zone_names.cmake` (one line a layer, with what it is: a
  new layer is a deliberate edit, so a misspelling cannot start a second
  `store` or `solver` in the traces); that no name is used twice; and that the
  required set of zones, plots and QML scopes still exists (removed
  instrumentation is a failed test, as removed unit tests are). The scene's
  stage zones `scene.shake`, `scene.drop` and `scene.settle` are chosen at run
  time by `BoardScene.stageZone()`, so they are pinned as string literals in the
  QML.
- `profiling.trace_cli` and `profiling.trace_app` read a **trace**. They run
  `wjs-cli --repeat` and the application's `--smoke` (offscreen) with the
  capture attached on a port of their own, export the trace and assert that the
  zones and plots a trace of the program is made of are in it: what the sources
  cannot tell, a zone in code that never runs or a name only a run chooses.
  They need a build with Tracy on and the capture tools (the same vcpkg
  feature; `build/<preset>/vcpkg_installed/<triplet>/tools/tracy`), so in a
  Release build they report themselves skipped, with the reason. `render.record_ms`
  is not asked of the offscreen run: its software renderer has no render pass
  to record.

The zone-name registry also names a few layers the next zones are called after
(`assist`, `parent`, `session`, `sound`, `speech`); they are moved up with the
first zone that uses them.

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
| `data.load_phase` | zone | phonics | `PhaseSet::load_directory`, so a `--phase-directory` load | the words of every phase |
| `scene.step` | zone | both | `DiceSim::advance`, once a device sample (one or two a frame) | the simulation steps that call took |
| `scene.prepare` | zone (QML) | both | `BoardScene.prepare`, only when the board's faces are not in an atlas already | the cells of the atlas built |
| `scene.play` | zone (QML) | both | `BoardScene.play`, once a play | none |
| `scene.show` | zone (QML) | both | `BoardScene.show`, once a board is shown without motion | none |
| `scene.shake`, `scene.drop`, `scene.settle` | zone (QML) | both | `BoardScene.step`, one a frame, by the stage the script is in (`BoardScene.stageZone()` chooses the name, so the check pins them as literals) | none |
| `ui.trace` | zone (QML) | both | `FlatBoard.qml`, the trace layer of the flat board, one for each pointer or key event that traces (`TraceLayer.qml` has the same calls and is the same name) | none |
| `ui.screen` | zone (QML) | both | `Main.qml`, the loader's `onLoaded`: the screen transition's work | none |
| `scene2d.play`, `scene2d.step`, `scene2d.show` | zone (QML) | the 2D fallback (**not built**, so not required by any check) | `qml/scene/fallback/BoardScene.qml`, its script calls | none |
| `ui.frame_ms` | plot | both | the frame clock, once a frame | the interval between swaps |
| `ui.first_frame_ms` | plot | both | the frame clock, once | process start to the first frame |
| `render.frame_ms` | plot | both | the frame clock, once a frame | the render thread's own time for the frame |
| `ui.sync_ms` | plot | both | the frame clock, once a frame | the render thread waiting for the GUI thread |
| `render.record_ms` | plot | both | the frame clock, once a frame | the render thread recording the scene graph's main render pass (`beforeRenderPassRecording` to `afterRenderPassRecording`): the scene's draw calls |
| `ui.animate_ms` | plot | both | the frame clock, once a frame the GUI thread was asked for | the GUI thread from the update request to the end of animations and polish (`afterAnimating`): QML, bindings and animation, before the render thread is asked to synchronise |

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
`ui.frame_ms`, and `ui.first_frame_ms` once, from the start of the process to
the first frame. The frame is split into the two threads' shares:

- **GUI thread:** `ui.animate_ms`, the time the GUI thread was busy from the end
  of the last frame's synchronisation (which released it) to the end of this
  frame's animations and polish (`afterAnimating`): the events, the animations'
  tick and the bindings it sets off, the polish. It is the event loop's busy
  time (`QAbstractEventDispatcher`'s awake and aboutToBlock), less the wait for
  the render thread. The scene's own per-frame work (`scene.shake` etc.) is the
  largest part of it in play.
- **Render thread:** `ui.sync_ms`, its wait for the GUI thread, and
  `render.frame_ms`, its own time for the frame; and inside that,
  `render.record_ms`, recording the scene graph's main render pass
  (`beforeRenderPassRecording` to `afterRenderPassRecording`): the scene's draw
  calls. `render.frame_ms` includes the render thread's wait to present, which
  is not work.

The report leaves out the same warm-up frames for all of them (a smoke run's
first frames create the scene and compile its shaders: 150 ms of
`render.frame_ms` once, which is not a frame the player sees again). The swap is connected
directly, so the mark and the plot are sent where the frame ended: the render
thread of Qt's threaded render loop (the GUI thread of the basic one). The mark
is the process's, not a thread's, and the named frame set is `UI`. The
application names its threads `main` (in `main`) and `render` (the first time
the scene graph is initialised, and only when that is a thread of its own: under
the basic render loop the scene graph is the GUI thread's, which keeps its name).

The signals run on the render thread, which the profiler does not control: they
belong to a state that the connections keep alive, which reaches the profiler
only under its own mutex until the destructor says otherwise. So the profiler
may be destroyed while the render thread is inside a frame, and no frame is
handled after the destructor returns (the first version asserted "Called object
is not of the correct type (class destructor may have already run)" in a stress
test of exactly that; `tests/app/frame_profiler_test.cpp` keeps it).

The frame clock keeps a rolling window of the last 4,096 intervals and their
report: count, mean, p50, p95, p99, max, the frames over 16.7 and over 33 ms,
the first frame's time, and the **refresh rate and name of the screen the window
is on**, which the budget is held against. `--smoke` prints it (below); the frame budget is
held on it (tests/app/frame_timing_test.cpp,
[performance design](design/performance.md#budgets)). A trace has the same
numbers under `ui.frame_ms` and `render.frame_ms`.

`app.startup`, `app.load_qml`, `data.load_phase`, the scene's zones and the
seam that opens them from QML are in the table above and in
[QML and the scene](#qml-and-the-scene) below. There are no messages yet:
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

Profiler.mark("ui.screen")             // a moment with no work: a zero-length zone
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
its per-frame stage; the flat board's path work (one per pointer or key event);
a screen's transition; the 2D fallback's script calls. The names are in the
table above. `Profiler.value()` is the QML
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
smoke: frames=240 warmup=60 span_ms=2394.81 p50_ms=10.02 p95_ms=10.45 p99_ms=11.72 max_ms=13.60 over16_7=0 over33=0 first_frame_ms=851.82 render_mean_ms=0.59 render_max_ms=6.58 sync_mean_ms=0.05 sync_max_ms=1.48 refresh_hz=100.00 record_mean_ms=0.03 record_max_ms=0.15 animate_mean_ms=0.60 animate_max_ms=1.37 screen=Acer_X34_P
ui.frame_ms: 240 frames over 2394.81 ms: mean 10.02, p50 10.02, p95 10.45, p99 11.72, max 13.60 ms (the window is on 'Acer X34 P', which refreshes at 100.00 Hz)
ui.frame_ms over 16.7 ms: 0 (0.00%)   over 33 ms: 0
ui.first_frame_ms: 851.82 ms (process start to the first frame)
render.frame_ms: 241 frames, mean 0.59, p95 0.93, max 6.58 ms (the render thread's own time for a frame)
render.record_ms: 240 frames, mean 0.03, p95 0.09, max 0.15 ms (recording the main render pass: the scene's draw calls)
ui.sync_ms: 240 frames, mean 0.05, p95 0.08, max 1.48 ms (the render thread waiting for the GUI thread)
ui.animate_ms: 240 frames, mean 0.60, p95 1.05, max 1.37 ms (the GUI thread busy from the last synchronisation to the end of this frame's animations and polish)
```

The first line is the one a machine reads: `key=value`, no spaces in a value.
`tests/app/frame_timing_test.cpp` holds the frame budget on it, against **the
refresh rate of the screen the window was on** (`refresh_hz`; tests/perf/
budgets.hpp, "The window's frame"): `p50_ms` within one period and 0.3 ms
(16.97 ms at 60 Hz, 10.3 ms at 100 Hz), `p95_ms` within one period and 2 ms,
`max_ms <= 33.0` and `over33 == 0`. A fixed 17 ms is right for a 60 Hz screen
and was wrong for a 100 Hz one: it could not see a window that held only half
its frames there.

The report above is a Release build on a desktop with two screens, with the
window on the 100 Hz one. **Which screen the window is on changes the answer
(next section).**

### The frame on a desktop with two screens

The desktop this was measured on has two: a 3440x1440 landscape at 100 Hz
(`Acer X34 P`) and, as the primary screen, a 1200x1920 portrait at 59.95 Hz
(`SyncMaster`). Windows puts a new window on either, depending on what had the
focus, so the same binary was measured on both, by moving the window in the
first second of `--smoke` (Windows Release build, MSVC 19.51, Qt 6.12, Direct3D
11, the default threaded render loop, an RTX 4090 at 3-6% utilisation; the
median and 95th percentile of `ui.frame_ms`, the mean of `render.frame_ms`):

| Window | On | p50 / p95 ms | `render.frame_ms` mean |
| --- | --- | --- | --- |
| The application, `--smoke 240` | 100 Hz screen | **10.0 / 10.5** | 0.6 to 1.1 ms |
| The application, `--smoke 240` | 59.95 Hz screen | **19.9 / 20.3** | 6.8 to 7.3 ms |
| A Qt Quick window with a spinning rectangle | 59.95 Hz screen | 17 / 18 | |
| A Qt Quick 3D window (MSAA, a cube) | 59.95 Hz screen | 17 / 18 | |
| The shake prototype (the application's scene) | 59.95 Hz screen | 16.7 / 17.2 | 0.8 to 1.2 ms (its own count) |
| The application, `QSG_RENDER_LOOP=basic` or `windows` | 59.95 Hz screen | 16.7 / 17.6 | 0.9 to 1.1 ms |
| The application, `QSG_RHI_BACKEND=opengl` | 59.95 Hz screen | 16.7 / 16.9 | 16.6 ms |
| The application, `QSG_RHI_BACKEND=d3d12` | 59.95 Hz screen | 16.9 / 21.1 | 6.2 ms |
| The application, `QSG_RHI_BACKEND=vulkan` | 59.95 Hz screen | 19.8 / 20.7 | 11.7 ms |
| The application, `QT_QUICK_CONTROLS_STYLE=Basic` | 59.95 Hz screen | 16.7 / 20.3 | 3.5 ms |
| The application, `QT_QPA_PLATFORM=windows:nodarkmode` | 59.95 Hz screen | 16.8 / 20.3 | 4.0 ms |
| The application, every core busy (32 spinning processes) | 100 Hz screen | 10.1 / 14.2 | 1.1 ms |
| The application, the basic render loop | 100 Hz screen | 14.0 / 95.2 | 2.4 ms |

What that says:

- **The measurement is right.** `frameSwapped` is the presentation cadence, and
  the same clock reads 10.0 ms on one screen, 16.7 ms on the other for the
  prototype, and 17 ms for plain Quick windows.
- **The scene is not what is slow.** The scene's frame is 0.6 ms of GUI thread
  (`ui.animate_ms`), 0.6 ms of render thread and 0.03 ms of recording, at 3 to
  6% of the GPU. The prototype, which plays the same scene in a window of its
  own, holds 16.7 ms on the 59.95 Hz screen.
- **It is the window of the application on a 59.95 Hz screen next to a 100 Hz
  one.** There it presents every 19.9 ms (50 frames a second: neither the screen's
  16.7 ms nor a multiple of the other screen's 10 ms vblank that would explain
  it), and the render thread spends 7 ms a frame in the present. A plain Quick
  window and a Quick 3D one do not do it, nor does the prototype. Changing the
  Quick Controls style, switching off Qt's dark-mode integration, using the
  OpenGL backend or the basic render loop each puts it back to the screen's
  rate, which points at how the application's window is set up and presented,
  and at the interplay of the threaded loop with the desktop compositor on a
  mixed-refresh desktop. **Cause not found.** The application draws its own
  controls (the kit), so the `Basic` style costs it nothing and is the first
  thing to try in `main.cpp`; `QSG_RENDER_LOOP` and `QSG_RHI_BACKEND` are
  environment variables Qt reads, and the application could set one itself
  before it creates the window.
- **A busy machine moves the figure too**, which is why the budget is opt-in and
  runs alone (`RUN_SERIAL`): with every core busy the 100 Hz screen keeps its 10.1 ms
median and its 95th percentile goes to 14.2 ms, which is what the `p95` row of
the budget is for.
- **The frame check is honest.** On the 59.95 Hz screen it fails (19.9 ms
  against 16.97), and the earlier reports of a failing p50 on "a 100 Hz
  display" were this: the window was on the other screen. The report now says
  which (`screen=`, `refresh_hz=`), and the budget follows it.

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
it, as the nesting says. `ui.trace` is not in the table: a smoke run sends no
pointer input. `scene2d.*` never appears: the 2D fallback is not built.

The offscreen run this document first printed (60 frames, a p50 at 15.7 ms) was
**not** a desktop 3D measurement; the windowed figures are in
[the frame on a desktop with two screens](#the-frame-on-a-desktop-with-two-screens),
and on a desktop with a display `--smoke 300` prints the line to read.

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
