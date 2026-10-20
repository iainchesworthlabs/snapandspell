# WordJewelsSolver

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)](docs/architecture.md)
[![License: GPL v3+](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)](LICENSE)

Finds every word on a Boggle board.

The goal is to hold a real Boggle board up to a webcam, or a phone or tablet
camera, and have it solve the board, as a learning aid for young children (five
and six year olds) playing with an adult. That goal is not met yet: today you
type the board in and it finds the words. A reader that finds a board in a
camera frame and reads its letters now exists (`src/scan`), tested on drawn
frames only: no camera is connected to it and it has not seen a real board.

> **Status: pre-release, nothing released yet.** The solver, the game and the
> window are real and tested. The camera backends do not exist yet, and letter
> recognition is built and tested on synthetic frames only. A Windows build now
> also installs and packages: CI checks a ZIP's contents, runs the packaged
> solver and the packaged window headless, and uploads it as an artifact. An
> NSIS installer is configured for a machine that has NSIS; the Linux and macOS
> packages are configured but not yet built. CI builds and tests it on Windows
> (MSVC), Linux (GCC) and macOS (AppleClang), and the Windows leg builds the
> application too. [CHANGELOG.md](CHANGELOG.md) says
> what is in and what is not proven, [docs/audit-0.1.md](docs/audit-0.1.md) orders
> the whole codebase against the requirements, and
> [docs/roadmap.md](docs/roadmap.md) says what comes next.

## What there is today

