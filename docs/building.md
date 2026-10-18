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
| PowerShell 7 (`pwsh`) | 7.0 or newer | For the [platform-isolation check](platform-isolation.md#enforcement) and the [packaging check](#checking-a-package). Optional: without it the `platform.isolation` test is not registered |
| Network access | | The first configure bootstraps vcpkg and builds Catch2. Packaging needs none |
| Qt | 6.12 | Only for the optional Qt Quick window, see [below](#the-qt-quick-window) |
| NSIS (`makensis`) | any recent | Optional: adds the Windows installer to [packaging](#packaging). Without it a package is a ZIP |
| `dpkg-deb`, `dpkg-shlibdeps` | any recent | Optional: add the `.deb` to [packaging](#packaging) on Linux (`dpkg-dev`). `rpmbuild` (`rpm`) likewise adds the `.rpm` |

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
`wjs_unit_tests` and `wjs_perf_tests` ([Testing](testing.md#performance-tests)). `ctest` fails if it finds no tests at all. A build can also be installed and
made into a package; [Packaging](#packaging) has the commands and the output.

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
| `WJS_BUILD_APP` | `OFF` | Build the Qt Quick window. Needs Qt 6.12, including Quick 3D; the binary is GPL-3.0 only |
| `WJS_BUILD_TESTS` | `ON` | Build the test suites, and register the platform tests. Turning it off also stops vcpkg building Catch2 |
| `WJS_BUILD_SHAKE_PROTOTYPE` | `OFF` | Build the harness around the 3D board scene as `ShakePrototype`. Needs `WJS_BUILD_APP=ON` and Qt Quick 3D; the harness links the same scene module as the application, so both are GPL-3.0 only. See [the prototype](design/shake-prototype.md#how-it-was-measured) |
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

It needs Qt 6.12 with the modules Core, Gui, Qml, Quick, Quick Controls 2,
Quick 3D and LinguistTools (the `qttools` module: `lupdate` and `lrelease`), as
a prebuilt install (from the Qt online installer, or with
[aqtinstall](https://github.com/miurahr/aqtinstall)). Qt is not built by vcpkg.
The board scene is 3D, so a build of the window links Qt Quick 3D, which Qt
offers only under the GPL-3.0 or commercially: the binary is **GPL-3.0 only**
(the recorded decision, [Board design](design/board.md#rendered-3d-or-drawn-in-2d)).
`cmake/FindQt6.cmake` looks for the kit, in this order:

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
anywhere. An installed or packaged copy is given its Qt runtime by Qt's own
install-time deploy script instead (see [Packaging](#packaging)). On Linux
nothing is deployed: Qt is found at run time the same way it was found at build
time, and a distribution package declares it as a dependency. On macOS the
application bundle is given Qt's runtime at install time; no Mac has built the
window with Qt yet, so that is configured but unverified.

With the window enabled, `ctest` also runs `wjs_app_tests`, the Catch2 tests of
the model the window talks to. They need Qt's runtime beside them, which the
build arranges the same way.

The window plays the real game from the start: the standard board, a roll of the
project's dice (`dice/3x3.txt` to `6x6.txt`, deployed beside the executable) at
3x3 to 6x6 against the English dictionary (`dictionaries/en.txt`), with a game in
progress kept in the store between runs. The phonics board needs phase files, and
the repository has none: the files have to be written and reviewed by a person
qualified to do it. To play it, keep a directory of
`phase-<x>.graphemes.txt` and `phase-<x>.words.txt` files **outside the
repository**, choose the Phonics board in the parent area's settings, and start
the program with the directory
([the options](architecture.md#the-qt-quick-window)):

```sh
build/windows-msvc-release/bin/WordJewelsSolver.exe --phase-directory <dir> --store <file> --seed 7
```

Give `--store` a file of its own when the phase is test data, or what is found
goes into the store of your own profile. `--store` applies with no phase too: it is
the file the parent area keeps its settings in and the game keeps its finds in.

### The interface's languages

The window is offered in English, French, Spanish, German, Yiddish, Hebrew,
Chinese and Japanese, and Hebrew and Yiddish read right to left
([Application design](design/application.md#languages-right-to-left-and-fonts)).
One Qt Linguist catalogue per language is checked in under `i18n/`
(`wjs_en.ts` … `wjs_ja.ts`), and `src/app/language_manager.*` loads the compiled
one from the application's resources as `:/i18n/wjs_<code>.qm`.

`lrelease` runs at build time, as part of building the window: each `.ts` is
compiled into `build/<preset>/src/app/i18n/` and embedded in the `wjs_i18n`
library. Nothing has to be done by hand for a normal build.

After adding, changing or moving a `qsTr()` string, regenerate the catalogues
and translate anything new:

```sh
cmake --build --preset windows-msvc-debug --target wjs_update_translations
```

That target runs the kit's `lupdate` over the source list in
`cmake/Translations.cmake` and rewrites every `i18n/*.ts`; the translations
already there are kept, and a new message is written as `type="unfinished"`
until a person fills it in. Qt Linguist (in the same `qttools` module) opens a
`.ts` for translating. The test `i18n.catalogues` regenerates the catalogues
into a temporary directory and fails when the checked-in ones differ, so a
`qsTr()` without its catalogue is caught (docs/testing.md,
docs/ci.md#the-jobs-in-ciyml).

The translations are **mechanical and unreviewed** for now; a later pass with
the literacy reviewer replaces them (docs/audit-0.1.md, section 8).

## Packaging

A build can be installed and packaged. The `install()` rules put both programs,
the words and dice they read, and the notices into an install tree; CPack turns
that tree into a ZIP, and into the platform's own package where its tool is
installed. Nothing here needs the network.

One package holds both programs rather than one package each: the window cannot
run without the dictionary and the dice, the command-line solver is small, and a
per-program split would duplicate exactly those files and the notices. The
library is not installed at all; it is this tree's own headers and archive, not
a third-party SDK ([FAQ](faq.md)).

Prerequisites: a built preset (above), and, optionally, the tool for a native
package. Without it the package is still made in the formats that need no tool,
and the configure says which it will make. A ZIP is always offered.

| Tool | Adds | Where |
| --- | --- | --- |
| `makensis` | the NSIS installer (`.exe`) | Windows, from [NSIS](https://nsis.sourceforge.io/) |
| `dpkg-deb` and `dpkg-shlibdeps` | the `.deb` | Linux, in `dpkg-dev` |
| `rpmbuild` | the `.rpm` | Linux, in `rpm` |
| `hdiutil` | the `.dmg` | macOS, part of the system |

```sh
# Windows, from the same Developer PowerShell the build used
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release
cpack --preset package-windows-msvc-release

# or without a preset, in the build tree
cd build/windows-msvc-release
cpack -G ZIP
```

The package presets are `package-windows-msvc-release`,
`package-linux-gcc-release` and `package-macos-clang-release`; each uses the
configure preset whose build tree it packages. `-G` names one generator;
without it, cpack runs every generator the configure selected.

What lands in `build/<preset>/packages/`:

| Generator | Produces | Run when |
| --- | --- | --- |
| ZIP | `WordJewelsSolver-<version>-<system>.zip` | always |
| NSIS | `WordJewelsSolver-<version>-win64.exe` | Windows, when `makensis` is found |
| DragNDrop | `WordJewelsSolver-<version>-Darwin.dmg` | macOS |
| TGZ | `WordJewelsSolver-<version>-Linux.tar.gz` | Linux |
| DEB | `wordjewelssolver_<version>_<arch>.deb` | Linux, when `dpkg-deb` and `dpkg-shlibdeps` are found |
| RPM | `wordjewelssolver-<version>-<release>.<arch>.rpm` | Linux, when `rpmbuild` is found |

The ZIP's contents are under one directory named after the package (CPack's
default for the archive generators):

```text
WordJewelsSolver-0.1.0-win64/
  bin/
    WordJewelsSolver.exe
    wjs-cli.exe
    Qt6*.dll, qt.conf
    dictionaries/en.txt
    dice/3x3.txt … 6x6.txt
    NOTICES.txt
    LICENSE.txt
  plugins/            Qt's platform and image-format plugins
  qml/                the Qt QML modules the window imports, Quick 3D included
```

The installed window is self-contained on Windows: Qt's own install-time deploy
script (`qt_generate_deploy_qml_app_script`, `src/app/CMakeLists.txt`) copies the
runtime beside the program, and the plugins and QML modules one level above it,
with the `qt.conf` that points there. `bin/` is the directory both programs read
their dictionary from and the window reads its dice from, so a program moved out
of the package needs that directory moved with it. On Linux there is no deploy
step: the window uses the system's Qt, and the distribution packages name what
it needs. On macOS the bundle carries Qt and its data inside itself; no Mac has
built the window with Qt yet, so that layout is configured but unverified.

`cmake --install build/<preset> --prefix <dir>` writes the same tree without
packaging it. That is useful for a look, and it is what the packaging checks
below unpack.

On Windows the package needs the same Developer shell the build used:
`windeployqt` copies the compiler's runtime when `VCINSTALLDIR` is set, and
prints a warning and skips it when it is not.

### Checking a package

Two checks cover packaging:

- `package.config` is a ctest in every build. It reads the generated
  `CPackConfig.cmake` and install scripts, and fails if the package does not
  name this project and its licence, if no ZIP is offered, or if the install
  rules stopped carrying either program, the dictionary, the dice, the notices
  or the licence text.
- `tools/package/check_package.ps1` makes the package (or takes one with
  `-Archive`), checks its required files, writes `SHA256SUMS` beside it, unpacks
  it, solves a board with the packaged `wjs-cli` and its own dictionary, and
  runs the packaged `WordJewelsSolver --smoke 10` with the offscreen platform
  plugin:

```powershell
./tools/package/check_package.ps1 -Build build/windows-msvc-release
./tools/package/check_package.ps1 -Build build/windows-msvc-release -NoSmoke
```

The Windows CI leg runs the same script and uploads the ZIP ([CI](ci.md)).

### Third-party notices

Every package carries `NOTICES.txt` beside the programs and the GPL text as
`LICENSE.txt`. The application also embeds the notice as
`:/notices/NOTICES.txt`, which the About screen shows under Licences. The file
is assembled at configure time from `notices/fragments/`
(`cmake/Notices.cmake`) and states the project's licence; that the application
binary is GPL-3.0-only because it links Qt Quick 3D; Qt's LGPL-3.0 and GPL-3.0
options, with the LGPL text and the Quick 3D section; the ENABLE word list's
public-domain status; the translations; the bundled asset; and that Catch2 is
used only to test. [Licensing](licensing.md#notices) explains what each entry
means, and the `notices.sections` test holds the generated file to those
sections in every build.

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

A build can be installed and packaged ([Packaging](#packaging)); without that
step the programs run from `build/<preset>/bin/`, with the dictionary and dice
the build placed beside them.

## Troubleshooting

- **"Could not find toolchain file: /scripts/buildsystems/vcpkg.cmake"** or
  **"The vcpkg toolchain file was not found"**: `VCPKG_ROOT` is not set in the
  shell (or Visual Studio) that ran the configure, or does not point at a vcpkg
  clone. See [vcpkg](#vcpkg); a program started before `setx` needs restarting.
- **"does not contain the pinned commit"**: the clone is on a commit that does
  not have `cmake/vcpkg-commit.txt` in its history. This is what a clone left in
  **detached HEAD** looks like after a `vcpkg` bootstrap: `git -C
  "$VCPKG_ROOT" status` says "HEAD detached", and `git pull` does not help.
  Check the branch out: `git -C "$VCPKG_ROOT" checkout master` (then `pull` if
  the pin is not there).
- **"A suitable version of git was not found (required v2.55.0.windows.5)"**
  and then a failed download of PortableGit: the system git is older than the
  vcpkg tool requires. Either install git 2.55 or newer, or put the git that
  ships with Visual Studio on `PATH` for the configure — for example
  `C:\Program Files\Microsoft Visual Studio\2026\Community\Common7\IDE\CommonExtensions\Microsoft\TeamFoundation\Team Explorer\Git\cmd`
  (the path of your own install).
- **`wjs-cli: ...dictionaries/en.txt: the dictionary file could not be opened`**:
  there is no `dictionaries/en.txt` beside the executable. A build copies it
  there from `data/dictionaries/en.txt`; if you moved the executable, move the
  `dictionaries` folder with it, or pass a word list with `-d <file>`.
- **Qt is not found**: pass `-DCMAKE_PREFIX_PATH=<kit directory>` (the directory
  that holds `lib/cmake/Qt6`), or set `QT_ROOT_DIR`. On the author's machine the
  shared tree is `C:\Repos\common\qt\6.12.0\msvc2022_64`, and
  `C:\Repos\common\env.ps1` sets it: dot-source that script before configuring.
- **A program fails to start with `STATUS_DLL_NOT_FOUND`**: on Windows, a Debug
  build needs the ASan runtime beside it (the build copies it) and the Qt window
  needs Qt's DLLs (the build runs `windeployqt`). If you moved the executable
  out of `bin/`, move those with it.
- **`cpack` makes only a ZIP, or fails with "Cannot find NSIS compiler
  makensis"**: NSIS is optional and only `makensis` on `PATH` at configure time
  adds the installer; reconfigure after installing it. A `-G NSIS` names the
  generator on the command line and overrides that check, so it fails rather
  than degrade - the same is true of `-G DEB` or `-G RPM` where their tools are
  missing.
- **"Cannot find NSIS compiler makensis"**: see above; install NSIS or drop the
  `-G NSIS`.
- **The packaged window does not start, but the one in `bin/` does**: the
  package keeps Qt's plugins and QML modules one level above `bin/`
  ([Packaging](#packaging)); moving `bin/` alone leaves them behind. Unpack the
  whole archive, or move the `plugins/`, `qml/` and `qt.conf` with it.
