# WordJewelsSolver

[![CI](https://github.com/iainchesworthlabs/wordjewelssolver/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/iainchesworthlabs/wordjewelssolver/actions/workflows/ci.yml)
[![Coverage: statements](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Fiainchesworthlabs%2Fwordjewelssolver%2Fquality-history%2Fcoverage-line.json)](docs/ci.md#quality-history)
[![Coverage: branches](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Fiainchesworthlabs%2Fwordjewelssolver%2Fquality-history%2Fcoverage-branch.json)](docs/ci.md#quality-history)
[![CodeQL](https://github.com/iainchesworthlabs/wordjewelssolver/actions/workflows/codeql.yml/badge.svg)](https://github.com/iainchesworthlabs/wordjewelssolver/actions/workflows/codeql.yml)
[![zizmor](https://github.com/iainchesworthlabs/wordjewelssolver/actions/workflows/zizmor.yml/badge.svg)](https://github.com/iainchesworthlabs/wordjewelssolver/actions/workflows/zizmor.yml)
[![Scorecard](https://api.securityscorecards.dev/projects/github.com/iainchesworthlabs/wordjewelssolver/badge)](https://securityscorecards.dev/viewer/?uri=github.com/iainchesworthlabs/wordjewelssolver)
[![Docs](https://github.com/iainchesworthlabs/wordjewelssolver/actions/workflows/docs.yml/badge.svg)](https://github.com/iainchesworthlabs/wordjewelssolver/actions/workflows/docs.yml)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)](docs/architecture.md)
[![License: GPL v3+](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)](LICENSE)
![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux%20%7C%20macOS%20%7C%20iOS-lightgrey)

Finds every word on a Boggle board.

It is also a game and a learning aid for young children (five and six year
olds): **Solo Play** is a Boggle-like game where the child shakes a box of dice
and finds words, and **Mode A** reads a real board from a camera (or a typed
board) and helps the child find its words while an adult plays. The reader that
finds a board in a camera frame and reads its letters (`libs/scan`) is tested on
**drawn frames only**: the camera backend is written (Qt Multimedia, on the
desktops) but it has never read a real board.

> **Status: pre-release, 0.1.0 not yet released.** The solver, the Solo Play game
> and the parent area are real and tested; they run as a Qt Quick desktop
> application, and CI builds and tests the library and the command-line solver
> on Windows (MSVC), Linux (GCC) and macOS (AppleClang), and the application on
> Windows. Release builds are packaged by `release.yml` for the three desktops;
> an ad-hoc iOS build for the author's own devices is written and has **never run
> on a device**; an Android build is being worked on separately and does not exist
> yet. The release pipeline (signing, SBOM, attestations) has not yet run on
> GitHub. [CHANGELOG.md](CHANGELOG.md) says what is in and what is not proven,
> [docs/requirements.md](docs/requirements.md) lists each requirement with the
> test that shows it and the gaps that remain, and
> [docs/roadmap.md](docs/roadmap.md) says what comes next.

A board in play, in Solo Play at its smallest window (more in
[the game design](docs/design/game.md#what-it-looks-like)):

![Solo Play: a 3x3 board with CAT found, the words tray and the actions](docs/design/img/solo-playing.png)

## What there is today

| Part | Where | State |
| --- | --- | --- |
| The library: board parsing, a trie dictionary, scoring, a depth-first solver, dice and deals, the game session, the store, the parent gate and dashboard. No GUI, no operating system code | `libs/core` | Unit tested |
| `wjs-cli`, a command-line front end | `apps/cli` | Works; not covered by a test of its own |
| Per-OS code (finding the executable, a UTF-8 console), and nothing else per-OS | `libs/platforms` | Built and tested by CI on Windows, Linux and macOS |
| **The Solo Play game in Qt Quick**: home, the box, play (the board and the panel), the end-of-board summary, My jewels and the parent area (gate, dashboard, settings, about), playing the real standard board — the project's dice at 3x3 to 6x6 against the full dictionary — with the library's session and store; the phonics board behind a phase directory; the interface in eight languages (English, French, Spanish, German, Yiddish, Hebrew, Chinese and Japanese) with RTL for Hebrew and Yiddish; the feedback engines (Qt Multimedia sound, the device's own text-to-speech, Qt Sensors motion and a haptics platform interface) behind the parent's Sound, Celebration sounds, Sound it out, Vibration and Shake rows | `apps/app/src`, `libs/core` | Built when a prebuilt Qt 6.12 is found (`WJS_BUILD_APP=AUTO`; `ON` requires it, `OFF` skips it); model, touch, session, parent, settings, feedback and motion tests, the language and catalogue checks, a lint of the QML and the scene contract; built on the Windows leg of CI (the Linux and macOS legs build the library and the command-line solver; the release workflow builds the application on those two); installed and packaged with CPack, with its Qt runtime and the third-party notices. The three feedback modules are optional: a kit without them builds their silent variants |
| A scanning library: finds a board in a camera frame, reads each die's letter, and a confirm-and-correct model so the solver sees only a confirmed board; the `camera` and `board_reader` interfaces, and a desktop camera over Qt Multimedia | `libs/scan`, `apps/app/src/scan`, `libs/platforms` | Unit tested and held to an accuracy target on **synthetic frames only**; the camera backend is never constructed in a test and has not read a real frame; never tried on a real board ([design](docs/design/scanner.md)) |
| Catch2 unit, scanner, app and performance tests, Quick Test suites for the QML, and the repository checks (platform isolation, privacy, packaging, translations, toolchain pins) | `tests`, `tools/checks` | Pass in CI on Windows, Linux and macOS (what each leg runs: [docs/ci.md](docs/ci.md)) |
| An ad-hoc iOS build for the author's own devices | `libs/platforms/ios`, `.github/workflows/release.yml` | Written; the signing and the install have **never run** ([docs/ios-install.md](docs/ios-install.md)) |

The sound cues are placeholder WAVs owned by the project
(`assets/sounds`, written by `tools/sounds/make_sounds.ps1`); the phoneme clips
and recorded cheers are the reviewer's recording job, and the sound-it-out
sequence is built but has no sounds to play. Haptics is a no-op on the desktops
and its Android/iOS backends wait for those targets. Nothing captures audio.

Not built or not proven: reading letters from a real picture (the reader is tested
on drawn frames only), a published release, an Android target, a published browser version, phone
layouts, the tablet's sensor and haptic backends, and the phonics content (it needs
a literacy reviewer, so the shipped game plays the standard board). The English
word list is ENABLE, which is in `data/dictionaries/`: see
[data/README.md](data/README.md).

## Build and run the command-line solver

You need CMake (3.28 or newer to configure; CI uses 4.4), Ninja, Git, and a C++23
compiler. CI builds and tests with MSVC 19.51 (Visual Studio 2026), GCC 15.2 and
AppleClang 21, and a nightly job also builds with Clang; older compilers are not
tried. PowerShell 7 (`pwsh`) is needed for the repository checks (platform
isolation, the word lists and dice, the phonics data and the feedback sounds);
those are skipped by the tests if it is not installed.

```sh
git clone https://github.com/iainchesworthlabs/wordjewelssolver
cd wordjewelssolver

# needs VCPKG_ROOT set to a clone of the labs' vcpkg fork (one clone serves every
# project on the machine; it is not vendored): docs/building.md
cmake --preset windows-msvc-debug          # or linux-gcc-debug, macos-clang-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

The window is built too when a prebuilt Qt 6.12 kit is found: `WJS_BUILD_APP` defaults to `AUTO`, and the labs' shared kit at `C:\Repos\common\qt` is searched automatically (set `QT_ROOT_DIR` or `CMAKE_PREFIX_PATH` for a kit somewhere else). Pass `-DWJS_BUILD_APP=OFF` for a solver-only build, `ON` to require Qt, and `-DWJS_BUILD_SHAKE_PROTOTYPE=ON` for the scene harness (see [docs/building.md](docs/building.md#the-qt-quick-window)).

The presets are `windows-msvc-{debug,release,profiling,analysis}`,
`windows-clang-coverage`, `linux-gcc-{debug,release,profiling,coverage,analysis}`,
`linux-clang-debug`, `macos-clang-{debug,release,profiling}` and `ios-release`;
the Debug and profiling presets build with Tracy (a profiler that listens on a
local port), the Release presets without it. Each builds into `build/<preset>/`
and puts its programs in `build/<preset>/bin/`. The two coverage presets hold the unit
tier to 90% line and 80% branch coverage (`coverage-unit`); see
[docs/testing.md](docs/testing.md#coverage). To install and package a build, add
`cpack --preset package-<os>-<compiler>-release` after the build: the ZIP (and
the platform's own installer where its tool is installed), with the Qt runtime,
the word list, the dice and the third-party notices, lands in
`build/<preset>/packages/`. `tools/package/check_package.ps1` checks a package
and runs the packaged programs. The first configure uses the shared vcpkg
clone to build Catch2 (and Tracy, for a Debug preset), which needs network
access; packaging needs none. On Windows, run the commands from a shell where the MSVC toolchain is on
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
libs/
  core/           wjs:: the game logic: board, dictionary, scoring, solver, dice,
                  the game session, the store, the parent gate, feedback policy
  scan/           wjs::scan, the board reader (camera frame to letters)
  platforms/      everything that differs between operating systems
    interfaces/     one header per OS service (wjs/platform/*.hpp)
    windows/ linux/ macos/ ios/ android/ posix/
                    implementations, and each platform's platform.cmake
apps/
  cli/            wjs-cli
  app/            the Qt Quick application (WordJewelsSolver; WJS_BUILD_APP=AUTO):
    qml/            the screens (kit/ is the design system), solo/, scan/, parent/
    src/            the C++ behind them: solo/, scan/, parent/, feedback/, scene/
                    (the 3D board), camera/, variants/
    translations/   the eight interface catalogues (Qt Linguist .ts)
cmake/            Platform.cmake (the one place the OS is chosen), Vcpkg.cmake,
                  warnings, sanitizers, coverage, Tracy, Qt discovery,
                  translations, installation and CPack (Packaging.cmake), the
                  third-party notices (Notices.cmake)
assets/ data/     fonts and sounds; the dictionary, the dice and the word lists
notices/
  fragments/      one plain-text section each of NOTICES.txt
  licences/       the licence texts those sections reproduce
tests/            the repository's own checks: system/, checks/ (the quality
                  tier's registrations) and the topic directories the checks read
                  (platform/, privacy/, package/, notices/, ...)
                  The tests of the code are beside it: libs/core/tests/unit,
                  libs/core/bench (work budgets), libs/scan/tests,
                  apps/app/tests/unit (the application's models and the stub
                  shells), apps/app/tests/prototype, apps/app/ui-tests (Quick Test)
tools/
  checks/         check_platform_isolation.ps1
  coverage/ data/ the coverage gate; the word-list, dice and phonics checks
  hooks/          git hooks (commit-msg, pre-push); see CONTRIBUTING.md
  package/        check_package.ps1, the packaged programs' check
  policy/         check_text_policy.ps1, the rule for commit and PR text
  release/ repo/  signing, SBOM and release checks; the repository rulesets
  lightprobe/ scan/ sounds/
                  the 3D scene's light probe, the scanner's corpus renderer,
                  the feedback sounds
.github/          CI workflows, rulesets and pinned requirements
docs/             this documentation (mkdocs-material)
```

The operating system is a build-system decision, never a source-code one: no
`#ifdef`, no OS header and no "which OS am I on" outside `libs/platforms`. A
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
| [docs/requirements.md](docs/requirements.md) | Each requirement, where it is built, the test that shows it, and what is left |
| [docs/product-requirements.md](docs/product-requirements.md) | The product brief the roadmap is built from |
| [docs/design/index.md](docs/design/index.md) | The game, board, phonics, application, parent-area and scanner design, as built and as still intended |
| [docs/ios-install.md](docs/ios-install.md) | Installing the ad-hoc iOS build on the author's own devices |
| [docs/faq.md](docs/faq.md) | Short answers |

The documentation is built with mkdocs-material (`mkdocs.yml`), strictly, by
`.github/workflows/docs.yml`, and published from `main` at
<https://iainchesworthlabs.github.io/wordjewelssolver/> once the repository's
Pages source is set ([docs/ci.md](docs/ci.md#documentation-site)).

## Verifying a release

Release files are signed with the organisation's GPG key, whose fingerprint is
`FD13CB8CAC8316648CD974D08149254C41894DBE` (expires 2028-10-21), and carry a `SHA512SUMS` file, a software bill of
materials and build-provenance attestations. How to check a download:
[SECURITY.md](SECURITY.md#verifying-a-release).

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

