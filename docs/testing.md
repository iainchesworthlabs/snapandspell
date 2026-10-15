# Testing

The tests are Catch2 unit tests for the library, the platform layer and (when
the Qt window is built) its model, a performance tier with its own binary, and
four checks about the build itself. They run in CI on Windows (MSVC), Linux
(GCC) and macOS (AppleClang), 43 tests each, of which the eight of the
performance tier skip themselves in a Debug build (see
[Performance tests](#performance-tests)); the Qt model tests run only where the
window is built, which is Windows so far.

## Running them

After a build (see [Building](building.md)):

```sh
ctest --preset windows-msvc-debug              # or linux-gcc-debug, macos-clang-debug
ctest --preset windows-msvc-debug -R platform  # just the two build checks
```

Every Catch2 `TEST_CASE` is registered with CTest by name
(`catch_discover_tests`), so a failing case is named in the CTest output, and
`ctest --preset <preset>` shows the output of any that fail. To run the
unit-test program directly, with Catch2's own filters:

```sh
build/windows-msvc-debug/bin/wjs_unit_tests --list-tests
build/windows-msvc-debug/bin/wjs_unit_tests "[solver]"
```

The tags are `[board]`, `[dictionary]`, `[scoring]`, `[solver]`, `[platform]`
and `[profiling]`, and `[app]` in `wjs_app_tests`.

## What there is

| Test file | Tests | What it holds down |
| --- | --- | --- |
| `tests/unit/board_test.cpp` | `wjs::Board::parse` | One letter per tile or separated tiles (`Qu I T`), `/` and `\r\n` as row breaks, each way a board is refused (empty, ragged, bad tile, over 16 by 16), `contains` |
| `tests/unit/dictionary_test.cpp` | `wjs::Dictionary` | Case-insensitivity, prefix versus word, words refused (apostrophe, hyphen, digit, empty), duplicates counted once, stepping through the trie, loading skips blanks and non-words, a missing file is an error value, a file round trip |
| `tests/unit/scoring_test.cpp` | `wjs::score_word` | The printed Boggle scores from 2 letters to 12, and the letter-rarity scores for `E`, `Z`, `EZ` |
| `tests/unit/solver_test.cpp` | `wjs::solve` | Adjacency, the minimum length, the path reported for a word, a tile used at most once, a word reached by several paths reported once, the `Qu` tile, the scoring scheme applied, an empty dictionary; and the work counter (`wjs/solver_work.hpp`): visits, solves and the largest, reset, and that each thread has its own |
| `tests/unit/profiling_test.cpp` | `wjs/profiling.hpp` | That the variant CMake chose is the one it was asked for, that every call can be made with no profiler connected, and that a zone cannot be copied or moved |
| `tests/unit/paths_test.cpp` | `wjs::platform::executable_path` | That it names the running test program: absolute, an existing file, called `wjs_unit_tests` |
| `tests/app/solver_model_test.cpp` | The window's `SolverModel` | Solving once a dictionary is loaded, a dictionary that cannot be opened, a board that cannot be read. Built only with `WJS_BUILD_APP` |
| `tests/perf/` | The solver and the word list, as `wjs_perf_tests` | See below: [Performance tests](#performance-tests) |
| `tests/profiling/check_presets.cmake` | `CMakePresets.json`, `vcpkg.json`, `cmake/Tracy.cmake` | See below: `profiling.shipped_presets` |
| `tests/profiling/check_zone_names.cmake` | Every `zone<"...">` under `src/` | See below: `profiling.zone_names` |
| `tests/platform/check_fragments.cmake` | Every `src/platforms/*/platform.cmake` | See below: `platform.fragments` |
| `tools/checks/check_platform_isolation.ps1` | The source tree | See below: `platform.isolation` |

The first solver case uses the board of the
[README example](https://github.com/iainchesworthlabs/wordjewelssolver/blob/develop/README.md#a-worked-example):

```text
C A T
O R E
D G S
```

Of the dictionary `CAT CAR CARE ARE TAR TEA DOG COD ROD CATS REST AT`, exactly
`CARE ARE CAR CAT COD DOG ROD TAR TEA` are found, in that order, because `CATS`
and `REST` cannot be traced and `AT` is below the minimum length.

## The build checks

**`platform.fragments`** runs `tests/platform/check_fragments.cmake`. A build
only includes the one `platform.cmake` for the platform it targets, so a typo in
another platform's fragment (a renamed file, a hook that was never defined)
would otherwise be found only by someone on that platform. The check includes
every fragment under `src/platforms/`, each in a process of its own, and holds
it to the contract in `cmake/PlatformContract.cmake`: every variable and hook
defined, `WJS_PLATFORM_DESKTOP` either `ON` or `OFF`, every file it compiles
present. It also checks that `cmake/Platform.cmake` has a branch selecting each
fragment, and that every branch has a fragment. It runs on whatever host runs
the tests, so a Linux machine checks the Windows and macOS fragments.

**`platform.isolation`** runs `tools/checks/check_platform_isolation.ps1`
against the source tree. It is registered only when `pwsh` is found. What it
checks, and why, is in [Platform isolation](platform-isolation.md#enforcement).
You can run it by hand from the repository root:

```sh
pwsh ./tools/checks/check_platform_isolation.ps1
```

**`profiling.shipped_presets`** runs `tests/profiling/check_presets.cmake`. A
profiling build has a Tracy client in it, which opens a TCP port and answers
discovery broadcasts, so nothing that ships may have it. The check resolves
`inherits` the way CMake does and fails if any configure preset other than a
`*-profiling` one turns `WJS_ENABLE_TRACY` on (a release preset, above all), if
the option's default in `cmake/Tracy.cmake` is not `OFF`, if `tracy` is a base
dependency of `vcpkg.json` or `profiling` a default feature, if the top-level
`CMakeLists.txt` requests the feature other than under that option, or if a CI
workflow turns the option on by hand. It reads only the presets' JSON, so it
runs on every host and in every CI leg. [Profiling](profiling.md) is the page
about why.

**`profiling.zone_names`** runs `tests/profiling/check_zone_names.cmake`. A
zone's source location is a function-local static of `zone<"name">()`, so one
name is one place and a name used twice would show the first place's file and
line for both. It scans `src/` and fails on a name that is used twice or is not
`<layer>.<thing>`, lower case and dotted.

## Performance tests

`wjs_perf_tests` is a binary of its own, linked against `wjs::lib` and nothing
else (no Qt, no platform layer), so that it can be built and run alone and so
that nothing else's allocator churn is in the way of a stopwatch. It is
modelled on [Cubeulator's](https://github.com/iainchesworthlabs/cubeulator/blob/main/docs/testing.md#performance-tests)
and is the implementation of the tier in the [performance
design](design/performance.md). **Two halves, and the split is the design:**

| | Holds | Exact? | Runs |
| --- | --- | --- | --- |
| **Work budgets** | What the code *does*: visits, words found, the word list's words and trie nodes, and a digest of every result | Yes, on every machine and compiler | In every `ctest` of an optimised build |
| **Wall-time checks** | What it *takes*, as a multiple of a calibration workload run in the same process | No, and loose on purpose | Only when asked: `WJS_PERF_TIMING=1`, or a `*-perf` test preset |

```sh
ctest --preset windows-msvc-release -L perf        # the tier, work budgets only
ctest --preset windows-msvc-perf                   # the tier, with the wall-time checks
build/windows-msvc-release/bin/wjs_perf_tests "[.perf-report]"   # what each metric is now
```

(`linux-gcc-` and `macos-clang-` likewise.) Every case carries the `perf`
label. A **Debug or sanitizer build skips** the work budgets (reported as
skipped, not failed: the corpus is much longer there, and the counts are the
same in every build), and a coverage or Tracy build skips the wall-time checks.

### The corpus

The same boards on every machine, compiler and standard library:

- **Random boards:** 200 each of 3x3, 4x4, 5x5 and 6x6. Board `i` of size `n` is
  drawn from `std::mt19937` seeded `7919 * i + n`, one draw a tile in row-major
  order, **taken modulo** (never `std::uniform_int_distribution`, whose algorithm
  is each standard library's own). A draw picks a letter by ENABLE's letter
  counts, in integer arithmetic. It is a test corpus, not a set of dice.
- **Four boards built to hold many words** (4x4, 5x5 and two 6x6) and **two with
  none** (one letter throughout).

### What is counted

| Metric | Budget |
| --- | --- |
| `dictionary.words`, `dictionary.nodes` | Exactly 172,823 and 387,889. They change with the data (`data/README.md`) |
| `solve.visits.max.<n>x<n>` over the 200 boards | A quarter above the measurement: 640, 1,940, 3,580, 6,470 |
| `solve.visits.rich.*` | A quarter above: 3,550, 6,900, 11,800, 15,500 |
| `solve.words.rich.*` | Exactly 1,126, 1,464, 1,788, 327 |
| `solve.visits.empty.*`, `solve.words.empty.*` | Exactly 100 and 256; 0 |
| `solve.digest` | Exactly. An FNV-1a digest of every word, score and path over the whole corpus, in the order `solve()` returns them |

A **visit** is a tile step that stayed inside the dictionary's trie: the
letters of the path so far, with the tile's own, still begin some word. The
solver counts them in a member of its search and adds the total to a
thread-local tally (`wjs/solver_work.hpp`) once as the solve ends, so the cost
is one add a solve and nothing per visit; nothing in the application reads it.
`wjs::solve`'s signature is as it was. A separate case holds the solver's tally
to a second walk written out in the test with nothing but the dictionary's
trie, so the budgets are held to a definition and not to an implementation.

Every threshold is in `tests/perf/budgets.hpp`, with the figure it is above
beside it. The measurements and the machine are in the
[performance design](design/performance.md#what-was-measured).

### What a failure means, and how to move a budget

A work failure names the metric, the corpus seed that gave a maximum, and both
numbers: the search did more than it did. Find out why before touching the
number. A budget moves when the work **genuinely** changed (a different move
order, a new dictionary), by a person, with the reason in the commit, and never
to turn a red run green. `solve.digest` is the same: it moves when the solver's
output was meant to change, and the failure says which number to set.

A time failure is the ratio of the task's fastest run to the calibration
workload's fastest run, alternating, so that the machine's speed and a stretch of
load cancel. The budgets are several times what the ratio measures, so they are
for the order-of-magnitude slowdown a count cannot see (an allocation or a copy
per visit). They must never fail because another process was busy; if one does,
the budget is too tight, and that is a finding about the budget. The timing
cases are `RUN_SERIAL`, so `ctest -j` runs them one at a time.

The Linux (GCC) Release build runs the work budgets on every push, in the
`Performance` leg ([CI](ci.md)); the Debug legs prove only that the tier
compiles warning-free.

### Capturing it

A short run of `wjs-cli` is over before a profiler attaches; the tier's binary
runs for seconds, which makes it a good thing to capture:
[Profiling](profiling.md#capturing-a-trace).

## Sanitizers, warnings and Debug

Debug builds run the tests under AddressSanitizer (and UBSan on GCC and Clang),
and every first-party target is built with warnings as errors. A sanitizer
finding aborts the test. Details, and how to turn them off, are in
[Building](building.md#sanitizers-and-warnings).

## Coverage

The `linux-gcc-coverage` preset builds the library, the platform layer and the
tests with gcov instrumentation and the sanitizers off. Run `ctest` against it
and the `.gcda` files hold what was executed. The report tool and the threshold
(if CI enforces one) are described in [CI](ci.md); this repository does not
fix a number here.

## Adding a test

- Put the file in `tests/unit/` (library and platform interface), `tests/app/`
  (the window's model) or `tests/platform/` (a case only one platform can run),
  and add it to the executable's source list in `tests/CMakeLists.txt`. The
  lists are explicit, not globbed.
- Name each `TEST_CASE` as a sentence that says what is true, and give it a
  tag. The name is what CTest shows.
- A test that needs a file writes it to the system's temporary directory and
  removes it, as `dictionary_test.cpp` does. No test reads a real word list:
  tests build their dictionaries from a few words in the test itself.
- A test must not need a camera, a network or a word list that is not in the
  tree.
- A change in behaviour comes with the test that shows it; a bug fix with the
  test that failed.

## What is not tested

- **`wjs-cli` itself.** Nothing runs the program: option parsing, the exit
  codes and the output format are checked by hand. The library behind it is
  tested, and the README example is the solver case above.
- **The window.** The model is tested. The QML has been loaded offscreen to see
  that it starts and reads its `--dictionary` argument, and not exercised by a
  test or run on a real screen.
- **The console code on Linux and macOS.** It does nothing there (a POSIX
  terminal already carries UTF-8), so there is nothing to observe. The path code
  is covered by `paths_test` in CI on both.
- **A UTF-8 console on Windows**, and files opened by paths longer than
  `MAX_PATH` (a [known problem](building.md#platform-notes)).
- **Performance of the application.** The tier holds the solver and the word
  list. It is not the frame time of the window, a launch, or anything on a
  device, and nothing has been measured on a tablet or a phone
  ([performance design](design/performance.md)). Boards larger than 6x6 are not
  in the corpus.
- **Real word lists.** The tests use made-up lists of a dozen words. The
  behaviour on a list of a few hundred thousand words has not been checked by
  a test.
- **Threads.** The library keeps no shared state between calls, but nothing
  tests solving from several threads at once.
