# Building

This page builds WordJewelsSolver from source. To use `wjs-cli`, see the
[home page](index.md#using-the-command-line-solver).

CI builds and tests all three desktops: Windows (MSVC 19.51), Linux (GCC 15.2,
Ubuntu 26.04) and macOS (Apple clang 21, Apple silicon), each in Debug with
warnings as errors and sanitizers. Those were the compilers the runner images
shipped at the last verified run (2026-10-21): CI installs no compiler, so the
versions move with the images. Only the Windows leg builds the Qt Quick
application; the Linux and macOS legs build the library, the platform layer, the
command-line solver and the scanner. A Release package of the application is
built and checked on Windows on every push; the Linux and macOS application
builds and packages are in the release workflow, which has not run yet, and an
unsigned iOS compile passes in its own workflow. A nightly run adds clang-tidy,
MSVC's `/analyze` and a Clang AddressSanitizer + UBSan build, but a schedule
starts only from the default branch, so it cannot until `develop` is merged to
`main` ([CI](ci.md#nightly-analysis)). The compiler minimums below are the
intent; only the versions just named have been tried. What CI runs is in
[CI](ci.md).

## Requirements

| What | Version | Notes |
| --- | --- | --- |
| CMake | 4.0 or newer | The presets require it. CI pins 4.4.2 |
| Ninja | any recent | The presets use the Ninja generator (CI pins 1.13.2) |
| A C++23 compiler | MSVC from Visual Studio 2022 17.x or Visual Studio 2026; GCC 14 or newer; AppleClang 16 or newer | Verified: MSVC 19.51 (Visual Studio 2026), GCC 15.2, Apple clang 21. The code uses `std::expected` and `std::format` |
| Git | any recent | Clones this repository and checks the vcpkg commit |
| vcpkg | the labs' fork, at or after the commit in `cmake/vcpkg-commit.txt` | One shared clone, found through `VCPKG_ROOT`, see [below](#vcpkg) |
| PowerShell 7 (`pwsh`) | 7.0 or newer | For the [platform-isolation check](platform-isolation.md#enforcement), the [data checks](testing.md#what-there-is) (the word lists, the dice, the phonics data and the feedback sounds) and the [packaging check](#checking-a-package). Optional: without it those ctests are not registered |
| Network access | | The first configure bootstraps vcpkg and builds Catch2. Packaging needs none |
| Qt | 6.12 | Only for the optional Qt Quick window, see [below](#the-qt-quick-window); an iOS build also needs Xcode, see [iOS](#ios) |
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

vcpkg is never vendored: there is no submodule and no `deps/vcpkg`, so a fresh
clone, or a `git worktree`, needs only `VCPKG_ROOT`, and every tree on the machine
shares the one clone and its binary cache. Nothing in a working tree needs to
point at it. The repository adds one overlay port of its own for Tracy
(`cmake/vcpkg-ports/tracy`, named in `vcpkg-configuration.json`; see
[Profiling](profiling.md)).

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

The programs are in `build/<preset>/bin/`: `WordJewelsSolver`
(`WordJewelsSolver.exe` on Windows), `wjs-cli` (`wjs-cli.exe`), and the test
binaries `wjs_unit_tests` and `wjs_perf_tests`
([Testing](testing.md#performance-tests)). The window is built when Qt 6.12 is
found; the paragraph under [The Qt Quick window](#the-qt-quick-window) says how
it is found and how to skip it. `ctest` fails if it finds no tests at all. A
build can also be installed and made into a package; [Packaging](#packaging) has
the commands and the output.

The executable targets are named after the programs they make, so one of them
can be built alone:

```sh
cmake --build --preset windows-msvc-debug --target WordJewelsSolver
cmake --build --preset windows-msvc-debug --target wjs-cli
```

The rest of the target list is libraries (`wjs_*`), the test binaries
(`wjs_*_tests`), and the screens' test harnesses (`wjs_parent_preview`,
`wjs_scan_preview`, `wjs_quick_tests`). IDEs that read CMake's `FOLDER`
property, Visual Studio's CMake Targets View among them, put the programs and
the application's modules in an Application folder, the libraries under
Libraries, and the tests and tools under Tests and Tools. `wjs_scan_render` is
a one-off corpus renderer and is not part of the default build; ask for it by
target when it is wanted ([Testing](testing.md#the-scanner)).

## Presets

A preset configures into `build/<preset>/` and is named
`<os>-<compiler>-<config>`. Each is available only on its own host, so
`cmake --list-presets` lists the ones for the machine you are on. The plain Debug
presets have the sanitizers **and Tracy** on; a Release preset has the Qt
application on (a missing Qt is an error, unless you pass `-DWJS_BUILD_APP=OFF`, as
the solver-only release legs do), Tracy and the sanitizers off and reviewed phonics
required.

| Preset | Host | vcpkg triplet | Notes |
| --- | --- | --- | --- |
| `windows-msvc-debug` | Windows | `x64-windows` | Debug, with AddressSanitizer |
| `windows-msvc-release` | Windows | `x64-windows` | Release: the package that ships |
| `linux-gcc-debug` | Linux | `x64-linux` | Debug, with AddressSanitizer and UBSan; uses `g++` |
| `linux-gcc-release` | Linux | `x64-linux` | Release |
| `linux-gcc-coverage` | Linux | `x64-linux` | Debug with gcov instrumentation and the sanitizers and Tracy off; `coverage-unit` gates it at 90% lines / 80% branches |
| `windows-clang-coverage` | Windows | `x64-windows` | Debug with LLVM source-based coverage (clang-cl, Visual Studio generator, multi-config); `coverage-unit` is the same gate |
| `linux-clang-debug` | Linux | `x64-linux` | Debug, with Clang's AddressSanitizer and UBSan; uses `clang++`. The nightly Clang sanitizer leg |
| `macos-clang-debug` | macOS | `arm64-osx` | Debug, with AddressSanitizer and UBSan |
| `macos-clang-release` | macOS | `arm64-osx` | Release |
| `ios-release` | macOS | `arm64-ios` | An Xcode project for an iPhone or iPad: Release, no tests, no `wjs-cli`, unsigned unless given a team and a profile. Needs Qt's iOS kit; see [iOS](#ios) |
| `android-arm64-release` | any | `arm64-android` | An APK and an AAB for an arm64 tablet or phone: Release, no tests, no `wjs-cli`, unsigned unless asked. Needs Qt's Android kit, the SDK, the NDK and a JDK; see [Android](#android) |
| `android-arm64-debuggable` | any | `arm64-android` | As above, a debuggable APK signed with Android's debug key. Tracy is off, so it is not named `*-debug` |
| `web-release` | any | `wasm32-emscripten` | A directory of static files for a browser (WebAssembly, one thread): Release, no tests, no `wjs-cli`, no hardening flags. Needs Qt's WebAssembly kit and Emscripten; see [Web](#web) |
| `windows-msvc-profiling` | Windows | `x64-windows` | RelWithDebInfo with Tracy, sanitizers off. A development build, never shipped: see [Profiling](profiling.md) |
| `linux-gcc-profiling` | Linux | `x64-linux` | As above, with `g++` |
| `macos-clang-profiling` | macOS | `arm64-osx` | As above |
| `windows-msvc-analysis` | Windows | `x64-windows` | MSVC's `/analyze`, with warnings as errors and SARIF logs; no tests, no application, Tracy and sanitizers off. The nightly MSVC analysis leg |
| `linux-gcc-analysis` | Linux | `x64-linux` | A Debug configure for clang-tidy's compile database and CodeQL: no tests, no application, Tracy and sanitizers off (so vcpkg installs nothing), C++ module scanning off. The nightly clang-tidy and CodeQL legs |

There are test presets of the same names (`ctest --preset <name>`) for every preset
that builds tests, tier presets (`<preset>-unit`, `-integration`, `-system` and
`-ui` for the three Debug presets) and `windows-msvc-perf`, `linux-gcc-perf` and
`macos-clang-perf` for the performance timings ([Testing](testing.md#the-tiers)); and
package presets for the three desktops' Release builds ([Packaging](#packaging)).

`windows-msvc-analysis` and `linux-gcc-analysis` turn the application and the tests
off, so the analysis stays on the sources.

The macOS presets use the Apple Silicon triplet. On an Intel Mac, pass
`-DVCPKG_TARGET_TRIPLET=x64-osx` (untried). The Linux GCC presets name the
compiler `g++`; if yours is older than GCC 14, pass `-DCMAKE_CXX_COMPILER=g++-14`, or
whichever newer one you have.

For a combination the presets do not cover, make your own in
`CMakeUserPresets.json`, which is git-ignored.

## Options

Pass these to the configure step, for example
`cmake --preset linux-gcc-debug -DWJS_BUILD_TESTS=OFF`.

| Option | Default | Meaning |
| --- | --- | --- |
| `WJS_BUILD_CLI` | `ON` | Build `wjs-cli` |
| `WJS_BUILD_APP` | `AUTO` | Build the Qt Quick window. `AUTO` builds it when Qt 6.12 is found and leaves it out otherwise; `ON` requires Qt and fails the configure without it; `OFF` never looks. Needs Quick 3D, so the binary is GPL-3.0 only |
| `WJS_BUILD_TESTS` | `ON` | Build the test suites, and register the platform tests. Turning it off also stops vcpkg building Catch2 |
| `WJS_BUILD_SHAKE_PROTOTYPE` | `OFF` | Build the harness around the 3D board scene as `ShakePrototype`. Needs the application built (`WJS_BUILD_APP=AUTO` with Qt found, or `ON`) and Qt Quick 3D; the harness links the same scene module as the application, so both are GPL-3.0 only. See [the prototype](design/shake-prototype.md#how-it-was-measured) |
| `WJS_ENABLE_SANITIZERS` | `ON` | AddressSanitizer (and UBSan, where the compiler has it) in Debug builds. See [Sanitizers](#sanitizers-and-warnings) |
| `WJS_ENABLE_HARDENING` | `ON` | The shared hardening flags on the libraries, the command line and the application: stack protector, `_FORTIFY_SOURCE=3` outside Debug, relro and immediate binding, PIE and control-flow protection on GCC and Clang; `/GS /guard:cf /sdl` on MSVC. Only the `linux-fuzz` preset turns it off (a fortified libc call bypasses AddressSanitizer); the switch is also for finding a toolchain problem. See [Security](security.md#hardening) |
| `WJS_ENABLE_FUZZING` | `OFF` | The libFuzzer harnesses of `libs/<lib>/fuzz`, with AddressSanitizer and UBSan. Clang only, and not with the other instrumentation switches, Tracy or the hardening flags; the `linux-fuzz` preset sets it on its own. See [Fuzzing](fuzzing.md) |
| `WJS_SANITIZER_OPTIMIZE` | `ON` | Build first-party code with `/O1` instead of `/Od` in a Debug build that has MSVC's AddressSanitizer on. The sanitizer makes every unoptimised access and struct copy a call, and the tests ran about twice as fast with it. `OFF` to step through the code in a debugger. See [Sanitizers](#sanitizers-and-warnings) |
| `WJS_ENABLE_MSVC_ANALYSIS` | `OFF` | MSVC's static analyzer during compilation (`/analyze`, with `/W4 /WX`), SARIF logs written beside the objects. Needs MSVC; the `windows-msvc-analysis` preset turns it on. See [Static analysis](#static-analysis) |
| `WJS_ENABLE_COVERAGE` | `OFF` | Source coverage instrumentation, read back with gcovr (GCC/Clang) or llvm-cov (clang-cl). Only the `*-coverage` presets turn it on. See [Coverage](#coverage) |
| `WJS_COVERAGE_MIN_LINE` | `90` | The unit tier's line floor, as a percentage. `coverage-unit` fails below it |
| `WJS_COVERAGE_MIN_BRANCH` | `80` | The unit tier's branch floor, as a percentage. `coverage-unit` fails below it |
| `WJS_COVERAGE_OUTPUT_DIR` | `<build>/coverage` | Where the reports and profile data are written |
| `WJS_REQUIRE_REVIEWED_PHONICS` | `ON` for Release, RelWithDebInfo and MinSizeRel builds and multi-config generators, else `OFF` | The review gate: the build runs `tools/data/check_phonics_review.cmake` (a CMake script; no PowerShell) and stops while a file in `data/phonics` has no reviewed-by and reviewed-on header. The repository has no phonics data yet. The release presets set it. See [Testing](testing.md#phonics-data-the-checker-and-the-review-gate) |
| `WJS_PACKAGE_PRERELEASE` | empty | A SemVer prerelease identifier appended to the package version (`rc.1` gives `...-0.1.0-rc.1-...`, a deb `0.1.0~rc.1`, an rpm release `0.rc.1`). `release.yml` passes it from the tag |
| `WJS_IOS_SIGNING_TEAM`, `WJS_IOS_SIGNING_PROFILE` | empty | The Apple team ID and the provisioning profile's name that sign the iOS application target. See [iOS](#ios) |
| `WJS_ENABLE_TRACY` | `ON` for Debug build types, else `OFF` | The Tracy profiler: makes the calls in `wjs/profiling.hpp` real, and installs the client and its tools through the vcpkg `profiling` feature. A Debug build is instrumented by default, so a regular development build can be connected to by the profiler; a Release build can be instrumented by hand, for data from a real session; nothing that ships may have it on ([Profiling](profiling.md)) |

C++23 is required, with compiler extensions off, for every target.

## The Qt Quick window

The window is the game of [Application design](design/application.md) in Qt Quick,
over the library: Solo Play, the Family Assistant that reads a board with the camera,
and the parent area (`apps/app/src`; [Architecture](architecture.md#the-qt-quick-window)). Its target is
`WordJewelsSolver`, the program it makes (`WordJewelsSolver.exe` on Windows,
where it is a GUI-subsystem program with no console window; an application
bundle on macOS). `WJS_BUILD_APP` is `AUTO`, so the window is built whenever a
Qt 6.12 kit is found and the same preset still builds `wjs-cli` and the tests on
a machine without one. Pass `-DWJS_BUILD_APP=OFF` to leave the window out
deliberately, or `-DWJS_BUILD_APP=ON` to make a missing Qt a configure error
(the Windows CI leg does the latter).

It needs Qt 6.12 with the modules Core, Gui, Qml, Quick, Quick Controls 2, Quick
Layouts, Quick Shapes, Quick 3D and LinguistTools (`lupdate` and `lrelease`, in the
base kit from 6.12; before that the `qttools` module), as a prebuilt install (from
the Qt online installer, or with
[aqtinstall](https://github.com/miurahr/aqtinstall)). In the 6.12 repository Quick 3D
and its shader tools are modules of their own, so a kit installed by aqtinstall names
`qtquick3d qtshadertools`; CI also names the optional `qtmultimedia qtsensors
qtspeech` (below). aqtinstall 3.3.0, the last PyPI release, cannot read the per-architecture
Windows repository of Qt 6.11 and later; CI installs it from the commit in
`.github/toolchain-versions.json` (`qt.aqtsource`). Qt is not built by vcpkg.
The board scene is 3D, so a build of the window links Qt Quick 3D, which Qt
offers only under the GPL-3.0 or commercially: the binary is **GPL-3.0 only**
(the recorded decision, [Board design](design/board.md#rendered-3d-or-drawn-in-2d)).

**The feedback and camera modules are optional.** Sound, speech and motion use
Qt Multimedia, Qt Sensors and Qt TextToSpeech, and Mode A's live camera uses Qt
Multimedia. The application finds each with `OPTIONAL_COMPONENTS`: a kit
without one builds that engine's **silent variant**, or for the camera the
**`camera-none`** backend, and the game is complete, only quieter and with the
typed board as the scanner's way in. The shared kit on the author's machine,
`C:\Repos\common\qt\6.12.0\msvc2022_64`, has all three: the module archives were
fetched from the Qt repository and merged into the kit
(`lib/cmake/Qt6{Multimedia,Sensors,TextToSpeech}`, their DLLs in `bin`), and the
kit is found with no other flag. A kit that lacks them needs
nothing special — configure and build as below — and the silent variants are
chosen for the missing modules. The Windows CI leg installs all three with
[install-qt-action](https://github.com/jurplel/install-qt-action) instead of the
PyPI aqtinstall, because Qt 6.11+ split the Windows repository per architecture
([CI](ci.md#toolchain-choices)).

The three feedback backends are chosen the way `wjs/profiling.hpp` is
(docs/profiling.md): one directory under `apps/app/src/variants/` goes on the include
path, every variant has the same header and interface, and no source asks
whether the module is there. The camera uses the same pattern: `apps/app/src/camera`
holds `CameraSource` (the QML element the screen binds to), `CameraBackend` (the
seam) and the frame conversion, and `apps/app/src/variants/camera-multimedia` or
`camera-none` supplies the backend. Where the kit has Qt Multimedia, the ready
screen shows a live `VideoOutput` and Capture reads one frame; where it does
not, the same screen says there is no camera and the typed board is the way on.
The camera's Qt Multimedia plugins and QML module are deployed beside the window
(`qml/QtMultimedia`, `plugins/multimedia`) and the package check requires them
([Packaging](#packaging)).

The sound assets are the project's own
placeholders: `tools/sounds/make_sounds.ps1` writes `assets/sounds/*.wav`
(16-bit PCM mono, deterministic; `-Check` proves the checked-in files), and the
build embeds them as `:/sounds/<name>.wav`. The real phoneme clips and cheers
replace them after the literacy review. `wjs_unit_tests "[feedback],[motion],[haptics]"`
and `wjs_app_tests "[feedback],[motion]"` run the policy and the processing
without a speaker or a sensor; `ctest -L camera` runs the camera's cases and
`camera.screens` draws the live view headless from a fake source.

`cmake/FindQt6.cmake` looks for the kit, in this order:

1. `-DCMAKE_PREFIX_PATH=<path to the kit>` or `-DQt6_DIR=<...>`, which always win;
2. the `QT_ROOT_DIR` environment variable, which `install-qt-action` and similar
   tools export;
3. the usual install locations for the platform: `C:/Qt`, `~/Qt` and the labs'
   shared `C:/Repos/common/qt` on Windows; `~/Qt`, `/opt/Qt` and the system
   `qt6` directories on Linux; `~/Qt` and Homebrew's `qt6` on macOS.

With a kit in one of those places, the preset needs no flag at all:

```sh
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug --target WordJewelsSolver
build/windows-msvc-debug/bin/WordJewelsSolver.exe
```

A kit somewhere else is named on the command line. `ON` here also proves the
configure fails if that kit cannot be found:

```sh
cmake --preset windows-msvc-debug -DWJS_BUILD_APP=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64
```

On Windows the build runs `windeployqt` after linking, so Qt's DLLs and the QML
modules the window imports are copied beside the executable and it starts from
anywhere. An installed or packaged copy is given its Qt runtime by Qt's own
install-time deploy script instead (see [Packaging](#packaging)). On Linux
nothing is deployed: Qt is found at run time the same way it was found at build
time, and a distribution package declares it as a dependency. On macOS the
application bundle is given Qt's runtime at install time; no macOS desktop build of
the window has run yet (the release workflow's macOS leg will be its first), so that is
configured but unverified.

With the window enabled, `ctest` also runs `wjs_app_tests`, the Catch2 tests of
the model the window talks to, and the Qt Quick Test suites that drive the screens
(`ctest -R quick`, `wjs_quick_tests`). They need Qt's runtime beside them, which the
build arranges the same way, and run under the offscreen platform plugin.

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
One Qt Linguist catalogue per language is checked in under `apps/app/translations/`
(`wjs_en.ts` … `wjs_ja.ts`), and `apps/app/src/language_manager.*` loads the compiled
one from the application's resources as `:/i18n/wjs_<code>.qm`.

`lrelease` runs at build time, as part of building the window: each `.ts` is
compiled into `build/<preset>/apps/app/i18n/` and embedded in the `wjs_i18n`
library. Nothing has to be done by hand for a normal build.

After adding, changing or moving a `qsTr()` string, regenerate the catalogues
and translate anything new:

```sh
cmake --build --preset windows-msvc-debug --target wjs_update_translations
```

That target runs the kit's `lupdate` over the source list in
`cmake/Translations.cmake` and rewrites every `apps/app/translations/*.ts`; the translations
already there are kept, and a new message is written as `type="unfinished"`
until a person fills it in. Qt Linguist (in the same package as `lupdate`) opens a
`.ts` for translating. The test `i18n.catalogues` regenerates the catalogues
into a temporary directory and fails when the checked-in ones differ, so a
`qsTr()` without its catalogue is caught (docs/testing.md,
docs/ci.md#the-jobs-in-ciyml).

The translations are **mechanical and unreviewed** for now; a later pass with
the literacy reviewer replaces them (docs/design/application.md, "Languages, right-to-left and fonts").

## Running the programs

`WordJewelsSolver` takes these options; `wjs-cli`'s are in the
[home page](index.md#using-the-command-line-solver).

| Option | Meaning |
| --- | --- |
| `--store <file>` | The file the parent area keeps its settings in and the game keeps its finds in. Without it, the per-user application data directory; where there is nowhere to keep one, the store lives in memory only |
| `--phase-directory <dir>` | A directory of `phase-<x>.graphemes.txt` and `phase-<x>.words.txt` files for the phonics board; `--phase <name>` picks the phase and applies only with it |
| `--dictionary <file>` | A word list instead of `dictionaries/en.txt` beside the executable |
| `--seed <n>` | A fixed seed, so the boards repeat; a smoke run plays a fixed seed when none is given |
| `--smoke [<frames>]` | Plays the shake cycle, measures that many frames (300 without a number), prints the frame report and leaves; see [Profiling](profiling.md#the-smoke-mode) |
| `--wait-for-profiler`, or `WJS_WAIT_FOR_PROFILER=1` | In a Tracy build, waits up to 5 seconds for a profiler to attach before the start-up it is meant to see; a smoke run always does |

Exit codes. `WordJewelsSolver`: **0** ran to the end; **2** the game could not be set
up (the word list, the dice or the phase directory is missing or unreadable); **3** a
smoke run presented no frame (a machine with no display cannot render Quick 3D); **4**
the window failed to load. 1 is left to the toolkit and to a crash. `wjs-cli`:
**0** solved, **1** usage, **2** the word list, **3** the board, and **4** an
exception the program did not foresee and **5** the result could not be written to
standard output (the codes 0 to 3 are in the [home
page](index.md#exit-codes)). When `WordJewelsSolver` cannot start it says why in a
dialog on Windows, where it has no console, and on standard error everywhere; set
`WJS_NO_DIALOGS=1` (or run with `QT_QPA_PLATFORM` set to `offscreen` or `minimal`) to
suppress the dialog in a script or a test.

`WordJewelsSolver.exe` is a GUI-subsystem program: its standard output (the smoke
report) reaches a redirect, but Qt's log lines (`qWarning` and the like) do not
unless `QT_ASSUME_STDERR_HAS_CONSOLE=1` is set.

## iOS

An iPhone or iPad build is a device build for the author's own devices, ad hoc
signed; [Installing on my iPhone/iPad](ios-install.md) is the install, and
[CI](ci.md#ios) is how the release workflow makes it. It needs a Mac with Xcode 16
or later (the iOS 18 SDK), because Xcode is what compiles, signs and archives it.
The unsigned compile (configure, build and the bundle check below) passes on a GitHub
macOS runner; signing, the IPA, an install and a run on a device have not been done
([what is unverified](ci.md#what-ios-has-and-has-not-shown)).

Qt comes as two kits of the same version: its iOS kit (static Qt), and a macOS Qt
for the host tools a cross build runs. aqtinstall installs both with
`--autodesktop`:

```sh
aqt install-qt mac ios 6.12.0 ios --autodesktop --outputdir ~/Qt \
    -m qtquick3d qtshadertools qtmultimedia qtsensors qtspeech
export QT_ROOT_DIR=~/Qt/6.12.0/ios
export QT_HOST_PATH=~/Qt/6.12.0/macos
export VCPKG_ROOT=...            # as for any preset

cmake --preset ios-release       # an Xcode project in build/ios-release/
cmake --build --preset ios-release   # unsigned: compiles for a device
libs/platforms/ios/packaging/verify_ipa.sh "$(find build/ios-release -name WordJewelsSolver.app -type d | head -n 1)"
```

(`aqt` is the pinned one: `.github/toolchain-versions.json`, `qt.aqtsource`.) The
preset turns the tests and `wjs-cli` off, Tracy and the sanitizers off, builds for
`arm64` with an iOS 18.0 deployment target, and signs nothing. The vcpkg toolchain is
still the CMake toolchain file, chaining to Qt's; the manifest has no dependency
without the tests or Tracy, so vcpkg installs nothing.

A signed build gives the application target a team and a provisioning profile name
(the profile's name in the Apple portal), with the distribution certificate in a
keychain and the profile in `~/Library/MobileDevice/Provisioning Profiles`:

```sh
cmake --preset ios-release -DWJS_IOS_SIGNING_TEAM=<team id> "-DWJS_IOS_SIGNING_PROFILE=<profile name>"
xcodebuild archive -project build/ios-release/WordJewelsSolver.xcodeproj -scheme WordJewelsSolver \
    -configuration Release -destination "generic/platform=iOS" -archivePath build/ios-release/WordJewelsSolver.xcarchive
xcodebuild -exportArchive -archivePath build/ios-release/WordJewelsSolver.xcarchive \
    -exportPath build/ios-release/export -exportOptionsPlist ExportOptions.plist
```

`ExportOptions.plist` is the one `release.yml` writes (method `ad-hoc`, the team, manual
signing, `Apple Distribution`, the profile by name); read it there. The built `.app`
is where Xcode puts it, so the check above looks for it.

## Android

An Android build makes an **APK** (or an **AAB**) of the application for an arm64 tablet or
phone running Android 9 or later; [CI](ci.md#android) is how the debug-signed one and the
release one are made, and what has and has not been shown on a device is there too
([what is unverified](ci.md#what-android-has-and-has-not-shown)). It cross-builds from
Windows, Linux or macOS with five things, all of one version:

| What | Which | Where it is on the author's machine |
| --- | --- | --- |
| Qt for Android | 6.12.0, `android_arm64_v8a` | `C:\Repos\common\qt\6.12.0\android_arm64_v8a` (`QT_ANDROID_DIR`) |
| A desktop Qt | 6.12.0, for the host tools and `androiddeployqt` | `C:\Repos\common\qt\6.12.0\msvc2022_64` (`QT_HOST_PATH`) |
| The Android SDK | platform 36, build-tools 36.0.0 | `C:\Repos\common\android-sdk` (`ANDROID_SDK_ROOT`) |
| The NDK | the one the kit was built with: r27c, `27.2.12479018` | `ANDROID_NDK_ROOT` |
| A JDK | 17 or later, on `PATH` or `JAVA_HOME` | any |

`C:\Repos\common\env.ps1` sets `QT_ANDROID_DIR`, `QT_HOST_PATH` (the Windows Qt) and
`ANDROID_SDK_ROOT`; set `ANDROID_NDK_ROOT` to the NDK under the SDK (the shared SDK has
28.2.13676358, which compiled and linked this program here; the kit's own toolchain file names
r27c, so use that, `sdkmanager "ndk;27.2.12479018"`, for a build you ship). **Write
those paths with forward slashes** (`C:/Repos/...`): a backslash in `QT_HOST_PATH` is read as an
escape by CMake's package finder and the configure stops with `Invalid character escape`.
With aqtinstall, both Qt kits come in one go:

```sh
aqt install-qt all_os android 6.12.0 android_arm64_v8a --autodesktop --outputdir ~/Qt \
    -m qtquick3d qtshadertools qtmultimedia qtsensors qtspeech
export QT_ANDROID_DIR=~/Qt/6.12.0/android_arm64_v8a
export QT_HOST_PATH=~/Qt/6.12.0/gcc_64          # msvc2022_64 on Windows, macos on a Mac
export ANDROID_SDK_ROOT=...  ANDROID_NDK_ROOT=...
export VCPKG_ROOT=...                           # as for any preset
```

(`aqt` is the pinned one: `.github/toolchain-versions.json`, `qt.aqtsource`. The host is
`all_os` for Android from Qt 6.7. This is the command the install action runs; it was not run
here, where the kits were already in the shared tree.)

```sh
cmake --preset android-arm64-debuggable            # build/android-arm64-debuggable/
cmake --build --preset android-arm64-debuggable    # the apk target: a debug-signed APK
libs/platforms/android/packaging/verify_apk.sh \
    build/android-arm64-debuggable/apps/app/android-build/build/outputs/apk/debug/android-build-debug.apk --signed
adb install -r build/android-arm64-debuggable/apps/app/android-build/build/outputs/apk/debug/android-build-debug.apk
```

(`verify_apk.sh` finds `aapt2` and `apksigner` in the SDK's newest build-tools.) The package is
under the target's directory, `build/<preset>/apps/app/android-build/build/outputs/`.
`android-arm64-release` is the same with Release and an unsigned APK
(`.../apk/release/android-build-release-unsigned.apk`, which no device installs); `--target
aab` makes the bundle (`.../bundle/release/android-build-release.aab`). The presets turn the
tests and `wjs-cli` off, and Tracy and the sanitizers off: Tracy's client listens on the
network, which the application has no permission for. For that reason the debuggable preset
is not named `*-debug` ([CI](ci.md#android)). The vcpkg toolchain is still the CMake
toolchain file, chaining to Qt's; the manifest has no dependency without the tests or Tracy, so
vcpkg installs nothing. The module scan for C++ modules is off in the preset: nothing here uses
modules, and on a Windows host its command line exceeds what `cmd.exe` runs.

To sign a release build with your own key, ask for it on the configure line and name the
keystore in the environment (`tools/release/new_android_keystore.sh` makes one, and prints
what the release workflow's secrets need):

```sh
export QT_ANDROID_KEYSTORE_PATH=... QT_ANDROID_KEYSTORE_ALIAS=... \
       QT_ANDROID_KEYSTORE_STORE_PASS=... QT_ANDROID_KEYSTORE_KEY_PASS=...
cmake --preset android-arm64-release -DQT_ANDROID_SIGN_APK=ON -DQT_ANDROID_SIGN_AAB=ON
cmake --build --preset android-arm64-release
cmake --build --preset android-arm64-release --target aab
```

The application's modules are found as on the desktops: Qt Multimedia, Sensors and Text To
Speech are optional, and a kit without them (the shared one) builds the silent variants, so a
package from it has no live camera view, no shake and no speech. The CI kit has them.
No Android build runs the tests: the library's, the scanner's and the screens' tests run on the
host builds (a device or an emulator would be needed for these ones, and none is wired).

## Web

A web build makes a directory of static files, `build/web-release/web-bundle/`, that a browser plays:
the page, a boot script, Qt's loader and the program as WebAssembly. What it is, what it lacks and how a
child's progress is kept is on [Playing in a browser](web.md); [CI](ci.md#web) is how it is built and
looked at on every change. It cross-builds from Linux, macOS or Windows with three things, all of one
version:

| What | Which |
| --- | --- |
| Qt for WebAssembly | 6.12.0, `wasm_singlethread` (`QT_WEB_DIR`) |
| A desktop Qt | 6.12.0, for the host tools: moc, rcc, qmlcachegen, lrelease (`QT_HOST_PATH`) |
| Emscripten | **5.0.5**, the one the kit asks for (`EMSDK`); the kit refuses another |

With aqtinstall and emsdk (no module is needed: the web build has no Qt Quick 3D, Multimedia, Sensors or
Speech):

```sh
aqt install-qt all_os wasm 6.12.0 wasm_singlethread --autodesktop --outputdir ~/Qt
git clone https://github.com/emscripten-core/emsdk.git && cd emsdk
./emsdk install 5.0.5 && ./emsdk activate 5.0.5 && source ./emsdk_env.sh
export QT_WEB_DIR=~/Qt/6.12.0/wasm_singlethread
export QT_HOST_PATH=~/Qt/6.12.0/gcc_64          # msvc2022_64 on Windows, macos on a Mac
export VCPKG_ROOT=...                           # as for any preset
```

(`aqt` is the pinned one: `.github/toolchain-versions.json`, `qt.aqtsource`; the Emscripten version
and the emsdk commit are `emscripten.*` in the same file. Write paths with forward slashes on Windows.)

```sh
cmake --preset web-release
cmake --build --preset web-release                # the bundle is build/web-release/web-bundle
libs/platforms/web/packaging/verify_web.sh build/web-release/web-bundle --version 0.1.0
libs/platforms/web/packaging/smoke_web.sh build/web-release/web-bundle     # needs Chrome
python3 -m http.server --directory build/web-release/web-bundle            # then open localhost:8000
```

Things that differ from the other builds:

- **The 2D board scene.** The platform fragment says `WJS_PLATFORM_SCENE 2d`: the board scene is the drawn
  one (`apps/app/qml/scene/fallback`), and Qt Quick 3D is not looked for, linked or needed. The application
  is not GPL-3.0 only on Quick 3D's account there.
- **No import scan.** Qt's own finalisation scans every `.qml` file under `apps/app` and links what they
  import, which for a static build would pull in Quick 3D (the 3D scene's files), Qt Test (the UI suites'),
  Qt Multimedia and four more Qt Quick Controls styles: 10 MB of the program (43 MB before, 32 after). The fragment turns the scan off
  and names the modules itself (`libs/platforms/web/qml_plugins_web.cpp`, `platform.cmake`);
  `platform.web` fails when an application's QML imports a module not named there.
- **No hardening flags** (`WJS_ENABLE_HARDENING=OFF` in the preset). The shared hardening set links
  with `-z relro -z now`, which `wasm-ld` does not know (it warns "unknown -z value" at every link),
  and the set is `icl-common-infra`'s, copied in as `cmake/icl/`. The compile-time half (the stack
  protector, `_FORTIFY_SOURCE`) does work with Emscripten and has not been weighed for the web; turning
  it on is a change to the shared module (skip the linker options for a WebAssembly target), not to
  this repository.
- **The data is in the program.** The English word list and the dice are compiled into the program's
  resources and laid down in the browser's in-memory file system at start, as on Android.
- **Warnings are errors** here too: `EM_ASM`'s `$0` is silenced for the two files that use it, and a
  32-bit `size_t` is why the found-words hash (`libs/core/src/game_rules.cpp`) is 64 bits wide whatever
  `size_t` is (the value is unchanged where it already was).

The tests (`WJS_BUILD_TESTS`) are off: nothing here runs in a browser's test harness, and the library's tests
run on the host builds.

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
| `dpkg-deb` and `dpkg-shlibdeps` | the `.deb` (command-line solver only) | Linux, in `dpkg-dev` |
| `rpmbuild` | the `.rpm` (command-line solver only) | Linux, in `rpm` |
| `hdiutil` | the `.dmg` | macOS, part of the system |
| `linuxdeploy` and its Qt plugin | the `.AppImage` (the application, with Qt inside) | Linux; `libs/platforms/linux/packaging/build_appimage.sh` fetches them, pinned |

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
| DEB | `wordjewelssolver_<version>_<arch>.deb` | Linux, when `dpkg-deb` and `dpkg-shlibdeps` are found and the application is not built |
| RPM | `wordjewelssolver-<version>-<release>.<arch>.rpm` | Linux, when `rpmbuild` is found and the application is not built |
| *(AppImage)* | `WordJewelsSolver-<version>-x86_64.AppImage` | Linux, with the application: `build_appimage.sh`, not CPack |

The deb and rpm are for the command-line solver: the application needs a Qt 6.12
with Quick 3D that no distribution packages, so a package of it could only declare
dependencies nobody can satisfy. On Linux the application is the AppImage, which
carries its Qt. A prerelease is packaged under its own version:
`-DWJS_PACKAGE_PRERELEASE=rc.1` gives `...-0.1.0-rc.1-...`, a deb `0.1.0~rc.1` and
an rpm release `0.rc.1`.

CPack **refuses to package a build that has Tracy in it** (the Debug and profiling
presets): `cmake/PackagePreflight.cmake` stops it and says why, and a development
package has to be asked for by name with `cpack -D CPACK_WJS_ALLOW_DEV_PACKAGE=ON`.
The Release presets turn the application on (a missing Qt is an error), Tracy off
and the sanitizers off.

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
  plugins/            Qt's platform, image-format and multimedia plugins
  qml/                the Qt QML modules the window imports, Quick 3D included,
                      and QtMultimedia where the kit has the camera's module
```

The installed window is self-contained on Windows: Qt's own install-time deploy
script (`qt_generate_deploy_qml_app_script`, `apps/app/CMakeLists.txt`) copies the
runtime beside the program, and the plugins and QML modules one level above it,
with the `qt.conf` that points there. The camera adds Qt Multimedia to that
tree: `plugins/multimedia/*` (the platform's capture backend) and
`qml/QtMultimedia` (the `VideoOutput` the live view is), deployed because the
application imports the module. A kit without Qt Multimedia has neither and the
window says there is no camera. `bin/` is the directory both programs read
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
  or the licence text. Where the kit has Qt Multimedia it also reads the input
  list of the application's QML import scan and fails if the camera's
  `QtMultimedia` hint file is gone, because that is what makes the install-time
  deploy copy `qml/QtMultimedia` and the multimedia plugins.
- `tools/package/check_package.ps1` makes the package (or takes one with
  `-Archive`), checks its required files (and, when the window links Qt
  Multimedia, that `plugins/multimedia` and `qml/QtMultimedia` are in it),
  writes `SHA256SUMS` beside it, unpacks
  it, solves a board with the packaged `wjs-cli` and its own dictionary, and
  runs the packaged `WordJewelsSolver --smoke 10` with the offscreen platform
  plugin:

```powershell
./tools/package/check_package.ps1 -Build build/windows-msvc-release
./tools/package/check_package.ps1 -Build build/windows-msvc-release -NoSmoke
```

The Windows CI leg runs the same script on its Debug build, as a development
package, and `Package (Windows, Release)` runs it on the Release build, which must
carry no `TracyClient`, AddressSanitizer runtime or `.pdb` (`-ForbidTracy Auto`, the
default, holds a Release tree to that); each uploads its ZIP ([CI](ci.md)). iOS is
not packaged by CPack: the IPA is made by `xcodebuild` ([iOS](#ios)).

### Making a release package

A release is the Release presets, packaged, checked and signed. `release.yml`
does it on a `v*` tag ([CI, "Releases"](ci.md#releases)); the steps are the
ones to run by hand to see what a release will contain:

```powershell
# Windows, from a Developer PowerShell (the application needs Qt 6.12.0:
# QT_ROOT_DIR or CMAKE_PREFIX_PATH)
cmake --preset windows-msvc-release -DWJS_BUILD_APP=ON
cmake --build --preset windows-msvc-release
ctest --preset windows-msvc-release
./tools/package/check_package.ps1 -Build build/windows-msvc-release
./tools/release/check_release_build.ps1 -Build build/windows-msvc-release -Expect .zip
```

```sh
# Linux, the command-line solver (no Qt needed)
cmake --preset linux-gcc-release -DWJS_BUILD_APP=OFF
cmake --build --preset linux-gcc-release
ctest --preset linux-gcc-release
cpack --preset package-linux-gcc-release                  # every generator the configure chose
tools/release/smoke_package.sh build/linux-gcc-release/packages/*.{zip,tar.gz,deb,rpm}

# Linux, the application, as an AppImage (QT_ROOT_DIR names a Qt 6.12 kit)
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
libs/platforms/linux/packaging/build_appimage.sh --build build/linux-gcc-release   --version 0.1.0 --out build/linux-gcc-release/packages --qt-prefix "$QT_ROOT_DIR" --smoke

# macOS, the application bundle in a disk image
cmake --preset macos-clang-release
cmake --build --preset macos-clang-release
cpack --preset package-macos-clang-release
libs/platforms/macos/packaging/verify_bundle.sh build/macos-clang-release/packages/*.dmg --smoke
```

The Release presets turn the application on, Tracy and the sanitizers off, and
require reviewed phonics data (`WJS_REQUIRE_REVIEWED_PHONICS`; it needs `pwsh`).
`check_release_build.ps1` fails if the profiler or the sanitizers are on in the
cache or a package holds either, and `smoke_package.sh`
opens each format a user would (ZIP, tar.gz, deb, rpm, a mounted disk image)
and solves a board with the dictionary it carries. For the NSIS installer,
install NSIS before configuring (`makensis` is found at configure time), then
`cpack -G NSIS --config build/windows-msvc-release/CPackConfig.cmake`.

The SBOM of a release directory:

```sh
pwsh tools/release/generate_sbom.ps1 -Tag v0.1.0 -PackagesDir dist -Output dist/WordJewelsSolver-0.1.0.cdx.json
```

The checksums and signatures are made by the release workflow with the organisation's key; the
author's set-up is in [Releasing](releasing.md#what-the-author-sets-up-once), and users check a download as
in [Releasing](releasing.md#checking-a-download). Packages are
named after the project version (`0.1.0`), and a prerelease's files carry its
identifier (`-DWJS_PACKAGE_PRERELEASE=rc.1`, which `release.yml` passes from a
`v0.1.0-rc.1` tag: `...-0.1.0-rc.1-...`, a deb `0.1.0~rc.1`, an rpm release `0.rc.1`),
so a final release's files are never named like a rehearsal's.

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
dependencies, and two optional features:

| Feature | Packages | When |
| --- | --- | --- |
| `tests` | `catch2` (BSL-1.0) | Whenever `WJS_BUILD_TESTS` is `ON`; CMake selects it before `project()` |
| `profiling` | `tracy` (the client, with `on-demand`) and, for the host, `tracy`'s command-line tools (`tracy-capture`, `tracy-csvexport`) | Whenever `WJS_ENABLE_TRACY` is `ON`: the Debug presets (the coverage and analysis ones turn it off) and the profiling presets. `tracy` is overridden to one version and built from the repository's overlay port, without ISA extensions ([Profiling](profiling.md)) |

On Linux the Tracy tools are built from `dbus` upward, which needs `autoconf`,
`autoconf-archive`, `automake`, `libtool`, `libltdl-dev` and `python3-venv` installed
first (CI's `linux-tracy-tools` action does it); without them pass
`-DWJS_ENABLE_TRACY=OFF`.

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
  The code is also built with `/O1` (no `/Od`, no `/Ob0`): see `WJS_SANITIZER_OPTIMIZE`.
- clang-cl: disabled.

Release builds are not instrumented. `-DWJS_ENABLE_SANITIZERS=OFF` turns it all
off for a Debug build. `linux-clang-debug` is the same flags with `clang++`, and
is what the nightly Clang sanitizer leg builds ([CI](ci.md#nightly-analysis)).

ThreadSanitizer is a separate runtime and cannot be combined with those, so it has
its own preset, `linux-clang-tsan` (`WJS_ENABLE_TSAN`, built at `-O1` with frame
pointers, Tracy off). It builds the solver, the scanner and the command line and
runs their unit, integration and system tests, with `halt_on_error` and the
suppressions in `tools/sanitizers/tsan.supp` (a reason and an issue for each
entry). The Qt application is not built: Qt's prebuilt libraries are not
instrumented, and TSan would report races in them that are not there.

```sh
cmake --preset linux-clang-tsan
cmake --build --preset linux-clang-tsan
ctest --preset linux-clang-tsan
```

## Shared CMake modules

`cmake/icl/` is a copy of the modules in `icl-common-infra` (warnings with a
helper for generated code, hardening flags, sanitizers, Tracy, coverage, the
vcpkg and Qt finders, notices and the Qt application install). It is not edited
here: a change goes to `icl-common-infra`, and the copy follows the commit that
`.github/workflows/ci.yml` pins.

```sh
python3 tools/ci/repin.py --infra ../icl-common-infra --version vX.Y.Z   # move the pins and the copy
python3 tools/ci/sync_cmake.py --infra ../icl-common-infra --check       # what CI runs
```

The build includes a module only when this project has adopted it, one at a
time. The ones it still has of its own are in `cmake/`.

## Static analysis

`clang-tidy` reads the compile database the configure step writes
(`CMAKE_EXPORT_COMPILE_COMMANDS=ON` is a preset default), so it needs no build.
The `linux-gcc-analysis` preset is the lean configure CI uses:

```sh
pip install -r .github/requirements/analysis.txt
cmake --preset linux-gcc-analysis
# run-clang-tidy.py on Linux; the wheel installs run-clang-tidy.exe on Windows.
run-clang-tidy.py -p build/linux-gcc-analysis -config-file "$PWD/.clang-tidy" \
  -warnings-as-errors '*'
```

`.clang-tidy` holds the check list and the reason each disabled check is out;
the pinned version is in `.github/requirements/analysis.txt`. The nightly job
runs the same command over every translation unit in the database.

The formatting job runs the repository's `.clang-format` with one pinned release
(`.github/requirements/lint.txt`, in a virtual environment: there is none on the PATH)
as `clang-format --dry-run --Werror` over every C++ file under `libs/`, `apps/` and `tests/`;
use the same release before a push.

MSVC's analyzer is part of the compile, under the `windows-msvc-analysis`
preset:

```sh
cmake --preset windows-msvc-analysis
cmake --build --preset windows-msvc-analysis
```

`/WX` turns a C6xxx finding into a failed build, and each translation unit
writes a `*.nativecodeanalysis.sarif` beside its object; the nightly job uploads
them to code scanning. A local configure does not upload anything. The option
is off in every other preset, so a normal build is unchanged.

## Coverage

`linux-gcc-coverage` is Debug with `WJS_ENABLE_COVERAGE=ON` and the sanitizers
off: it compiles the library, the platform layer and the tests with
`--coverage -fprofile-update=atomic -fno-inline`, so running the tests writes
gcov data (`.gcda` files beside the objects). `windows-clang-coverage` is the
same build with clang-cl and LLVM's source-based coverage
(`-fprofile-instr-generate -fcoverage-mapping`), read back with
`llvm-profdata`/`llvm-cov`; it uses the Visual Studio generator because the
ClangCL toolset is a multi-config one, so the build is
`cmake --build --preset windows-clang-coverage` and the tests need
`--config Debug` (`ctest --preset windows-clang-coverage`).

Both presets define the same targets:

```sh
cmake --build --preset linux-gcc-coverage --target coverage-unit
cmake --build --preset windows-clang-coverage --target coverage-unit
```

- `coverage-unit` runs the `unit` tier and gates `libs/core` at **90% of lines and
  80% of branches** (the project's minimum standard). Below either floor the
  build fails and names the report.
- `coverage-ui` runs the `ui` tier (the Qt Quick Test suites) and reports
  `apps/app/src`, the C++ the QML reaches; it has no floor.
- `coverage` runs both.

The reports are under `build/<preset>/coverage/<tier>/` (`summary.json`,
`report.txt`, `coverage.md` and HTML), and the same gate is CI's
`Coverage (Linux, GCC)` job. See [Testing](testing.md#coverage) for the scopes,
the thresholds and how a tier's counters are isolated.

## Platform notes

**Windows.** The word list and any file the programs open must be reachable by
a path of at most 260 characters (`MAX_PATH`). Opening a file by a longer path
fails unless the executable is long-path aware; this was found during testing
and is not fixed yet. Keep word lists at a short path.

**Windows: a camera reports no video input.** Windows allows only Medium
mandatory integrity and above to open a camera. A build tree made by a
low-trust (Low-integrity) session stamps its files Low, so the application
built and run from it sees no camera even though Windows and Media Foundation
do (the scan screen's "Try camera again" also stays empty). Raise the label,
as a normal shell can and a Low one cannot:

```powershell
icacls <build-directory> /setintegritylevel "(OI)(CI)M" /T
```

Rebuilds from the same low-trust session stamp the new outputs Low again, so
build from a normal shell, or re-run the command after each such build. The
application's own behaviour is unchanged either way: a camera it cannot see is
said so on the scan screen, and the typed board remains the way on.

**Linux.** `wjs-cli` finds its default word list through `/proc/self/exe`, so it
needs `/proc` mounted, as on any normal system.

**macOS.** The executable's own path comes from `_NSGetExecutablePath`, with
symlinks resolved. This code and the Linux code are built and tested by CI on every
push (the library, the platform layer, the command-line solver and the scanner, in
Debug with the sanitizers); the Qt Quick window has never been built for either desktop.

**iOS.** The same call finds the executable, which is the root of the flat bundle,
and the dictionary and dice are beside it; the data directory is
`Library/Application Support` in the application's own container. The unsigned build
compiles in CI; it has never run on a device ([iOS](#ios)).

**Android.** The program is a shared library inside a package, so there is no executable beside
the data: `paths_android.cpp` unpacks the dictionary and the dice, which are compiled into the
resources, into the application's private directory and answers with a path in it; the data
directory is that application's files directory. Built and packaged on Windows, not run on a
device ([Android](#android)).

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
- **The window is missing from the build**: `WJS_BUILD_APP=AUTO` found no Qt
  6.12 and built the command-line program and tests alone; the configure says so.
  Pass `-DCMAKE_PREFIX_PATH=<kit directory>` (the directory that holds
  `lib/cmake/Qt6`) or set `QT_ROOT_DIR`, then reconfigure. On Windows the labs'
  shared tree at `C:\Repos\common\qt\6.12.0\msvc2022_64` is searched
  automatically; `C:\Repos\common\env.ps1` also sets `QT_ROOT_DIR`. Pass
  `-DWJS_BUILD_APP=ON` to turn a missing kit into an error instead.
- **`ninja: ... not mentioned in its dyndep file`**, or a `ctest` that runs a stale
  binary: two builds of the same preset ran at once (two scripts, or an IDE and a
  terminal). Build one preset in one process at a time; separate presets are separate
  trees and can run side by side. A `cmake --build --preset` also needs its
  `cmake --preset` first ("not a directory").
- **Every compile fails with "Cannot open include file" on Windows, or a script ends
  in "The input line is too long"**: the compiler's environment is missing or was set
  up repeatedly. Run from a Developer PowerShell, or in a script dot-source Visual
  Studio's `Launch-VsDevShell.ps1 -Arch amd64 -SkipAutomaticLocation` **once per
  process** (every repeat grows `PATH` and `INCLUDE`); it can print a
  `vswhere.exe` warning and still work, and `vcvars64.bat` can fail on the same
  lookup.
- **vcpkg fails building Tracy's tools on Linux** (`dbus`, `libxcrypt`, `libsystemd`,
  missing `autoreconf`): the autotools are not installed, see
  [Dependencies](#dependencies); or turn Tracy off with `-DWJS_ENABLE_TRACY=OFF`.
- **"libs/core/bench is missing, as it is in a source archive"**: a `git archive` leaves the
  test tree out; configure with `-DWJS_BUILD_TESTS=OFF`.
- **A test fails to compile with an ambiguous symbol under MSVC**: a constant named
  like a `<cmath>` function (`tan`, `sin`, `log`) in test code. Rename it.
- **`cpack` fails with "Refusing to package this build"**: the tree has Tracy in it
  (a Debug or profiling preset). Package a Release preset; a development package is
  asked for by name, see [Packaging](#packaging).
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
