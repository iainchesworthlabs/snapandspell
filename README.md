# WordJewelsSolver

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)](docs/architecture.md)
[![License: GPL v3+](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)](LICENSE)

Finds every word on a Boggle board.

The goal is to hold a real Boggle board up to a webcam, or a phone or tablet
camera, and have it solve the board, as a learning aid for young children (five
and six year olds) playing with an adult. That goal is not met yet: today you
type the board in and it finds the words. The camera, and reading letters from
a picture, are still to be built.

> **Status: pre-release, nothing released yet.** The solver is real and
> tested. The camera, letter recognition and installers do not exist yet. CI
> builds and tests it on Windows (MSVC), Linux (GCC) and macOS (AppleClang).
> [CHANGELOG.md](CHANGELOG.md) says what is in and what is not proven, and
> [docs/roadmap.md](docs/roadmap.md) says what comes next.

## What there is today

| Part | Where | State |
| --- | --- | --- |
| The library: board parsing, a trie dictionary, scoring, a depth-first solver. No GUI, no operating system code | `src/lib` | Unit tested |
| `wjs-cli`, a command-line front end | `src/cli` | Works; not covered by a test of its own |
| Per-OS code (finding the executable, a UTF-8 console), and nothing else per-OS | `src/platforms` | Built and tested by CI on Windows, Linux and macOS |
| The Solo Play screens in Qt Quick: home, the box, play (the board and the panel) and the resting screen, over a fake of the game until the library's session is wired | `src/app` | Opt-in (`-DWJS_BUILD_APP=ON`, needs Qt 6.12); model, layout, touch and fake tests and a lint of the QML, run on a Windows desktop only; not packaged |
| Catch2 unit tests, a platform-contract test and the platform-isolation check | `tests`, `tools/checks` | Pass on Windows |

Not built yet: camera input, recognising letters in an image, installers and
packages, and a phone, tablet or browser target. The English word list is ENABLE,
which is in `data/dictionaries/`: see [data/README.md](data/README.md).

## Build and run the command-line solver

You need CMake 4.0 or newer, Ninja, Git, and a C++23 compiler. MSVC from
Visual Studio 2022 17.x or Visual Studio 2026, GCC 14 or newer and AppleClang
16 or newer are the intent; MSVC 19.51 (Visual Studio 2026) is the only one
verified so far. PowerShell 7 (`pwsh`) is needed only for the platform-isolation
check, which is skipped by the tests if it is not installed.

```sh
git clone --recurse-submodules https://github.com/iainchesworthlabs/wordjewelssolver
cd wordjewelssolver

cmake --preset windows-msvc-debug          # or linux-gcc-debug, macos-clang-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

The presets are `windows-msvc-{debug,release}`, `linux-gcc-{debug,release,coverage}`
and `macos-clang-{debug,release}`; each builds into `build/<preset>/` and puts
its programs in `build/<preset>/bin/`. The first configure bootstraps the
vendored vcpkg and builds Catch2, which needs network access. On Windows, run
the commands from a shell where the MSVC toolchain is on the path (a Developer
PowerShell for Visual Studio). [docs/building.md](docs/building.md) has the
details, the options, and a known problem with very long paths on Windows.

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
  app/            the Qt Quick window (built only with WJS_BUILD_APP)
  platforms/      everything that differs between operating systems
    interfaces/     one header per OS service (wjs/platform/*.hpp)
    windows/ linux/ macos/ posix/
                    implementations, and each platform's platform.cmake
cmake/            Platform.cmake (the one place the OS is chosen), warnings,
                  sanitizers, coverage, Qt discovery
tests/            unit/ (library), app/ (the window's model), platform/
tools/
  checks/         check_platform_isolation.ps1
  hooks/          git hooks (commit-msg, pre-push); see CONTRIBUTING.md
  policy/         check_text_policy.ps1, the rule for commit and PR text
.github/          CI workflows
deps/vcpkg        vcpkg, as a git submodule
docs/             this documentation (mkdocs-material)
```

The operating system is a build-system decision, never a source-code one: no
`#ifdef`, no OS header and no "which OS am I on" outside `src/platforms`. A
check enforces it. See [docs/platform-isolation.md](docs/platform-isolation.md).

## Documentation

| | |
| --- | --- |
| [docs/index.md](docs/index.md) | What it is, and how to use `wjs-cli` |
| [docs/building.md](docs/building.md) | Presets, options, vcpkg, Qt, platform notes |
| [docs/testing.md](docs/testing.md) | The test suites, the checks, coverage, the performance tests |
| [docs/profiling.md](docs/profiling.md) | Profiling with Tracy: building, capturing, reading a trace |
| [docs/architecture.md](docs/architecture.md) | The library, the platform layer, how they fit |
| [docs/platform-isolation.md](docs/platform-isolation.md) | The rule that keeps the OS out of the source, and its check |
| [docs/licensing.md](docs/licensing.md) | The licence, the dependencies, the word-list question |
| [docs/ci.md](docs/ci.md), [docs/branch-protection.md](docs/branch-protection.md) | What CI runs, and how the branches are protected |
| [docs/roadmap.md](docs/roadmap.md) | What is planned |
| [docs/product-requirements.md](docs/product-requirements.md) | The product brief the roadmap is built from |
| [docs/faq.md](docs/faq.md) | Short answers |

The documentation is built with mkdocs-material (`mkdocs.yml`) and will be
published at <https://iainchesworthlabs.github.io/wordjewelssolver/> once the
repository is public.

## Contributing

Contributions are welcome. [CONTRIBUTING.md](CONTRIBUTING.md) has the branching
model (gitflow: `develop` for day-to-day work, `main` for releases), the rule
about platform code, and how pull requests are reviewed. Everyone taking part
is held to the [code of conduct](CODE_OF_CONDUCT.md); a security problem is
reported privately, as the [security policy](SECURITY.md) says.

## Licence

GPL-3.0-or-later. See [LICENSE](LICENSE) and
[docs/licensing.md](docs/licensing.md), which also says what is not covered:
the English word list is ENABLE, a separate work that its originators released
into the public domain (provenance in [data/README.md](data/README.md)).

WordJewelsSolver is a project of Iain Chesworth Labs.
