# Building

This page builds WordJewelsSolver from source. To use `wjs-cli`, see the
[home page](index.md#using-the-command-line-solver).

CI builds and tests all three desktops: Windows (MSVC 19.51), Linux (GCC 15.2,
Ubuntu 26.04) and macOS (Apple clang 21, Apple silicon), each in Debug with
warnings as errors and sanitizers. The compiler minimums below are the intent;
only the versions just named have been tried. What CI runs is in [CI](ci.md).

## Requirements

| What | Version | Notes |
| --- | --- | --- |
| CMake | 4.0 or newer | The presets require it. 4.4.0 is what the verified build used |
| Ninja | any recent | The presets use the Ninja generator |
| A C++23 compiler | MSVC from Visual Studio 2022 17.x or Visual Studio 2026; GCC 14 or newer; AppleClang 16 or newer | Verified: MSVC 19.51 (Visual Studio 2026), GCC 15.2, Apple clang 21. The code uses `std::expected` and `std::format` |
| Git | any recent | Clones this repository and checks the vcpkg commit |
| vcpkg | the labs' fork, at or after the commit in `cmake/vcpkg-commit.txt` | One shared clone, found through `VCPKG_ROOT`, see [below](#vcpkg) |
| PowerShell 7 (`pwsh`) | 7.0 or newer | For the [platform-isolation check](platform-isolation.md#enforcement). Optional: without it the `platform.isolation` test is not registered |
| Network access | | The first configure bootstraps vcpkg and builds Catch2 |
| Qt | 6.12 | Only for the optional Qt Quick window, see [below](#the-qt-quick-window) |

## Clone

```sh
git clone https://github.com/iainchesworthlabs/wordjewelssolver
cd wordjewelssolver
```

## vcpkg

The dependency provider is vcpkg, from the labs' fork. It is **not** part of
this repository: one clone serves every project on the machine, and the
presets find it through the `VCPKG_ROOT` environment variable. Clone it once,
anywhere, and set the variable:

```sh
git clone https://github.com/iainchesworthlabs/vcpkg C:\Repos\vcpkg
setx VCPKG_ROOT C:\Repos\vcpkg      # Windows; open a new shell (or restart Visual Studio) after
export VCPKG_ROOT=~/vcpkg           # Linux and macOS, in your shell profile
```

`cmake/vcpkg-commit.txt` holds the vcpkg commit this project was verified with.
The configure checks that your clone contains that commit (it may be ahead of
it) and stops with the fix if it does not, for example `git -C %VCPKG_ROOT%
pull`. CI clones the fork at exactly that commit
([docs/ci.md](ci.md#the-vcpkg-binary-cache)).

## First build

Run these from a shell where the compiler is available: on Windows a Developer
PowerShell for Visual Studio (the Ninja generator does not find MSVC by itself).

```sh
# Windows
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug

# Linux
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug

# macOS
cmake --preset macos-clang-debug
cmake --build --preset macos-clang-debug
ctest --preset macos-clang-debug
```

The programs are in `build/<preset>/bin/`: `wjs-cli` (`wjs-cli.exe` on Windows),
`wjs_unit_tests` and `wjs_perf_tests` ([Testing](testing.md#performance-tests)). `ctest` fails if it finds no tests at all.

## Presets

A preset configures into `build/<preset>/` and is named
`<os>-<compiler>-<config>`. Each is available only on its own host, so
`cmake --list-presets` lists the ones for the machine you are on.

| Preset | Host | vcpkg triplet | Notes |
| --- | --- | --- | --- |
| `windows-msvc-debug` | Windows | `x64-windows` | Debug, with AddressSanitizer |
| `windows-msvc-release` | Windows | `x64-windows` | Release |
| `linux-gcc-debug` | Linux | `x64-linux` | Debug, with AddressSanitizer and UBSan; uses `g++` |
| `linux-gcc-release` | Linux | `x64-linux` | Release |
| `linux-gcc-coverage` | Linux | `x64-linux` | Debug with gcov instrumentation and the sanitizers off |
| `macos-clang-debug` | macOS | `arm64-osx` | Debug, with AddressSanitizer and UBSan |
| `macos-clang-release` | macOS | `arm64-osx` | Release |
| `windows-msvc-profiling` | Windows | `x64-windows` | RelWithDebInfo with Tracy, sanitizers off. A development build, never shipped: see [Profiling](profiling.md) |
| `linux-gcc-profiling` | Linux | `x64-linux` | As above, with `g++` |
| `macos-clang-profiling` | macOS | `arm64-osx` | As above |

The macOS presets use the Apple Silicon triplet. On an Intel Mac, pass
`-DVCPKG_TARGET_TRIPLET=x64-osx` (untried). The Linux presets name the compiler
`g++`; if yours is older than GCC 14, pass `-DCMAKE_CXX_COMPILER=g++-14`, or
whichever newer one you have.

For a combination the presets do not cover, make your own in
`CMakeUserPresets.json`, which is git-ignored.

## Options

Pass these to the configure step, for example
`cmake --preset linux-gcc-debug -DWJS_BUILD_TESTS=OFF`.

| Option | Default | Meaning |
| --- | --- | --- |
| `WJS_BUILD_CLI` | `ON` | Build `wjs-cli` |
| `WJS_BUILD_APP` | `OFF` | Build the Qt Quick window. Needs Qt 6.12 |
| `WJS_BUILD_TESTS` | `ON` | Build the test suites, and register the platform tests. Turning it off also stops vcpkg building Catch2 |
| `WJS_BUILD_SHAKE_PROTOTYPE` | `OFF` | Build the 3D board-scene prototype as `ShakePrototype`. Needs `WJS_BUILD_APP=ON` and Qt Quick 3D; the program it makes is GPL-3.0 only. See [the prototype](design/shake-prototype.md#how-it-was-measured) |
| `WJS_ENABLE_SANITIZERS` | `ON` | AddressSanitizer (and UBSan, where the compiler has it) in Debug builds. See [Sanitizers](#sanitizers-and-warnings) |
| `WJS_ENABLE_COVERAGE` | `OFF` | gcov instrumentation. Only the `linux-gcc-coverage` preset turns it on |
| `WJS_REQUIRE_REVIEWED_PHONICS` | `ON` for the release build types, else `OFF` | The review gate: the build runs `tools/data/check_phonics.ps1 -RequireReviewed` and stops while a file in `data/phonics` has no reviewed-by and reviewed-on header. Needs PowerShell 7. The release presets set it. See [Testing](testing.md#phonics-data-the-checker-and-the-review-gate) |
| `WJS_ENABLE_TRACY` | `OFF` | The Tracy profiler: makes the calls in `wjs/profiling.hpp` real, and installs the client and its tools through the vcpkg `profiling` feature. Only the `*-profiling` presets turn it on, and nothing that ships may ([Profiling](profiling.md)) |

C++23 is required, with compiler extensions off, for every target.

## The Qt Quick window

The window is the Solo Play game of [Application design](design/application.md)
in Qt Quick, over the library (`src/app`; [Architecture](architecture.md#the-qt-quick-window)). It is the
optional target `wjs_app`, built as `WordJewelsSolver` (`WordJewelsSolver.exe`
on Windows, where it is a GUI-subsystem program with no console window; an
application bundle on macOS). It is off by default until CI installs Qt.

It needs Qt 6.12 with the modules Core, Gui, Qml, Quick and Quick Controls 2,
as a prebuilt install (from the Qt online installer, or with
[aqtinstall](https://github.com/miurahr/aqtinstall)). Qt is not built by vcpkg.
`cmake/FindQt6.cmake` looks for it, in this order:

1. `-DCMAKE_PREFIX_PATH=<path to the kit>` or `-DQt6_DIR=<...>`, which always win;
2. the `QT_ROOT_DIR` environment variable, which `install-qt-action` and similar
   tools export;
3. the usual install locations for the platform: `C:/Qt` and `~/Qt` on
   Windows, `~/Qt` and `/opt/Qt` and the system `qt6` directories on Linux,
   `~/Qt` and Homebrew's `qt6` on macOS.

```sh
cmake --preset windows-msvc-debug -DWJS_BUILD_APP=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64
cmake --build --preset windows-msvc-debug
build/windows-msvc-debug/bin/WordJewelsSolver.exe
```

On Windows the build runs `windeployqt` after linking, so Qt's DLLs and the QML
modules the window imports are copied beside the executable and it starts from
anywhere. On Linux nothing is deployed: Qt is found at run time the same way it
was found at build time. On macOS the application bundle is not yet given Qt's
runtime (that comes with packaging).

With the window enabled, `ctest` also runs `wjs_app_tests`, the Catch2 tests of
the model the window talks to. They need Qt's runtime beside them, which the
build arranges the same way.

The window plays a stand-in game until it is told where a phase is: the
repository has none, because the phase files have to be written and reviewed by a
person qualified to do it. To play the library's session, keep a directory of
`phase-<x>.graphemes.txt` and `phase-<x>.words.txt` files **outside the
repository** and start the program with it
([the options](architecture.md#the-qt-quick-window)):

```sh
build/windows-msvc-release/bin/WordJewelsSolver.exe --phase-directory <dir> --store <file> --seed 7
```

Give `--store` a file of its own when the phase is test data, or what is found
goes into the store of your own profile. `--store` applies with no phase too: it is
the file the parent area keeps its settings in, and the stand-in game is made from them.

## Dependencies

The vcpkg manifest (`vcpkg.json`) is pinned to a `builtin-baseline`, and the
presets point `CMAKE_TOOLCHAIN_FILE` at the vcpkg toolchain under `VCPKG_ROOT`, so
dependencies are resolved at configure time. Today it has no runtime
dependencies, and one optional feature:

| Feature | Packages | When |
| --- | --- | --- |
| `tests` | `catch2` (BSL-1.0) | Whenever `WJS_BUILD_TESTS` is `ON`; CMake selects it before `project()` |

Licences of everything involved are in [Licensing](licensing.md).

## Sanitizers and warnings

Every first-party target is compiled with warnings as errors: `/W4 /WX
/permissive-` on MSVC, and `-Wall -Wextra -Wpedantic -Werror` with a longer list
(shadowing, old-style casts, conversions, null dereference, and more) on GCC and
Clang. A warning breaks the build, so fix it rather than silence it. The flags
are in `cmake/CompilerWarnings.cmake`; third-party code is not held to them.

Debug builds also turn on sanitizers (`cmake/Sanitizers.cmake`):

- GCC and Clang: AddressSanitizer and UndefinedBehaviorSanitizer, with
  `-fno-sanitize-recover=all` (the first finding aborts), plus the standard
  library's assertions (`_GLIBCXX_ASSERTIONS`).
- MSVC: AddressSanitizer only (it has no UBSan). `/RTC` and incremental linking
  are turned off because they cannot be used with it. The ASan runtime DLL
  (`clang_rt.asan_dynamic-*.dll`) is copied beside each executable at build
  time, so the programs start outside a Developer prompt too.
- clang-cl: disabled.

Release builds are not instrumented. `-DWJS_ENABLE_SANITIZERS=OFF` turns it all
off for a Debug build.

## Coverage

`linux-gcc-coverage` is Debug with `WJS_ENABLE_COVERAGE=ON` and the sanitizers
off: it compiles the library, the platform layer and the tests with
`--coverage -fprofile-update=atomic -fno-inline`, so running the tests writes
gcov data (`.gcda` files beside the objects). Turning that into a report is
done with a tool such as gcovr; how CI does it, and what it requires, is in
[CI](ci.md). See also [Testing](testing.md#coverage).

## Platform notes

**Windows.** The word list and any file the programs open must be reachable by
a path of at most 260 characters (`MAX_PATH`). Opening a file by a longer path
fails unless the executable is long-path aware; this was found during testing
and is not fixed yet. Keep word lists at a short path.

**Linux.** `wjs-cli` finds its default word list through `/proc/self/exe`, so it
needs `/proc` mounted, as on any normal system.

**macOS.** The executable's own path comes from `_NSGetExecutablePath`, with
symlinks resolved. Neither this nor the Linux code has been built or run yet.

There is no install or package step yet: the programs run from `build/<preset>/bin/`.

## Troubleshooting

- **"Could not find toolchain file: /scripts/buildsystems/vcpkg.cmake"** or
  **"The vcpkg toolchain file was not found"**: `VCPKG_ROOT` is not set in the
  shell (or Visual Studio) that ran the configure, or does not point at a vcpkg
  clone. See [vcpkg](#vcpkg); a program started before `setx` needs restarting.
- **"does not contain the pinned commit"**: your vcpkg clone is older than
  `cmake/vcpkg-commit.txt`. `git -C "$VCPKG_ROOT" pull` (or `fetch origin` if it
  says the commit is missing from the history).
- **`wjs-cli: ...dictionaries/en.txt: the dictionary file could not be opened`**:
  there is no `dictionaries/en.txt` beside the executable. A build copies it
  there from `data/dictionaries/en.txt`; if you moved the executable, move the
  `dictionaries` folder with it, or pass a word list with `-d <file>`.
- **Qt is not found**: pass `-DCMAKE_PREFIX_PATH=<kit directory>` (the directory
  that holds `lib/cmake/Qt6`), or set `QT_ROOT_DIR`.
- **A program fails to start with `STATUS_DLL_NOT_FOUND`**: on Windows, a Debug
  build needs the ASan runtime beside it (the build copies it) and the Qt window
  needs Qt's DLLs (the build runs `windeployqt`). If you moved the executable
  out of `bin/`, move those with it.