| Part | Where | State |
| --- | --- | --- |
| The library: board parsing, a trie dictionary, scoring, a depth-first solver, dice and deals, the game session, the store, the parent gate and dashboard. No GUI, no operating system code | `src/lib` | Unit tested |
| `wjs-cli`, a command-line front end | `src/cli` | Works; not covered by a test of its own |
| Per-OS code (finding the executable, a UTF-8 console), and nothing else per-OS | `src/platforms` | Built and tested by CI on Windows, Linux and macOS |
| **The Solo Play game in Qt Quick**: home, the box, play (the board and the panel), the resting screen and the parent area (gate, dashboard, settings, about), playing the real standard board — the project's dice at 3x3 to 6x6 against the full dictionary — with the library's session and store; the phonics board behind a phase directory; the interface in eight languages (English, French, Spanish, German, Yiddish, Hebrew, Chinese and Japanese) with RTL for Hebrew and Yiddish; the feedback engines (Qt Multimedia sound, the device's own text-to-speech, Qt Sensors motion and a haptics platform interface) behind the parent's Sound, Celebration sounds, Sound it out, Vibration and Shake rows | `src/app`, `src/lib` | Built when a prebuilt Qt 6.12 is found (`WJS_BUILD_APP=AUTO`; `ON` requires it, `OFF` skips it); model, touch, session, parent, settings, feedback and motion tests, the language and catalogue checks, a lint of the QML and the scene contract; built on the Windows leg of CI; installed and packaged with CPack, with its Qt runtime and the third-party notices. The three feedback modules are optional: a kit without them builds their silent variants |
| A scanning library: finds a board in a camera frame, reads each die's letter, and a confirm-and-correct model so the solver sees only a confirmed board; the `camera` and `board_reader` interfaces | `src/scan`, `src/platforms` | Unit tested and held to an accuracy target on **synthetic frames only**; no camera backend; never tried on a real board ([design](docs/design/scanner.md)) |
| Catch2 unit tests, the app tests, a platform-contract test and the platform-isolation check | `tests`, `tools/checks` | Pass on Windows |

The sound cues are placeholder WAVs owned by the project
(`assets/sounds`, written by `tools/sounds/make_sounds.ps1`); the phoneme clips
and recorded cheers are the reviewer's recording job, and the sound-it-out
sequence is wired but silent. Haptics is a no-op on the desktops and its
Android/iOS backends wait for those targets. Nothing captures audio.

Not built yet: a camera (the interface exists, with no backend), recognising
letters in a real picture (the reader is tested on drawn frames only), a phone,
tablet or browser target, the tablet's sensor and haptic backends, and a release
that publishes a package. The English word list is ENABLE, which is in
`data/dictionaries/`: see [data/README.md](data/README.md).

## Build and run the command-line solver

You need CMake 4.0 or newer, Ninja, Git, and a C++23 compiler. MSVC from
Visual Studio 2022 17.x or Visual Studio 2026, GCC 14 or newer and AppleClang
16 or newer are the intent; MSVC 19.51 (Visual Studio 2026) is the only one
verified so far. PowerShell 7 (`pwsh`) is needed for the repository checks
(platform isolation, the word lists and dice, the phonics data and the feedback
sounds); those are skipped by the tests if it is not installed.

```sh
git clone https://github.com/iainchesworthlabs/wordjewelssolver
cd wordjewelssolver

# needs VCPKG_ROOT set to a clone of the labs' vcpkg fork: docs/building.md
cmake --preset windows-msvc-debug          # or linux-gcc-debug, macos-clang-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

The window is built too when a prebuilt Qt 6.12 kit is found: `WJS_BUILD_APP` defaults to `AUTO`, and the labs' shared kit at `C:\Repos\common\qt` is searched automatically (set `QT_ROOT_DIR` or `CMAKE_PREFIX_PATH` for a kit somewhere else). Pass `-DWJS_BUILD_APP=OFF` for a solver-only build, `ON` to require Qt, and `-DWJS_BUILD_SHAKE_PROTOTYPE=ON` for the scene harness (see [docs/building.md](docs/building.md#the-qt-quick-window)).

The presets are `windows-msvc-{debug,release}`,
`windows-clang-coverage`, `linux-gcc-{debug,release,coverage}` and
`macos-clang-{debug,release}`; each builds into `build/<preset>/` and puts
its programs in `build/<preset>/bin/`. The two coverage presets hold the unit
tier to 90% line and 80% branch coverage (`coverage-unit`); see
[docs/testing.md](docs/testing.md#coverage). To install and package a build, add
`cpack --preset package-<os>-<compiler>-release` after the build: the ZIP (and
the platform's own installer where its tool is installed), with the Qt runtime,
the word list, the dice and the third-party notices, lands in
`build/<preset>/packages/`. `tools/package/check_package.ps1` checks a package
and runs the packaged programs. The first configure bootstraps the
vendored vcpkg and builds Catch2, which needs network access; packaging needs
none. On Windows, run the commands from a shell where the MSVC toolchain is on
the path (a Developer PowerShell for Visual Studio).
[docs/building.md](docs/building.md) has the details, the options, the packaging
commands and a known problem with very long paths on Windows.

### A worked example

A build puts the full English dictionary beside the program, so without `-d` that
is the word list. To follow this example exactly, use a small list of your own
instead: it holds the words the test suite checks, plus three that cannot be
found on the board below. Put it in `words.txt`, one word per line:

```text
cat
car
care
are
tar
tea
dog
cod
rod
cats
rest
at
```

The board is three rows of three tiles, `CAT`, `ORE` and `DGS`:

```text
C A T
O R E
D G S
```

Run the solver, giving the word list with `-d` and each row of the board as an
argument:

```sh
build/linux-gcc-debug/bin/wjs-cli -d words.txt CAT ORE DGS
```

```powershell
build\windows-msvc-debug\bin\wjs-cli.exe -d words.txt CAT ORE DGS
```

```text
CARE                1   r1c1 r1c2 r2c2 r2c3
ARE                 1   r1c2 r2c2 r2c3
CAR                 1   r1c1 r1c2 r2c2
CAT                 1   r1c1 r1c2 r1c3
COD                 1   r1c1 r2c1 r3c1
DOG                 1   r3c1 r2c1 r3c2
ROD                 1   r2c2 r2c1 r3c1
TAR                 1   r1c3 r1c2 r2c2
TEA                 1   r1c3 r2c3 r1c2
9 words, 9 points
```

Each line is a word, its score, and the tiles that spell it (`r2c3` is row 2,
column 3). The best words come first: highest score, then longest, then
alphabetical. `CATS` and `REST` are in the list but are not found, because the
S is not next to the T; `AT` is shorter than the default minimum of three
letters. A tile may be used once per word, and tiles touch horizontally,
vertically and diagonally.

A tile can hold more than one letter, as the "Qu" face of a Boggle die does.
Separate the tiles with spaces or commas and quote the row: `wjs-cli -d words.txt "Qu I T" "A B C" "D E F"`.
The options (`-m` for the shortest word, `-s rarity` for the other scoring
scheme), the exit codes and the board format are in
[docs/index.md](docs/index.md#using-the-command-line-solver).

## Layout

```text
src/
  lib/            wjs:: the game logic: board, dictionary, scoring, solver
  cli/            wjs-cli
  app/            the Qt Quick window (WordJewelsSolver; WJS_BUILD_APP=AUTO)
  platforms/      everything that differs between operating systems
    interfaces/     one header per OS service (wjs/platform/*.hpp)
    windows/ linux/ macos/ posix/
                    implementations, and each platform's platform.cmake
cmake/            Platform.cmake (the one place the OS is chosen), warnings,
                  sanitizers, coverage, Qt discovery, installation and CPack
                  (Packaging.cmake), the third-party notices (Notices.cmake)
notices/
  fragments/      one plain-text section each of NOTICES.txt
  licences/       the licence texts those sections reproduce
tests/            unit/ (library), app/ (the window's model), platform/,
                  notices/ and package/ (the packaging checks)
tools/
  checks/         check_platform_isolation.ps1
  hooks/          git hooks (commit-msg, pre-push); see CONTRIBUTING.md
  package/        check_package.ps1, the packaged programs' check
  policy/         check_text_policy.ps1, the rule for commit and PR text
.github/          CI workflows
docs/             this documentation (mkdocs-material)
```

The operating system is a build-system decision, never a source-code one: no
`#ifdef`, no OS header and no "which OS am I on" outside `src/platforms`. A
check enforces it. See [docs/platform-isolation.md](docs/platform-isolation.md).

## Documentation

| | |
| --- | --- |
| [docs/index.md](docs/index.md) | What it is, and how to use `wjs-cli` |
| [docs/building.md](docs/building.md) | Presets, options, vcpkg, Qt, packaging, platform notes |
| [docs/testing.md](docs/testing.md) | The test suites, the checks, coverage, the performance tests |
| [docs/profiling.md](docs/profiling.md) | Profiling with Tracy: building, capturing, reading a trace |
| [docs/architecture.md](docs/architecture.md) | The library, the platform layer, how they fit |
| [docs/platform-isolation.md](docs/platform-isolation.md) | The rule that keeps the OS out of the source, and its check |
| [docs/licensing.md](docs/licensing.md) | The licence, the dependencies, the word-list question |
| [docs/ci.md](docs/ci.md), [docs/branch-protection.md](docs/branch-protection.md) | What CI runs, and how the branches are protected |
| [docs/roadmap.md](docs/roadmap.md) | What is planned |
| [docs/audit-0.1.md](docs/audit-0.1.md) | The codebase ordered against the requirements, and the work plan |
| [docs/product-requirements.md](docs/product-requirements.md) | The product brief the roadmap is built from |
| [docs/design/ux-audit-2026-10/index.md](docs/design/ux-audit-2026-10/index.md) | The human-factors audit and design overhaul direction (October 2026); [visual brief](docs/design/ux-audit-2026-10/visual-brief.html) and [PDF](docs/design/ux-audit-2026-10/visual-brief.pdf); the [follow-up pass](docs/design/ux-audit-2026-10/follow-up-2026-10-19.md) records the screen-by-screen corrections, points, collection and help rules |
| [docs/faq.md](docs/faq.md) | Short answers |

The documentation is built with mkdocs-material (`mkdocs.yml`) and will be
published at <https://iainchesworthlabs.github.io/wordjewelssolver/> once the
repository is public.

## Verifying a release

Release files are signed with a GPG key whose fingerprint is
`9A0D96E8944899F94B1B37B51F1A8DA4C79B19D2` (expires 2028-10-19), and carry a `SHA256SUMS` file, a software bill of
materials and build-provenance attestations. How to check a download:
[SECURITY.md](SECURITY.md#verifying-a-release) and
[docs/ci.md](docs/ci.md#checking-a-download).

## Contributing

Contributions are welcome. [CONTRIBUTING.md](CONTRIBUTING.md) has the branching
model (gitflow: `develop` for day-to-day work, `main` for releases), the rule
about platform code, and how pull requests are reviewed. Everyone taking part
is held to the [code of conduct](CODE_OF_CONDUCT.md); a security problem is
reported privately, as the [security policy](SECURITY.md) says.

## Licence

GPL-3.0-or-later. A build of the window is GPL-3.0 only, because it links Qt
Quick 3D; each package states that plainly in its `NOTICES.txt`, which the
About screen also shows. See [LICENSE](LICENSE) and
[docs/licensing.md](docs/licensing.md), which also says what is not covered:
the English word list is ENABLE, a separate work that its originators released
into the public domain (provenance in [data/README.md](data/README.md)).

WordJewelsSolver is a project of Iain Chesworth Labs.

