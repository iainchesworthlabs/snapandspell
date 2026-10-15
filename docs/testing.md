# Testing

The tests are Catch2 unit tests for the library, the platform layer and (when
the Qt window is built) its model, plus two checks about the build itself. They
have been run on Windows with MSVC only; Linux and macOS have not been built.

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

The tags are `[board]`, `[dictionary]`, `[scoring]`, `[solver]` and
`[platform]`, and `[app]` in `wjs_app_tests`.

## What there is

| Test file | Tests | What it holds down |
| --- | --- | --- |
| `tests/unit/board_test.cpp` | `wjs::Board::parse` | One letter per tile or separated tiles (`Qu I T`), `/` and `\r\n` as row breaks, each way a board is refused (empty, ragged, bad tile, over 16 by 16), `contains` |
| `tests/unit/dictionary_test.cpp` | `wjs::Dictionary` | Case-insensitivity, prefix versus word, words refused (apostrophe, hyphen, digit, empty), duplicates counted once, stepping through the trie, loading skips blanks and non-words, a missing file is an error value, a file round trip |
| `tests/unit/scoring_test.cpp` | `wjs::score_word` | The printed Boggle scores from 2 letters to 12, and the letter-rarity scores for `E`, `Z`, `EZ` |
| `tests/unit/solver_test.cpp` | `wjs::solve` | Adjacency, the minimum length, the path reported for a word, a tile used at most once, a word reached by several paths reported once, the `Qu` tile, the scoring scheme applied, an empty dictionary |
| `tests/unit/paths_test.cpp` | `wjs::platform::executable_path` | That it names the running test program: absolute, an existing file, called `wjs_unit_tests` |
| `tests/app/solver_model_test.cpp` | The window's `SolverModel` | Solving once a dictionary is loaded, a dictionary that cannot be opened, a board that cannot be read. Built only with `WJS_BUILD_APP` |
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

## The two build checks

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
- **The console and path code on Linux and macOS.** Written, never built.
- **A UTF-8 console on Windows**, and files opened by paths longer than
  `MAX_PATH` (a [known problem](building.md#platform-notes)).
- **Performance.** There are no timing or throughput tests, and no
  measurements of the solver on large boards or large word lists have been made.
- **Real word lists.** The tests use made-up lists of a dozen words. The
  behaviour on a list of a few hundred thousand words has not been checked by
  a test.
- **Threads.** The library keeps no shared state between calls, but nothing
  tests solving from several threads at once.
