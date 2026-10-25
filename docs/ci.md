# CI, text policy and local hooks

This page is what runs on every change, what each job proves, and what keeps
the rules when GitHub cannot. The settings that sit above the workflows
(rulesets, required checks, the merge queue) are in
[Branch protection](branch-protection.md).

## What enforces what

The repository is public and its rulesets are applied ([Branch
protection](branch-protection.md)), so GitHub blocks a merge or a push that
breaks a rule. The same rules are also kept before a push, locally, by the
hooks in `tools/hooks`.

| Rule | Kept by | Also kept locally by |
| --- | --- | --- |
| Builds, tests, formatting, platform isolation | the `ci.yml` jobs, as required checks | `ctest` (includes the isolation and word-list checks) |
| Branch names | the Policy job, and the `branch-names` ruleset refuses the push | `tools/hooks/pre-push` |
| No attribution in commits and PR text | the Policy job | `tools/hooks/commit-msg` |
| Pull request required, no direct push, no force push | the `develop-protection` and `main-release-only` rulesets | - |
| Merge queue in front of `develop` | the `merge-queue-develop` ruleset | - |

Install the hooks once per clone with `git config core.hooksPath tools/hooks`.

## Workflows

| Workflow | Purpose | Runs on a private copy? |
| --- | --- | --- |
| `ci.yml` | the shared Policy job, `clang-format`, `platform-isolation`, the Windows / Linux / macOS build-and-test legs, a coverage report, the performance work budgets, ThreadSanitizer and a Release package check on Windows; which of them run depends on the event ([What runs when](#what-runs-when)); by hand, the profiling builds too | yes |
| `analysis.yml` | nightly, on `develop`: `clang-tidy`, MSVC `/analyze` and a Clang AddressSanitizer + UBSan build and test; no required checks | yes (the SARIF upload is skipped) |
| `release.yml` | on a `v*` tag: Release packages for Windows, Linux and macOS the ad-hoc iOS IPA and, when its signing secrets are set, the signed Android APK and AAB, source archive, SBOM, checksums, GPG signatures, attestations, a GitHub Release; by hand or weekly: the same without publishing ([Releases](#releases)) | yes (the build, test and signing steps; publishing needs a tag; the iOS job needs the Apple secrets, the Android job its own four) |
| `android-build.yml` | the debug-signed Android APK, built and looked inside, only when what Android reads changes; no secret, not required ([Android](#android)) | yes |
| `ios-build.yml` | an unsigned compile of the application for an iPhone/iPad, only when what iOS reads changes; no secret, not required ([iOS](#ios)) | yes |
| `docs.yml` | `mkdocs build --strict` of the documentation site, and its publication to GitHub Pages on every push to `main` that changes the docs ([Documentation site](#documentation-site)) | yes (the build; the publication needs Pages) |
| `release-tooling.yml` | lints the workflows (actionlint, ShellCheck) and self-tests the release scripts with a throw-away key; only when those files change (anything under `.github/`, `tools/release`, `tools/package`, a platform's `packaging/`, `tests/package`, the packaging CMake, the version files) | yes |
| `codeql.yml` | nightly CodeQL static analysis of the C++ | no - skipped |
| `dependency-review.yml` | fails a PR that adds a moderate-or-worse vulnerable dependency | no - skipped |
| `scorecard.yml` | OpenSSF Scorecard score and badge: on a push to `main`, weekly (Monday 04:00 UTC) and when branch protection changes | no - skipped |
| Dependabot (`.github/dependabot.yml`) | Actions pins, the pinned CI tooling, the vcpkg baseline | yes |

Every workflow declares `permissions: {}` at the top and gives each job only
what it names; every checkout sets `persist-credentials: false`; every
third-party action is pinned to a full commit SHA with its version in a
trailing comment; pull request data reaches scripts through `env:`, never by
interpolating `${{ }}` into a shell.

### Public-only workflows

`codeql.yml`, `dependency-review.yml` and `scorecard.yml` need a public
repository (or a paid plan). Each of their jobs carries
`if: ${{ !github.event.repository.private }}`, so they run now that the
repository is public and would be **skipped, not red**, if it were ever made
private again. `Scan dependency diff` is staged into the required checks after
its first report (see the staging table in
[Branch protection](branch-protection.md)).

The MSVC analyzer in `analysis.yml` runs on a private copy too; only its
`upload-sarif` step is skipped there, because uploading to code scanning needs a
public repository (or a paid plan), so its findings stay in the job log.

### Time limits

Every job has a `timeout-minutes`, so a hung step cannot hold a runner for
GitHub's default. The limits, by workflow:

| Workflow | Job (display name) | Limit, minutes |
| --- | --- | --- |
| `ci.yml` | `clang-format`, `platform-isolation` | 10 each |
| `ci.yml` | `Plan build legs` | 5 |
| `ci.yml` | the build-and-test legs: `Windows (MSVC)`, `Linux (GCC)`, `macOS (AppleClang)` and the dispatch-only profiling legs | **75** |
| `ci.yml` | `Performance (Linux, GCC)`, `Coverage (Linux, GCC)`, `ThreadSanitizer (Linux, Clang)` | 45 each |
| `ci.yml` | `Package (Windows, Release)` | 60 |
| `analysis.yml` | `clang-tidy` / `MSVC /analyze` / `Sanitizers (Linux, Clang)` | 30 / 60 / 45 |
| `codeql.yml` | `Analyze (C++)` | 60 |
| `release.yml` | `Prepare the release` / the four `Build ...` legs / `Build the iOS ad-hoc IPA` / `Build the signed Android packages` | 10 / 90 / 120 / 120 |
| `release.yml` | `SBOM, checksums and signatures` / `Attest provenance and SBOM` / `Publish the release` | 20 / 10 / 20 |
| `ios-build.yml` | `iOS (device, unsigned compile)` | 90 |
| `android-build.yml` | `Android (arm64-v8a, debug-signed APK)` | 90 |
| `docs.yml` | `Build the documentation (strict)`, `Publish to GitHub Pages` | 10 each |
| `release-tooling.yml` | `Lint the workflows and release scripts` / `Release tooling self-test` | 10 / 15 |
| `scorecard.yml` | `Analyze` | 15 |
| `dependency-review.yml` | `Scan dependency diff` | 10 |

The 75 minutes of the build-and-test legs is the Windows leg's: it builds the
application and its prototype with the sanitizers on and then runs about a
thousand tests, which takes about 40 minutes on a hosted runner; 45 minutes once
cancelled it in its last step.

Every `ctest` call in `ci.yml` also has `--timeout 900`, the default for a test
that sets none of its own, so one hung test fails within 15 minutes and is
named in the log, not found after the job limit.

## The jobs in `ci.yml`

The required checks are `CI Status` and `Scan dependency diff`. The other
job names are free to change, as `CI Status` waits for them by job id.

| Job (display name) | Runs on | What a green run proves |
| --- | --- | --- |
| `Policy` | ubuntu-24.04 | The shared static checks from `icl-common-infra` (`policy.yml`): branch name, commit and pull request text, preprocessor policy where a baseline exists, action pinning, the changelog fragment, the vcpkg baseline and REUSE where `REUSE.toml` exists. It runs every check even after one fails and lists them all. Not run by a manual dispatch. |
| `Vendored CMake modules` | ubuntu-24.04 | `cmake/icl/` is the copy of the modules in `icl-common-infra` at the commit `ci.yml` pins (`cmake/icl/SOURCE` names it). A bump of the pin fails here until `python3 tools/ci/repin.py --infra ../icl-common-infra --version vX.Y.Z` has been run. Not run by a manual dispatch. |
| `clang-format` | ubuntu-24.04 | Every `.cpp`/`.hpp` (and sibling extensions) under `libs/`, `apps/` and `tests/` is exactly what the repository's `.clang-format` produces, with one pinned clang-format release (`.github/requirements/lint.txt`). Any difference fails; the log names file and line. |
| `platform-isolation` | ubuntu-24.04 | `tools/checks/check_platform_isolation.ps1`: no preprocessor conditional, no OS header or OS-querying API outside `libs/platforms/`, no OS test in CMake outside the selector, no `#define`, every platform source wired. Fast, and deliberately not behind the build legs. |
| `Plan build legs` | ubuntu-24.04 | `tools/ci/plan.py` (its tests run first) reads the changed paths with the shared `plan_gate.py` and `.github/ci/plan.json` and decides which of the jobs below run ([What runs when](#what-runs-when)). An unknown leg name in a manual run fails here, with the list. The build, coverage, performance, package and TSan jobs wait for it. |
| `Windows (MSVC)` | windows-2025-vs2026 | Preset `windows-msvc-debug` configures, builds warning-free (`/W4 /WX`) with ASan, and all of ctest passes (one test per core on every leg; the cases that need the machine alone are `RUN_SERIAL`). Its objects come from a compiler cache ([below](#the-compiler-cache-windows)). The leg also builds the Qt application and the scene prototype, runs the application's offscreen smoke, and packages its Debug build as a **development** ZIP whose contents and packaged programs it verifies (the `wordjewelssolver-windows-x64-debug-dev` artifact: Tracy and the ASan runtime are in it by design; the package that ships is the `Package (Windows, Release)` job's). |
| `Linux (GCC)` | ubuntu-26.04 | Preset `linux-gcc-debug`: GCC 16 (`setup-gcc`; a pre-release snapshot until Ubuntu packages the release, which counts as meeting the floor) builds warning-free (`-Wall -Wextra -Wpedantic -Werror ...`) with ASan and UBSan, and all of ctest passes. The library, platform layer, CLI and scanner are built, not the Qt application (`-DWJS_BUILD_APP=OFF`). |
| `macOS (AppleClang)` | macos-26 (Apple silicon) | Preset `macos-clang-debug`: AppleClang from Xcode builds warning-free with ASan and UBSan, and all of ctest passes. As on Linux, without the Qt application. |
| `Coverage (Linux, GCC)` | ubuntu-26.04 | Preset `linux-gcc-coverage` builds and runs the unit tier, and gcovr over `libs/core` **fails the job below the project's minimum standard: 90% of lines and 80% of branches**. The HTML, `summary.json` and the table in the job summary are the `coverage-report` artifact. The same gate is `coverage-unit` locally ([Testing](testing.md#coverage)). |
| `Performance (Linux, GCC)` | ubuntu-26.04 | Preset `linux-gcc-release` (the one push leg that builds and tests a Release preset; `-DWJS_BUILD_APP=OFF`) builds warning-free and passes all of ctest, which in an optimised build includes the [work budgets](testing.md#performance-tests): visits, words and trie nodes over a fixed corpus, and a digest of every solution. The wall-time checks (`ctest --preset linux-gcc-perf`) run only in a manual run. In the merge queue and on a push; `CI Status` holds the run to it when the plan asked for it. |
| `Package (Windows, Release)` | windows-2025-vs2026 | Preset `windows-msvc-release` (the Qt application on, tests off, Tracy and the sanitizers off, the same Qt kit as the Windows leg) builds, then `tools/package/check_package.ps1` makes the ZIP and proves it carries no `TracyClient`, AddressSanitizer runtime or `.pdb`, unpacks it, solves a board with the packaged `wjs-cli` and runs the packaged window's offscreen smoke, and `tools/release/check_release_build.ps1` checks the cache and the package. The ZIP and `SHA256SUMS` are the `wordjewelssolver-windows-x64` artifact (14 days). It is the proof, in the merge queue and on a push, that a Release package is clean. |
| `ThreadSanitizer (Linux, Clang)` | ubuntu-26.04 | Preset `linux-clang-tsan` (`-O1`, Tracy off, the Qt application not built) builds the solver, the scanner and the command line and runs their unit, integration and system tests with `halt_on_error` and the suppressions in `tools/sanitizers/tsan.supp`. The image's Clang and its sanitizer runtime are used, as in `analysis.yml`. In the merge queue and on a push. |
| `CI Status` | ubuntu-24.04 | The aggregate of every job above. It fails when any of them failed, was cancelled, or was skipped although the plan asked for it. The rulesets require this job in place of the legs by name. A manual dispatch skips it. |

### What runs when

`Plan build legs` chooses from the event and the changed paths. `Policy`,
`Vendored CMake modules`, `clang-format` and `platform-isolation` always run,
drafts included.

| Event | Build and test | Also |
| --- | --- | --- |
| Draft pull request | none | |
| Pull request that changes only documentation, the changelog fragments, issue and PR templates, or files no build reads (`CODEOWNERS`, `dependabot.yml`, `REUSE.toml`, the hooks) | none | |
| Pull request that changes anything else | Linux (GCC) | Coverage |
| Pull request with the `deep` label | all three desktop legs | Coverage, performance, package, TSan |
| Merge queue entry, same rule for what changed | Linux and Windows; macOS too when the change touches `libs/platforms`, `cmake/`, the top-level CMake and manifest files or the workflows | Coverage, performance, package, TSan |
| Push to `develop` or `main`, same rule | all three desktop legs | Coverage, performance, package, TSan |
| Manual run | the legs named in `legs` (default `all`) | `tsan` is a leg name; the profiling legs run only by name |

A path that `.github/ci/plan.json` does not list, a diff that cannot be read
and a change to the workflows or `tools/ci` all count as "can alter the build".
Run `gh workflow run ci.yml --ref <branch>` (or add the `deep` label) before
queueing a change to threads, the standard library, platform code or CMake, so
the Windows and macOS legs are not first seen in the queue. A pull request
marked ready runs again with the full pull request rule.

What a build-and-test leg's ctest contains: the four tiers of
[Testing](testing.md#the-tiers) - the unit suite, the integration suite (the
scanner on every leg; the window's model and the Quick Test suites where Qt is
built), the system suite (`system.cli.*` runs `wjs-cli` itself and
`system.install` runs `cmake --install` into a throw-away prefix and executes
the installed solver on every leg; `system.app.*` runs the application's
start-up where the window is built) - and the repository checks, which carry
the `quality` label:

- `platform.fragments`: every platform fragment satisfies the platform
  contract, on whichever host this is, including that every interface header in
  `libs/platforms/interfaces` has an implementation in the fragment's sources.
- `platform.isolation`: the same script as the `platform-isolation` job; it
  needs `pwsh`, which every hosted image has.
- `profiling.shipped_presets` (no preset that ships has Tracy in it),
  `profiling.zone_names`, and `profiling.tracy_portable` (the repository's
  Tracy port passes `NO_ISA_EXTENSIONS`, so a cached Tracy tool runs on every
  runner: [The vcpkg binary cache](#the-vcpkg-binary-cache)).
- `profiling.perf_budgets_ran`: an optimised leg really ran the performance
  tier's work cases. They skip themselves in a Debug build, so the Debug legs
  prove only that the tier compiles on all three compilers; the `Performance`
  leg runs it.
- `analysis.config`: the nightly `analysis.yml` presets exist with their
  settings, and the CodeQL workflow is nightly.
- `toolchain.versions`: `ci.yml` and `analysis.yml` pin the CMake and Ninja of
  `.github/toolchain-versions.json`, `ci.yml` installs Qt as it says,
  `analysis.txt` pins its clang-tidy, and the vcpkg pins agree ([What CI
  pins](#what-ci-pins)). `toolchain.version` (with its
  self-test): `project(VERSION)`, `vcpkg.json` and the newest `CHANGELOG.md`
  release are one number.
- `notices.sections` (the generated third-party notices state every section a
  package must), `package.config` (CPack names this project, and the install
  rules carry the programs the build makes, their data and the notices),
  `privacy.sources`, `parent.wording` and `requirements.matrix` (each with a
  self-test of its own; the last holds [the requirements
  matrix](requirements.md) to the tests that exist).
- `data.sounds` (the checked-in feedback WAVs are what
  `tools/sounds/make_sounds.ps1` writes), `data.dice`, `data.wordlists`,
  `data.phonics` and its checker, and the phonics review gate.
- `release.tooling`: `tests/package/test_release_tools.sh`, the tag checks, the
  signing and verification with a throw-away key and the SBOM, and that
  `release.yml` and the iOS workflow keep the toolchain pins. It needs bash,
  gpg, jq, git and `pwsh`, and reports itself skipped (exit 77) without them;
  `release-tooling.yml` runs it where they are.

Two system cases, `profiling.trace_cli` and `profiling.trace_app`, run the
programs with Tracy's capture tool attached and read the trace
([Profiling](profiling.md)). They need a build with Tracy and its tools, which
every Debug leg has, and report themselves skipped otherwise; a capture tool
that will not run on the machine is a failure that says so.

The **Windows leg**, the one that builds the Qt application, also runs its
smoke mode after ctest (`WordJewelsSolver.exe --smoke 60`, offscreen with the
OpenGL RHI backend): the window, its screens and the game start up and present
frames, and the run fails if they do not (docs/profiling.md, "The smoke mode").
Quick 3D cannot render offscreen, so this is the start-up path and the 2D graph;
the frame budget is a desktop check.

That leg then **packages what it built** and proves the package is usable:
`tools/package/check_package.ps1` (docs/building.md, "Checking a package") runs
`cpack -G ZIP`, checks the archive carries the two programs, Qt's DLLs, plugins
and QML modules (Quick 3D included, and the multimedia plugin and
`qml/QtMultimedia` the camera's live view needs), the dictionary, the dice, `NOTICES.txt` and
`LICENSE.txt`, writes `SHA256SUMS`, unpacks it, solves a board with the packaged
`wjs-cli` and its installed dictionary, and runs the packaged
`WordJewelsSolver --smoke 10` offscreen. The ZIP and `SHA256SUMS` are uploaded
as the `wordjewelssolver-windows-x64-debug-dev` artifact. This is the Debug
build, so the package is a development one with `TracyClient.dll` and the ASan
runtime in it, and the script tells CPack so (`-ForbidTracy Auto`; CPack refuses
a Tracy build otherwise, [Building](building.md#packaging)). The package that
ships is the Release one, checked by `Package (Windows, Release)`. The script is
the same one a developer runs, so a broken install rule is red here on every
push rather than at release time.

That leg also carries the **interface's languages**: each of the eight
catalogues is checked for completeness, the QML screens are loaded in all eight
(including Hebrew and Yiddish, with the layout mirrored), the glyph check runs
on the platform's fonts, and `i18n.catalogues` regenerates the catalogues with
the kit's `lupdate` and fails when a checked-in one differs. The last is the
drift gate: a `qsTr()` added without regenerating `apps/app/translations/*.ts` is a red leg, not
a silent untranslated string. It is a ctest, so it skips itself (exit 4) on a
machine without Qt's Linguist tools, but the Windows leg installs them (below).

The same leg runs **the interactive QML at run time**: the `quick.*` suites, one per `apps/app/ui-tests/tst_*.qml`
([Testing](testing.md#the-interactive-qml-qt-quick-test)), start
`wjs_quick_tests` once each under `QT_QPA_PLATFORM=offscreen`, with Qt's
software renderer and no display, and a Qt warning fails the suite. They are
ordinary ctest entries (label `gui`), so they are part of the leg's `ctest`
like every other test: there is nothing extra to run. Their target's
`windeployqt` step deploys the `QtTest` QML module and the modules the suites
import beside the binary, as the application's own deploy does for its screens.

### Toolchain choices

- **No compiler install.** The code needs `<expected>` and `<format>`, which
  libstdc++ has from GCC 13 and Apple's libc++ has in current Xcode, so each
  leg uses the compiler its image ships. The `Show the toolchain` step puts
  the exact version in the log.
- **Debug presets.** The three build-and-test legs build the Debug presets
  because they carry the sanitizers (and Tracy, which a Debug build has on by
  default: [Profiling](profiling.md)). Release builds come from the
  `Performance` leg (Linux, GCC), `Package (Windows, Release)` and `release.yml`;
  a Release-only warning on macOS (`-Wnull-dereference`, for one) would first
  show up in a release rehearsal or locally.
- **Qt on the Windows leg.** The Windows leg installs the prebuilt Qt 6.12.0
  kit with `jurplel/install-qt-action` (pinned to v4.4.1 by commit SHA) and
  configures with `-DWJS_BUILD_APP=ON -DWJS_BUILD_SHAKE_PROTOTYPE=ON`, so the
  Qt Quick application, its 3D scene prototype, the app tests, `parent.screens`,
  `scan.screens`, `camera.screens`, `parent.languages`, `scan.languages`, the `quick.*`
  suites, `i18n.catalogues`, `app.scene_contract`, `app.qml_lint` and `quick.lint` all run on
  every push. The install names the modules `qtquick3d qtshadertools qtmultimedia qtsensors
  qtspeech` (see `aqt list-qt windows desktop --modules 6.12.0
  win64_msvc2022_64`): Quick 3D and its shader tools are modules of their own
  in the 6.12 repository, and a kit without them fails the configure on
  `Qt6Quick3D`; the
  base kit itself carries `lupdate`, `lrelease` and Qt Linguist (from 6.12
  `qttools` is an archive of the base package, and naming it as a module makes
  aqtinstall fail), and the modules are the application's sound, camera, motion and speech, so this is
  the leg that proves the real engines rather than their silent variants. The
  action's `aqtsource` input installs aqtinstall from a git commit rather than
  from PyPI, because Qt split the Windows repository per architecture from 6.11
  (`qt6_6120/qt6_6120_msvc2022_64/` instead of `qt6_6120/qt6_6120/`) and
  aqtinstall 3.3.0 — the last PyPI release — cannot read it: every 6.12.0
  install on Windows failed with "Failed to locate XML data" on a doubled
  `qt6_6120/qt6_6120/Updates.xml` path. aqtinstall PR #1000 fixed that on master
  and no release has carried it since, so the pin is the master commit that was
  installed and verified (2026-10-13, the same one the sibling `iclforge`
  project uses). Drop the key, which makes the action fall back to PyPI, once a
  release newer than 3.3.0 includes the fix. `cache: true` keeps the kit
  between runs. `Package (Windows, Release)` installs the same kit. The other push
  legs build the library, platform layer, CLI and scanner alone
  (`-DWJS_BUILD_APP=OFF`) and install no Qt: the desktop application has been
  built only on Windows. `release.yml` installs the Linux and macOS kits
  (`qt.kits` in `.github/toolchain-versions.json`) for its first builds of it
  there, and the iOS jobs install Qt's iOS kit; of those only the unsigned iOS
  compile has run ([below](#not-verified-until-it-runs-on-github)).
  `QT_QPA_PLATFORM=offscreen` lets the screens run on a runner with no display.
- **The feedback and camera modules are optional.** `apps/app/src` finds Multimedia,
  Sensors and TextToSpeech with `OPTIONAL_COMPONENTS` and selects a silent
  backend for a missing one (the variant pattern), so a contributor's kit still
  configures and builds; the camera uses the same choice, with a `camera-none`
  backend when Multimedia is missing. The Windows CI leg installs them on
  purpose, so the package check can expect `Qt6Multimedia.dll`,
  `Qt6Sensors.dll` and `Qt6TextToSpeech.dll` beside the window, and the camera's
  `plugins/multimedia` and `qml/QtMultimedia` in its package.
- **A compiler cache on the two Windows MSVC jobs** ([The compiler
  cache](#the-compiler-cache-windows)).
- **CMake 4.4.2 and Ninja 1.13.2** from `lukka/get-cmake`; the same pins the
  sibling projects use.
- **One place for the pins.** `.github/toolchain-versions.json` lists CMake,
  Ninja, clang-tidy, the Qt version, architecture, modules, the `install-qt-action` pin and
  the `aqtsource` commit, and names the vcpkg pin file. The workflows are held
  to it: the `toolchain.versions` ctest fails if a workflow pins another version,
  if the `install-qt-action` step stops naming the file's action, version,
  architecture, modules or aqtsource, if it stops asking for `cache: true`, if
  the old PyPI `aqtinstall==` line comes back, or if `vcpkg.json`'s
  `builtin-baseline` is not the commit in `cmake/vcpkg-commit.txt`. The two
  vcpkg pins move together ([The vcpkg binary cache](#the-vcpkg-binary-cache)).
  The file also names a Qt kit per host (`qt.kits`: `win64_msvc2022_64` on
  Windows, `linux_gcc_64`, `clang_64` on macOS, and the iOS kit with its macOS
  host kit); `toolchain.versions` reads `ci.yml` and `analysis.yml`, and
  `release.tooling` (`tests/package/test_release_tools.sh`) holds `release.yml`,
  `ios-build.yml` and their kits to the same file.
- **Local composite actions are referenced as `./.github/actions/...`.** The
  sibling projects use the `$/.github/actions/...` form (resolved against the
  workflow's own commit, runner 2.336.0 or newer); `./` works on every runner
  and is understood by actionlint. Every job that uses one checks the
  repository out first.

### Running some legs by hand

A push, pull request or queue run always runs every leg. A manual dispatch can
name just the ones it needs - useful in a fork or private copy, where hosted
minutes are limited and macOS minutes are billed at a multiple:

```sh
gh workflow run ci.yml --ref <branch> -f legs=macos
gh workflow run ci.yml --ref <branch> -f legs=clang-format,linux
```

`legs` is comma-separated with no spaces: `clang-format`, `platform-isolation`,
`windows`, `linux`, `macos`, `coverage`, `perf`, `package` (`Package (Windows,
Release)`), or `all` (the default).
Three more legs exist only for a dispatch and are **not** part of `all`: the
profiling builds (`windows-msvc-profiling`, `linux-gcc-profiling`,
`macos-clang-profiling`: RelWithDebInfo with Tracy, [Profiling](profiling.md)),
named `profiling-windows`, `profiling-linux` and `profiling-macos`, or
`profiling` for all three. They configure, build warning-free against the real
Tracy headers, and run all of ctest, which includes the test that the real
variant of `wjs/profiling.hpp` is the one in use. A push does not pay for them
as well as for the debug legs, which build Tracy by default (a Debug build is
instrumented: [Profiling](profiling.md)): the client opens a network port, and
vcpkg builds Tracy's tools for the host in every Linux leg (there that is a
chain of ports that needs autoconf and Python's venv). An unknown name fails
the run rather than running nothing. Jobs of a dispatched run are named
`dispatch - <leg>`, deliberately not the names of the required checks: a run
that skips legs would otherwise report "skipped" (which counts as a pass)
under a required check's name on the branch's head commit.

A branch other than `main` and `develop` gets no CI from a push; open a pull
request, or dispatch the workflow on it.

A dispatch on a branch is **cold**: an Actions cache is readable from its own
ref, a pull request's base branch and the default branch (`main`), and a
dispatched run is none of the last two, so its first run installs Qt from the
network, builds Tracy's tools and compiles every object. Its second run on the
same branch restores what the first one saved, so to time a change, dispatch
twice and read the second (the numbers in "Where the time goes" are such pairs).
A merge-queue entry is cold in the same way (below).

### Where the time goes

Measured from the Actions API on the runs of 2026-10-20 and 21 (every job's and
every step's start and end), and again on this change's own runs. Minutes.

**A pull request run** is its longest leg plus the wait for runners. The legs,
before the speed-up (serial ctest, no compiler cache; the Windows numbers
include the vcpkg cache and Qt cache hits):

| Job | Run time | Where it went |
| --- | --- | --- |
| `Windows (MSVC)` | 31 to 59 | Qt 0.7, Configure 0.6-1.3 (warm; 14-19 cold), **Build 6.7-16**, **Test 21-37** (serial), smoke and package 1-2 |
| `Package (Windows, Release)` | 6.5 to 14 | Qt 0.6-1.6, **Build 4.6-10.5** |
| `macOS (AppleClang)` | 11 to 13 | Build 1, **Test 9-11** (serial) |
| `Linux (GCC)` | 10 to 13 | Build 1-1.6, **Test 8-11** (serial) |
| `Coverage`, `Performance` | 2 to 4 | unit tier 1.5, Test 1.2 |
| `clang-format`, `platform-isolation`, `Plan build legs` | under 0.3 | |

and after (the second dispatch of the branch, `legs=all`, so with the caches a
pull request restores from develop; all 1129 tests of the Windows leg and all
767 of the Linux and macOS legs pass):

| Job | Run time | Build | Test |
| --- | --- | --- | --- |
| `Windows (MSVC)` | 24.2, on the slower of the two hosted CPUs below | 2.1 (was 6.7-16) | 17.2 there, 8.9 on the faster CPU (was 21-37 serial; the `-j` is the previous change's) |
| `Package (Windows, Release)` | 3.7 (was 6.5-14) | 1.0 (was 4.6-10.5) | none built |
| `Linux (GCC)` | 5.1 (was 10-13) | 1.1 | 3.4 (was 8-11) |
| `macOS (AppleClang)` | 4.8 (was 11-13) | 0.6 | 3.5 (was 9-11) |
| `Coverage (Linux, GCC)` | 2.1 | 1.2 | unit tier 0.3 (was 1.5) |
| `Performance (Linux, GCC)` | 3.4 | 1.1 | 0.9 (was 1.2) |

The Windows leg on the faster CPU, with every cache cold, was 27.7 minutes
(Configure 5.5, Build 10.1, Test 8.9); with the Configure and the Build of the
warm run it would be about 15.

**Hosted runners are not uniform.** The same Windows step took half as long on
one run as on the next: two runner image builds (with two MSVC builds) and two
CPU models (an EPYC 9V45 and an EPYC 7763) were handed out at the same time. The
1129 tests took 532 s on the first and 1030 s on the other, and a cold build 10.1
and 17.8 minutes. Compare runs by their `Total Test time` and by the CPU the job
printed, not by the job's total. A cache made under one compiler is not a hit
under the other (vcpkg's ABI hash and ccache both include the compiler), so a
change of image costs one cold run on it.

**The wait for a runner is the largest number there is.** In a busy hour a job
waited a median of 10 minutes (90th percentile 41-57) before it began, for a
three-second job as much as for a build, and the longest waits were 36 to 120
minutes. The build legs used to need the `plan` job's output, so they queued
twice: on the pull request run that raised this the Windows leg started 29
minutes after the run, behind a three-second job on `ubuntu-24.04`, and the
release package job, which has no dependency, started after 1.4. They now start
at once. The pool is the account's: this workflow's own parallelism (nine jobs
and a dozen steps for a run) competes with every other run in flight.

**What a push to `develop` repeats.** Its runs queue behind each other
(`cancel-in-progress` is off, a pending run is replaced by a newer one): one
waited 122 minutes behind another that itself took 59, and two in that window
were cancelled while pending. Each is the tree the merge queue (or the pull
request) already built, plus it stores the caches pull requests restore from;
that last part now happens in the first minutes (the vcpkg archives after
Configure, ccache after Build), so the rest of the run is verification. It is
kept, because a merge made with the administrators' bypass has no queue entry
and this is then the only run of the merged commit.

**A merge-queue entry is cold.** It reads none of develop's caches ("The vcpkg
binary cache"), so it installs Qt from the network (about 2 minutes), builds
Tracy's tools (4 minutes) and compiles every object (10 to 18 minutes). The Windows
job of a cold run was 27.7 minutes on the faster CPU and 47 on the slower one.

What changed, and what each was worth:

| Change | Measured |
| --- | --- |
| ctest runs one test per core on every leg (the Windows leg's was the previous change) | Linux Test 535-640 s to 201-249; macOS 602-680 to 212-274; coverage unit tier 88 to 19; performance 71 to 56 |
| ccache on the two Windows MSVC jobs | Windows Build 567-960 s to 115-126 (448 of 480 objects hit); Release package Build 599 s (a cold ccache run: 379-391) to 58 (212 of 232 hit) |
| Tracy's tools built without link-time optimisation | cold Tracy build 13 min (3.9 min now) on the hosted Windows runner, 13 to 4.4 on a 32-core machine locally; the cold Windows Configure from 14-19 min to 5.5 |
| The build legs no longer wait for `plan` | their start moves from after the `plan` job's wait to the run's own start: 25-30 minutes in the runs that raised it, nothing on a quiet hour |

What was looked at and not done:

- **A larger runner.** The organisation is on the free plan and the API answers
  "GitHub hosted runners are not supported for this organization" (public
  repositories are free on standard runners, which are 4 vCPU and 16 GB for
  Windows and Linux; larger ones need a paid plan and are billed per minute).
  The organisation's self-hosted runner groups are not used: on a public
  repository they would run code from pull requests.
- **Splitting the Windows suite across two runners.** Each shard would need
  Qt, the vcpkg archives, a configure and a build of its own (about 5 minutes
  warm) before it could run half of an 9-17 minute test step, and a build tree
  is too big to hand over as an artifact; there is nothing to gain, and two
  more jobs queue for a runner.
- **Precompiled headers and unity builds.** ccache cannot cache a compile that
  uses a precompiled header, and a unity build hides a warning's file and
  line; the 480 objects that miss on a change are, with ccache, the changed
  ones.
- **Skipping the push-to-`develop` run, or cancelling a running one.** Both
  would remove a check of the merged commit; see above.
- **Qt's archives, a blobless vcpkg clone, `apt`.** The Qt restore is 40 to 114
  seconds of the Windows job and a subset of the kit would save less than 30;
  the vcpkg clone is 11-19 seconds; `apt` is 11. None is on the critical path.
- **Defender.** Real-time protection is already off on the hosted Windows
  image, and `C:\` and `D:\` are excluded.
- **Merging the three small Ubuntu jobs.** Their names are required checks.

## Nightly analysis

Full static analysis is too slow to put in front of every push, so it runs once
a night in `analysis.yml` (02:17 UTC) and `codeql.yml` (02:47 UTC), and on
demand from the Actions tab. **None of these is a required check**: a red run is
a report for the next morning, not a blocked pull request.

| Job | Runs on | What it does |
| --- | --- | --- |
| `clang-tidy` | ubuntu-26.04 | Configures `linux-gcc-analysis` for the compile database only (Debug, no tests, no Qt application, Tracy and sanitizers off, so vcpkg installs nothing; C++ module scanning off because GCC 15's `-fdeps` flags are not clang-tidy 22 arguments) and runs the pinned clang-tidy (`.github/requirements/analysis.txt`) over it with `.clang-tidy`'s checks and **warnings as errors**. The database is checked to be non-empty first, so a misconfigured run cannot pass with nothing analysed. |
| `MSVC /analyze` | windows-2025-vs2026 | Preset `windows-msvc-analysis` builds the library, platform layer, scanner and CLI with `/analyze /analyze:external- /W4 /WX`. The C6xxx findings are errors, so a finding is a red build; the SARIF each translation unit writes goes to **Security → Code scanning** (category `msvc-analyze`). |
| `Sanitizers (Linux, Clang)` | ubuntu-26.04 | Preset `linux-clang-debug`: the image's `clang++` builds the tree with AddressSanitizer and UBSan, and the whole ctest suite runs under them. The push legs cover GCC, AppleClang and MSVC; this is the nightly Clang build. |
| `Analyze (C++)` | ubuntu-26.04 | `codeql.yml` runs CodeQL with the `security-extended` query suite over the lean library/platform/CLI build and uploads to code scanning. Public-only, as above. |

The push legs already run ASan and UBSan: `linux-gcc-debug` (GCC),
`macos-clang-debug` (AppleClang) and `windows-msvc-debug` (MSVC's ASan). The
nightly Clang leg is the fourth compiler, and the one that gives Clang's
sanitizer runtimes the whole ctest suite without waiting for a push.

### Running the same analysis locally

```sh
# clang-tidy: the pinned wheel and the compile database
pip install -r .github/requirements/analysis.txt
cmake --preset linux-gcc-analysis
run-clang-tidy.py -p build/linux-gcc-analysis -config-file "$PWD/.clang-tidy" \
  -warnings-as-errors '*'

# MSVC's analyzer: the SARIF is written beside the objects
cmake --preset windows-msvc-analysis
cmake --build --preset windows-msvc-analysis

# Clang with AddressSanitizer and UBSan
cmake --preset linux-clang-debug
cmake --build --preset linux-clang-debug
ctest --preset linux-clang-debug
```

`.clang-tidy` is deliberately explicit about the checks it does **not** run:
the new bounds checks, designated initializers, extra arithmetic parentheses,
ranges rewrites, the easily-swapped-parameter check and the enum-size suggestion
are disabled with the reason beside each in the file. The gate is the rest of
`bugprone-*`, `cppcoreguidelines-*`, `modernize-*`, `performance-*`,
`readability-*` and `clang-analyzer-*`, with warnings as errors. The pinned
version is the one the disabled list was written for; Dependabot's bump is the
moment to re-read it.

A scheduled run, and the workflow it runs, is read from the default branch,
`develop`. The two jobs that upload to code scanning (`MSVC /analyze`, CodeQL)
pass `ref` and the checked-out `sha`, so the alerts belong to the code that was
analysed. To analyse another branch, dispatch `analysis.yml` or `codeql.yml` on
it from the Actions tab.

Every Debug preset has Tracy on, so a Debug configure on Linux has vcpkg build
Tracy's tools for the host, which needs autotools. The composite action
`.github/actions/linux-tracy-tools` installs them: `ci.yml`'s Linux leg and the
nightly `Sanitizers (Linux, Clang)` job (`linux-clang-debug` is a Debug preset) use
it before Configure. The action is one `apt-get install`; the tools it makes
buildable are cached by the vcpkg binary cache, and built without ISA extensions
([The vcpkg binary cache](#the-vcpkg-binary-cache)). `Performance`, `Coverage`,
`Package (Windows, Release)`, `clang-tidy` and CodeQL do not need it: they
configure presets with Tracy off (`linux-gcc-release`, `linux-gcc-coverage`,
`windows-msvc-release`, `linux-gcc-analysis`), so there is nothing to install or
to cache, and CodeQL configures before its tracer starts regardless.

## Releases

`release.yml` turns a `v*` tag into a GitHub Release: Release packages built on
hosted runners, tested, listed in a software bill of materials, checksummed,
signed with the project's GPG key and attested. Nothing in it runs on a pull
request or the merge queue, and none of it is a required check.

### What a release holds

| Platform | Built on | Packages | Holds |
| --- | --- | --- | --- |
| Windows x64 | `windows-2025-vs2026`, MSVC, Qt 6.12.0 | `WordJewelsSolver-<v>-win64.zip`, `WordJewelsSolver-<v>-win64.exe` (NSIS installer) | the Qt Quick application and `wjs-cli`, with Qt's runtime, plugins and QML modules |
| Linux x64, the application | `ubuntu-26.04`, GCC, Qt 6.12.0 | `WordJewelsSolver-<v>-x86_64.AppImage` | the Qt Quick application with Qt inside it (linuxdeploy and its Qt plugin) |
| Linux x64, the solver | `ubuntu-26.04`, GCC | `.zip`, `.tar.gz`, `.deb`, `.rpm` | `wjs-cli` only |
| macOS arm64 | `macos-26`, AppleClang, Qt 6.12.0 | `WordJewelsSolver-<v>-Darwin.dmg` | the application bundle (Qt inside it) and `wjs-cli`; signed and notarised when the Apple secrets are set |
| iPhone / iPad (arm64) | `macos-26`, Xcode, Qt 6.12.0's iOS kit | `WordJewelsSolver-<v>-ios-adhoc.ipa`, `manifest.plist` | the application, ad-hoc signed for the devices in the provisioning profile; the manifest is the over-the-air install's ([iOS](#ios)) |
| Any | the tagged commit | `WordJewelsSolver-<v>-source.tar.gz` | `git archive` of the tag, so `.gitattributes`' `export-ignore` decides what stays out |

A prerelease's files carry its version (`...-0.1.0-rc.1-...`, a deb `0.1.0~rc.1`, an
rpm release `0.rc.1`): the tag's suffix reaches CPack as `WJS_PACKAGE_PRERELEASE`.

and, for the release as a whole:

| File | What it is |
| --- | --- |
| `SHA256SUMS` | SHA-256 of every file below, in `sha256sum` format |
| `WordJewelsSolver-<v>.cdx.json` | the CycloneDX 1.6 SBOM ([below](#the-sbom)) |
| `<file>.asc` | a detached GPG signature for every file above, `SHA256SUMS` included |
| `KEYS` | the public signing key (convenient, not a root of trust: check its fingerprint, [below](#checking-a-download)) |

Release builds have **Tracy off**: the Release presets turn it off
(`tests/profiling/check_presets.cmake` holds that), the workflow never sets it,
and every build job fails if `CMakeCache.txt` has it on or if a package that can
be listed (ZIP, tar.gz, deb) holds a file with Tracy or a sanitizer runtime in
its name.

**What is not built, and why.**

- **A deb or rpm of the application.** It needs a Qt 6.12 with Quick 3D that no
  distribution packages, so such a package could only declare dependencies nobody
  can satisfy (it used to ask for Qt 6.8). The application is the AppImage; the
  deb and rpm are the command-line solver's.
- **A ZIP of the macOS application, or a tarball of the Linux one.** The first
  holds an application that is not signed; the second has no Qt in it. CPack makes
  them (`check_package.ps1`-style checks do not need them) but the workflow does
  not publish them.
- **Android.** No platform section and no job in this tree
  (`cmake/Platform.cmake` refuses it): it is in progress on a separate branch
  and is not built here, so nothing on this page applies to it yet. [Decision
  D11](roadmap.md#4-decisions-that-gate-the-plan) makes 0.1 the three desktops
  and an ad-hoc iOS build for the author's own devices; Android, once it lands,
  is a private build for the author's own devices (D1). **iOS** is written, but
  only as an ad-hoc build for registered devices ([iOS](#ios)), and it has never
  run on a device; releasing to a public store is a separate licensing decision
  (D2).
- **Authenticode.** The Windows ZIP and installer are not Authenticode-signed
  (that needs a code-signing certificate), so SmartScreen warns on first run. The
  GPG signature and the attestation prove who built a file and that it is intact;
  they do not stop that warning.
- **macOS signing and notarisation need an Apple Developer ID**, which the
  repository does not hold. The release workflow signs, notarises and staples the
  disk image when six secrets exist (below), does nothing and says so when none
  does, and fails when only some do. Until then the disk image is unsigned and
  Gatekeeper warns.
- **Intel macOS and Linux arm64.** No hosted runner is in the matrix for them.

### The jobs

| Job | Runs on | What it does |
| --- | --- | --- |
| `Prepare the release` | ubuntu-24.04 | Decides the mode. For a tag: `tools/release/check_release_ref.sh` requires `vMAJOR.MINOR.PATCH[-prerelease]`, the same version in `CMakeLists.txt` and `vcpkg.json`, a `## [x.y.z]` section in `CHANGELOG.md` (the release notes) and, for a final release, that the commit is on `main`. A prerelease (`v0.1.0-rc.1`) may be cut from a branch and gets generated notes if it has no changelog section. |
| `Build ...` (4 legs: Windows, Linux solver, Linux application, macOS) | as the table | Configures the Release preset (the application on for the three legs that build it, so a missing Qt is an error and not a silent command-line-only package; Qt 6.12.0 from `install-qt-action`, the kit per host in `.github/toolchain-versions.json`), builds, runs **all** of ctest, then packages. The Windows and Linux application legs run the application's offscreen smoke on the build tree first. Windows also runs `tools/package/check_package.ps1` on the ZIP, and installs the NSIS installer silently and runs the installed `wjs-cli`. The Linux application leg makes the AppImage with `build_appimage.sh` and smokes it. Linux solver and macOS run `tools/release/smoke_package.sh` over every package they made; macOS then runs `verify_bundle.sh` (plist, camera text, Qt inside, no link to the build machine, offscreen start) and `sign_and_notarize.sh`. Every leg ends with `tools/release/check_release_build.ps1` (Tracy and sanitizers off, the expected packages present). |
| `Build the iOS ad-hoc IPA` | macos-26 | A tag, or a manual run unless its `ios` input is unticked ([iOS](#ios)). Requires the five Apple secrets, installs Qt's iOS and macOS kits, imports the certificate and profile, archives and exports the IPA with Xcode, checks it and writes `manifest.plist`. |
| `SBOM, checksums and signatures` | ubuntu-24.04 | Collects the packages, archives the source (`git archive`), writes the SBOM, writes `SHA256SUMS`, signs every file, then verifies the directory the way a user would. Uploads it as the `release-dist` artifact. |
| `Attest provenance and SBOM` | ubuntu-24.04 | Release only. `actions/attest-build-provenance` for every file, `actions/attest-sbom` tying the SBOM to the packages. This is the only job with `id-token: write` and `attestations: write`. |
| `Publish the release` | ubuntu-24.04 | Release only. Creates a **draft** with every file attached, publishes it, downloads it again and runs `verify_release.sh --attest` against the public release; if that fails the release is returned to a draft. The only job with `contents: write`. |

A tag-triggered run uses the workflow file **at the tagged commit**, so a release
is built by the pipeline that was reviewed with it.

### Rehearsals: the dry run

The same workflow, started by hand or weekly, builds, tests, packages, writes the
SBOM, signs and verifies - and publishes and attests nothing. The files are in
the run's `release-dist` artifact for 14 days.

```sh
gh workflow run release.yml --ref main                       # ephemeral key
gh workflow run release.yml --ref main -f signing=repository # the real secrets
```

- `signing=ephemeral` (the default, and what the weekly run uses) signs with a
  throw-away key made inside the run, so the pipeline is tested end to end
  without the real key and without secrets. `KEYS` in the artifact is that key's
  public half; check the artifact with
  `tools/release/verify_release.sh --dir <folder> --unpinned`.
- `signing=repository` signs with `GPG_PRIVATE_KEY` and `GPG_PASSPHRASE`, and
  checks the key against the committed fingerprint. Run it once after setting the
  secrets up and before the first tag: it is the test of the secrets.
- `ios` (default true, by hand only) also builds and signs the iOS IPA with the Apple
  secrets, which are real whatever `signing` says; untick it to rehearse the desktop
  packages alone. The weekly run never builds iOS ([iOS](#ios)).

The weekly run (Sunday 03:23 UTC) exists so that a Windows, Linux or macOS
Release build that has quietly stopped working is found on a Sunday and not on the
day of a release. Release builds are otherwise only exercised by this workflow
and the `Performance` leg (Linux).

**GitHub only starts a scheduled or manually dispatched workflow from the default
branch.** The default branch is `main`, and `release.yml` is not on it until
`develop` is merged there (the same is true of `analysis.yml`: `main` has neither
today). A tag push is the exception: it uses the workflow file at the tagged
commit, wherever that is.

### Cutting a release

1. In a release pull request (`release/<x.y.z>` into `main`): set `project(...
   VERSION x.y.z)` in `CMakeLists.txt` and `"version"` in `vcpkg.json`; give
   the version's section in `CHANGELOG.md` its date (`## [x.y.z] - <date>`; for 0.1.0
   the section `## [0.1.0] - unreleased` already exists, with a short `## [Unreleased]`
   above it) and, for a later release, rename `## [Unreleased]` and put a new empty
   one above it.
2. Optional, and worth doing for the first one: tag `v<x.y.z>-rc.1` on the release
   branch. It runs the real path - the real key, the attestations, a published
   prerelease - without making a final release. Delete the prerelease and its tag
   when done (a tag ruleset needs an admin to do it).
3. Merge to `main`, then tag the merge commit and push the tag:

   ```sh
   git tag -s v0.1.0 -m "WordJewelsSolver 0.1.0"   # or: git tag -a
   git push origin v0.1.0
   ```

4. Watch the run. When it is green the release is public and has been verified
   from the outside. A red `Publish the release` returns the release to a draft.

A packaging fix after a tag is a new tag (`v0.1.1`), not a moved one: the
`release-tags` ruleset refuses moving or deleting a `v*` tag except by an admin,
and a moved tag would make the SBOM, signatures and attestations of the first
one describe something that is no longer there.

### What the author sets up once

None of this is done by the repository, and the workflow stops, with the same
list in its message, if any of it is missing.

1. **Create the signing key** (done for the first release: fingerprint
   `9A0D96E8944899F94B1B37B51F1A8DA4C79B19D2`, expires 2028-10-19; the public half is `.github/release-signing/KEYS`
   and `FINGERPRINT`, and the two secrets are set). To make another, for a
   rotation, on your own machine from a checkout, in your own terminal (it prints
   no secret, and the output folder is outside the repository; the script refuses
   a folder in any git working tree):

   ```powershell
   ./tools/release/new_signing_key.ps1                       # -> ~/wjs-release-key
   ./tools/release/new_signing_key.ps1 -OutDir D:\keys\wjs -Email you@example.org -Install
   ```

   (`tools/release/new_signing_key.sh --out <dir> --email <addr> --install` is the
   same for a POSIX shell.) It uses a temporary keyring that never touches yours,
   makes an Ed25519 sign-only key (expires in two years, user id
   `WordJewelsSolver Release <address>`, the address from `git config user.email`
   unless given) protected by a random passphrase, and writes, into a folder only
   your account can read: `private-key.asc`, `public-key.asc`,
   `revocation-cert.asc`, `passphrase.txt`, `fingerprint.txt` and a `README.md`
   with the commands below. It then signs and verifies a test file with the
   exported files in fresh keyrings and shreds the temporary ones. `-Install`
   also copies the public key and fingerprint to `.github/release-signing/`.
   With Git for Windows' gpg (an MSYS build) the script hands gpg its home as
   `/c/...`, which that build's agent needs.
2. **Upload the two secrets** (done; repository secrets; `gh secret set` reads the
   value from standard input, so these print nothing of it):

   ```powershell
   Get-Content -Raw private-key.asc | gh secret set GPG_PRIVATE_KEY --repo iainchesworthlabs/wordjewelssolver
   Get-Content -Raw passphrase.txt  | gh secret set GPG_PASSPHRASE  --repo iainchesworthlabs/wordjewelssolver
   ```

   Then keep the private key and passphrase in your key store, the revocation
   certificate offline, and delete the first two from the output folder.
3. **Commit the public half** (done): `.github/release-signing/KEYS` and `FINGERPRINT`,
   and put the fingerprint where a reader will find it independently of the
   download: `SECURITY.md` and the README (both now carry it). The workflow refuses to sign with a key
   whose fingerprint is not the committed one.
4. **Apply the tag ruleset** (done, 2026-10-20; now `admin/apply.sh --only
   rulesets` in `icl-common-infra`, see [Branch protection](branch-protection.md)).
   It restricts `v*` tags to the owner and the release bot.
5. **Get the workflows onto `main`**: merge `develop` there. Until then nothing
   scheduled or dispatched from this repository exists on the default branch
   except `codeql.yml` and `scorecard.yml` (older copies).
6. **Pin the AppImage tools** (done: `linuxdeploy.pins` records the SHA-256 of
   linuxdeploy and of its Qt plugin, so a download that differs fails the build).
   To bump one, change the tag in its URL, set its hash to `PENDING` and run
   `libs/platforms/linux/packaging/pin_linuxdeploy.sh` on a machine you trust, then
   commit the result; while a hash is `PENDING` the build downloads by tag, prints
   the hash with a warning and carries on (trust on first use). The AppImage
   recipe itself has never run.
7. **Optional: macOS signing.** With an Apple Developer ID Application
   certificate, set the six secrets `MACOS_CERTIFICATE` (base64 of the `.p12`),
   `MACOS_CERTIFICATE_PASSWORD`, `MACOS_SIGN_IDENTITY` (`Developer ID Application:
   Name (TEAMID)`), `NOTARY_APPLE_ID`, `NOTARY_TEAM_ID` and `NOTARY_PASSWORD` (an
   app-specific password), e.g. `base64 -i cert.p12 | gh secret set MACOS_CERTIFICATE`.
   Without them the disk image is unsigned (the run says so in an annotation).
8. **iOS signing** (done 2026-10-20). Five repository secrets, from the Apple
   developer portal: `IOS_DIST_CERTIFICATE_BASE64` and `IOS_DIST_CERTIFICATE_PASSWORD`
   (the Apple Distribution certificate as a `.p12` with its private key, and its
   export password), `IOS_PROVISIONING_PROFILE_BASE64` (the ad-hoc profile for
   `org.iainchesworth.wordjewelssolver`), `APPLE_TEAM_ID` and
   `APPLE_PROVISIONING_PROFILE_NAME` (the profile's name exactly as the portal shows
   it). The profile lists the registered devices and expires 2027-07-16; a new device
   or a renewal means a new profile and a new secret
   ([Installing on my iPhone/iPad](ios-install.md#add-a-device)). The install link
   also needs the documentation site published ([Documentation site](#documentation-site)).
9. **Rehearse**: `gh workflow run release.yml --ref main -f signing=repository`,
   download the `release-dist` artifact and run
   `tools/release/verify_release.sh --dir <folder>` (it reads the committed
   fingerprint from your checkout).

Optional hardening, deliberately not required: create a `release` environment with
required reviewers and move the two secrets into it, then add `environment: release`
to the `sign` job, so that a tag push waits for a click before the key is used. A
dry run would then wait too, unless the dry run is split into a job of its own.

Rotating the key: run `new_signing_key.sh` again, replace the two secrets and the
files in `.github/release-signing/`, update the fingerprint in `SECURITY.md`, and
say so in the next release's notes. A revoked or expired key leaves the old
releases' signatures valid as of when they were made; users who verify them later
need the old `KEYS`, which stays in the repository history.

### iOS

The iPhone/iPad application is a private, ad-hoc build for the author's own devices
(decisions D1 and D11 in [the roadmap](roadmap.md#4-decisions-that-gate-the-plan)): not the App Store, not TestFlight.
Installing it, adding a device and renewing the signing are in
[Installing on my iPhone/iPad](ios-install.md). Here is how it is built.

**What is in the tree.** A platform section, `libs/platforms/ios/` (`platform.cmake`,
its paths implementation, `packaging/Info.plist.in`, the asset catalogue with the
icon and the launch colour, and `packaging/verify_ipa.sh`), selected by
`cmake/Platform.cmake` before `APPLE` (the one edit the isolation rules allow); and
the `ios-release` preset: Release, an Xcode project, `arm64-ios`, no tests, no
`wjs-cli`, Tracy and the sanitizers off like every Release preset. The haptics are
the desktops' shared no-op and the board reader the portable one, as the roadmap
has it; nothing buzzes yet.

**Two Qt kits.** A cross build needs Qt's iOS kit (static Qt) and a desktop Qt of
the same version for the host tools it runs (`moc`, `rcc`, `qmlcachegen`, `qsb`,
`lrelease`). `jurplel/install-qt-action` with `host: mac`, `target: ios`, `arch: ios`
installs both, the second through aqtinstall's `--autodesktop` with the same
modules, and the jobs name them `QT_ROOT_DIR` (`.../Qt/6.12.0/ios`) and
`QT_HOST_PATH` (`.../Qt/6.12.0/macos`), which the preset reads. Checked with the
pinned aqtinstall (`aqt list-qt mac ios`): Qt 6.12.0 has an `ios` architecture with
all five modules the application uses (`qtquick3d qtshadertools qtmultimedia
qtsensors qtspeech`), and the macOS desktop kit has them too. The versions are
`.github/toolchain-versions.json`'s (`qt.kits.ios`). The iOS kit is several
gigabytes, so the jobs do not cache it: a cache of that size would evict the
builds that run on every push.

**The deployment target is iOS 18.0** (`CMAKE_OSX_DEPLOYMENT_TARGET` in `ios-base`),
the minimum Qt's documentation gives for 6.12. With none set, Xcode would build for
the newest SDK and the app would refuse to install on anything older.

**`ios-build.yml`: the unsigned compile check.** `iOS (device, unsigned compile)`
configures and builds `ios-release` on `macos-26` and then looks inside the
`.app` with `verify_ipa.sh` (identifier, executable, version, icon, camera text, launch
screen, the data beside the executable, arm64, links only the system). It needs no
secret, and has run green on pull requests and on pushes to `develop` since
2026-10-21, in about a quarter of an hour. It runs on pull requests and pushes to `main`/`develop` that touch what iOS
reads (`libs/`, `apps/`, `cmake/`, `data/`, `assets/`, the root `CMakeLists.txt`,
presets and vcpkg manifest, the toolchain pins, the `vcpkg-setup` action, the workflow itself) and by hand
(`gh workflow run ios-build.yml --ref <branch>`). It is not a required check, for the
reason every path-filtered workflow here is not, and because a macOS runner is billed
at a multiple.

**`release.yml`, job `Build the iOS ad-hoc IPA`.** In order:

1. *Require the Apple signing secrets.* The five secrets, by name, before anything
   is downloaded: `IOS_DIST_CERTIFICATE_BASE64`, `IOS_DIST_CERTIFICATE_PASSWORD`,
   `IOS_PROVISIONING_PROFILE_BASE64`, `APPLE_TEAM_ID`, `APPLE_PROVISIONING_PROFILE_NAME`.
   Any missing is a red job with one message naming them and where they come from.
2. vcpkg (the presets' toolchain file chains to Qt's; the manifest has no dependency
   without the tests or Tracy, so nothing is built through it), CMake and Ninja at
   the pins, and the two Qt kits.
3. *Import the signing certificate and the provisioning profile.* A keychain made
   for the job (removed at the end, whatever happened), the `.p12` imported into
   it, the profile installed under its UUID. The profile is then held to the
   secrets: its name equals `APPLE_PROVISIONING_PROFILE_NAME`, its team is
   `APPLE_TEAM_ID`, its App ID is `<team>.org.iainchesworth.wordjewelssolver`, it lists
   devices (it is ad hoc) and has not expired. Only counts and names are printed.
4. *Configure (signed):* `cmake --preset ios-release` with
   `-DWJS_IOS_SIGNING_TEAM` and `-DWJS_IOS_SIGNING_PROFILE`, which sign the
   application target alone (the preset turns signing off for every other Xcode
   target: static libraries and Qt's resource targets have no bundle identifier
   and cannot be signed).
5. *Archive* (`xcodebuild archive`, scheme `WordJewelsSolver`) and *export*
   (`xcodebuild -exportArchive`, an `ExportOptions.plist` with method `ad-hoc`, the
   team, manual signing, `Apple Distribution` and the profile by name). The file is
   `WordJewelsSolver-<v>-ios-adhoc.ipa`.
6. *Check the IPA* with `verify_ipa.sh --signed --version <v>`: as the build check,
   plus the signature, the embedded profile, that it names devices, that it does not
   allow a debugger to attach and that it is for this bundle identifier.
7. *Write the manifest* with `tools/release/make_ios_manifest.sh`: `manifest.plist`,
   whose URL for the IPA is the release asset's, known from the tag before the release
   exists. Its tests are in `tests/package/test_release_tools.sh`.
8. Upload `package-ios`, and remove the keychain and the profile.

`SBOM, checksums and signatures` downloads `package-*`, so the IPA and the manifest are
in `dist/` with everything else and get the same `SHA256SUMS`, GPG signatures,
attestations (`attest` reads `SHA256SUMS`) and SBOM (an `.ipa` counts as carrying Qt,
like an AppImage and a disk image). `Publish the release` adds an install section to the
notes when there is a manifest.

**When it runs.** A `v*` tag builds iOS, so a release never lacks it by accident,
and never publishes unless the iOS job passed. A manual run builds it unless its
`ios` input is unticked. The weekly rehearsal does not: it would spend a macOS
runner and a signing identity every Sunday. The `sign` job tolerates a skipped iOS
job and refuses a failed one. If iOS is broken and a desktop release cannot wait, set
the repository variable `RELEASE_WITHOUT_IOS` to `true` (`gh variable set
RELEASE_WITHOUT_IOS --body true`): a tag then skips the job, with a warning, and the
notes have no iOS section. Delete the variable afterwards. Dispatch a rehearsal of the iOS job before the first
tag, because the first run is expected to fail somewhere
([below](#what-ios-has-and-has-not-shown)).

**The install page.** Safari installs an ad-hoc app from an `itms-services://` link
to an HTTPS manifest, and a release's notes cannot carry that link (GitHub strips the
scheme), so `docs/install-ios.html` builds it in the browser from `?tag=` (or finds the
newest release with a `manifest.plist`). MkDocs copies a non-Markdown file in `docs/`
into the site unchanged, so the page needs no nav entry and passes `mkdocs build
--strict`; it is deployed with the docs, so it exists only once
[the site is published](#documentation-site): a merge to `main` publishes it, so the
merge that precedes the first release makes the install link work. The release
workflow does not wait for it.

#### What iOS has and has not shown

Verified here, without a Mac: the platform fragment satisfies the contract
(`platform.fragments`), the isolation check passes with the new platform in the selector,
the presets' Tracy policy (`profiling.shipped_presets`) holds for `ios-release`, the
manifest generator and its refusals, the workflow files (actionlint with ShellCheck) and
the self-test of the pins (`test_release_tools.sh`: the five secrets, the kit, SHA pins,
no secret in the compile check, one bundle identifier, the camera text equal to
macOS's), `Info.plist.in` through `configure_file`, and the documentation build.
Qt 6.12.0's iOS kit and its five modules exist for the pinned aqtinstall.

Verified on GitHub by `ios-build.yml` (green on pull requests and on pushes to
`develop` since 2026-10-21): the configure (vcpkg's toolchain chaining to Qt's iOS
toolchain, `arm64-ios`, `find_package(Qt6)` with the host kit), the Xcode generate,
the compile and link against the static Qt kit with the warnings as errors, and the
unsigned checks of `verify_ipa.sh` on the bundle Xcode makes (identifier, executable,
version, icon, camera text, launch screen, the data beside the executable, arm64, links
only the system).

Not verified, because nothing has run it: **everything that signs, archives or runs on a
device.** Specifically, and in the order it is likely to fail:

- `xcodebuild archive` finding the application (it relies on the
  `install(TARGETS ... BUNDLE DESTINATION .)` rule), then `-exportArchive`. The method
  is spelled `ad-hoc`, the name Xcode deprecated for `release-testing`; both are
  accepted on recent Xcode, and if `ad-hoc` is not, that is the one-word fix;
- that the application's post-build copy of the dictionary and the dice lands before
  Xcode signs the bundle (the unsigned check proves the files are there, not that
  signing seals them);
- the keychain and profile handling (macOS tools that could not be run), the profile
  checks (PlistBuddy key paths), and `base64 --decode` on the runner;
- the signed checks of `verify_ipa.sh --signed` (the signature, the embedded profile,
  that it names devices and does not allow a debugger to attach);
- that Safari accepts the manifest at a GitHub release URL (it redirects to GitHub's
  asset host; CountdownSolver's flow did the same and installed), the install page,
  and the app starting on a device: Quick 3D on Metal, the shake, the camera and its
  permission prompt, the data directory in the container. **Nothing built for iOS has
  run on a device.**

The first signed run is expected to need fixes. A red `Build the iOS ad-hoc IPA` job
blocks a tag release (by design: a release should not quietly lack iOS), which is why
the rehearsal comes first, and why there is `RELEASE_WITHOUT_IOS`.

### Android

The Android application is built for tablets and phones that run Android 9 (API 28)
or later, in one architecture (arm64-v8a). Two packages come out of it: an **APK**, which
installs on a device that allows apps from outside the store, and an **AAB** (Android App
Bundle), which is what Google Play takes if a Play listing is ever made. Building it
locally is in [Building, Android](building.md#android); here is how CI and the release
make it.

**What is in the tree.** A platform section, `libs/platforms/android/` (`platform.cmake`,
the paths and fatal-error implementations, and `packaging/`: the manifest and resources
Qt packs, and `verify_apk.sh`), selected by `cmake/Platform.cmake` before `UNIX` (the NDK's
toolchain sets `UNIX` too); the presets `android-arm64-release` and
`android-arm64-debuggable`; `tools/release/new_android_keystore.sh`; and
`tests/platform/check_android.cmake`. The packaging is Qt's own: `qt_add_executable` makes
the application a shared library on Android and writes the deployment settings, and the
`apk` and `aab` targets run `androiddeployqt`, which gathers Qt's libraries, plugins and
the QML modules the application imports, and Gradle makes the package.

**Choices worth knowing.**

- *The data is unpacked, not installed beside an executable.* The word list and the dice
  are read from files by path, and nothing inside an APK is a file. `wjs_platform_configure_app`
  compiles them into the application's resources, and `paths_android.cpp` lays them down
  in the application's private directory once (a file that already matches is left
  alone) and answers `executable_path()` with where an executable would be in it, which
  is how the program finds them. These are the program's own read-only files, not the
  person's data; the privacy scan (`privacy.sources`) does not name this call, and this
  paragraph is where the exception is written down.
- *The manifest asks for the camera and nothing else.* Qt's core module asks for
  `INTERNET` and storage permissions at its manifest template's permissions
  placeholder, whatever the program uses, so the manifest leaves the placeholder out;
  `tests/platform/check_android.cmake` fails if it comes back or any other permission
  appears, `verify_apk.sh` checks the finished package (the merged manifest is what a
  device sees), and the privacy scan reads the source. The camera and the accelerometer
  are optional features, so a tablet without either still installs it. Android's backup
  is off, so the store is not copied to a cloud account.
- *No Tracy.* Its client listens on the network. The Android presets set it off, which is
  why the debuggable preset is not named `*-debug`: `tests/profiling/check_presets.cmake`
  reads that name as "Tracy on".
- *No haptics.* The only engine is the Vibrator service, which has no Qt API: it means
  JNI and the `VIBRATE` permission, a second permission this build does not ask for.
  `vibrate()` answers false, as on the desktops. The camera is Qt Multimedia's
  (shared with the desktops and iOS), the shake is Qt Sensors' accelerometer.
- *The QML debugger plugins are in the package.* Qt's Android dependency rules bundle the
  whole `plugins/qmltooling` directory with any program that has a QML module (13 files:
  the TCP debug server, the profilers, among them the Quick 3D profiler the Windows deploy
  leaves out). Excluding them through the plugin list does not work (`qt_import_plugins(...
  EXCLUDE_BY_TYPE qmltooling)` changes the list `androiddeployqt` is given and not what it
  copies) and the one way left is a replacement for Qt's `build.gradle` that tracks Qt's
  own. They are inert unless QML debugging is compiled into the program (`QT_QML_DEBUG` is
  not defined anywhere here), which has not been checked on a device. The licence is
  not the issue it is for a desktop package (the application is GPL-3.0-only); the open
  question is whether a release should carry a debug server at all.
- *The platform levels:* minimum 28 (what Qt 6.12 supports), target and compile 36 (Google
  Play's requirement is 35 or later). `versionCode` is derived from the version
  (0.1.0 is 10000); `-DWJS_ANDROID_VERSION_CODE=<n>` overrides it.
- *One application id,* `org.iainchesworth.wordjewelssolver`, the iOS bundle identifier.

**The toolchain.** A Qt for Android kit (arm64-v8a) and, for the host tools a cross build
runs (`moc`, `rcc`, `qmlcachegen`, `qsb`, `androiddeployqt`), a desktop Qt of the same
version; the Android SDK (platform 36, build-tools 36.0.0); the **NDK the kit was
built with** (r27c, `27.2.12479018`: the kit's own toolchain file names it); and a JDK 17
or later for Gradle. The pins are `.github/toolchain-versions.json`'s (`qt.kits.android`
and `android`); `tests/platform/check_android.cmake` holds both workflows to them.
`jurplel/install-qt-action` installs Qt with `host: all_os`, `target: android`,
`arch: android_arm64_v8a`: from Qt 6.7 the Android repository is `all_os`, not the
host's name (`aqt list-qt all_os android --arch 6.12.0` lists `android_arm64_v8a`, and
`aqt list-qt all_os android --modules 6.12.0 android_arm64_v8a` lists the five
modules the application uses, `qtquick3d qtshadertools qtmultimedia qtsensors qtspeech`),
and the action adds `--autodesktop`, which installs the Linux kit beside it (`gcc_64`).
`aqt list-qt linux android` finds nothing for 6.12 with the pinned aqtinstall. The kits are
several gigabytes, so the jobs do not cache them. The runner image already has the SDK, so
`sdkmanager` adds only the exact NDK, platform and build-tools; the JDK is the image's own
17 (`JAVA_HOME_17_X64`), so no action is added to the supply chain for it.

**`android-build.yml`: the debug-signed APK.** `Android (arm64-v8a, debug-signed APK)` runs
on `ubuntu-24.04`: the toolchain above, `cmake --preset android-arm64-debuggable`, the
build (which runs the `apk` target), then `verify_apk.sh` on what came out (the application
id and version, `minSdkVersion` 28 or later, `targetSdkVersion` 35 or later, the camera as the
only permission, the camera and accelerometer not required, `arm64-v8a` only, the program's
library inside, a launcher icon and activity, a valid signature) and an upload of
`WordJewelsSolver-<v>-android-arm64-debug-signed.apk` as `android-apk-debug-signed`
(14 days). Gradle signs it with Android's own debug key, so it installs on any device
that allows it and cannot be updated by a package signed with another key. It needs no secret and runs on
pull requests and pushes to `main`/`develop` that touch what Android reads, and by hand
(`gh workflow run android-build.yml --ref <branch>`). It is not a required check, for the
reason every path-filtered workflow here is not. There is no emulator job: a hosted
emulator needs hardware acceleration and an image download, neither reliable enough to
stand behind a green tick, and it would still not have a camera or an accelerometer worth
trusting.

**`release.yml`, job `Build the signed Android packages`.** The signing key is the
author's upload key, in four repository secrets. In order:

1. *Look for the Android signing secrets.* All four (`ANDROID_KEYSTORE_BASE64`,
   `ANDROID_KEYSTORE_PASSWORD`, `ANDROID_KEY_ALIAS`, `ANDROID_KEY_PASSWORD`) set: go on.
   None set: say so (a notice, not an error), do nothing else, and the release goes on
   **without an Android file**. Some but not all: fail with the missing names, since
   that is a mistake and not a choice. Nothing is built with a throw-away key: such a
   package could not be updated.
2. vcpkg, CMake and Ninja, the JDK, the SDK parts and the Qt kits, as above.
3. *Write the upload keystore* from the base64 secret into a file in the runner's temporary
   directory (`umask 077`); the file is removed at the end whatever happened.
4. *Configure* `android-arm64-release` with `-DQT_ANDROID_SIGN_APK=ON
   -DQT_ANDROID_SIGN_AAB=ON`, then *build* the `apk` and `aab` targets, with the keystore's
   path, alias and passwords in `QT_ANDROID_KEYSTORE_PATH`, `QT_ANDROID_KEYSTORE_ALIAS`,
   `QT_ANDROID_KEYSTORE_STORE_PASS` and `QT_ANDROID_KEYSTORE_KEY_PASS`, which
   `androiddeployqt` reads. The files are
   `WordJewelsSolver-<v>-android-arm64.apk` and `.aab`.
5. *Check the APK* with `verify_apk.sh --release --signed --version <v>`: as above, plus not
   debuggable and not signed with the debug key.
6. Upload `package-android`.

`SBOM, checksums and signatures` downloads `package-*`, so the APK and AAB are in `dist/` with
everything else and get the same `SHA256SUMS`, GPG signatures, attestations and SBOM entry (an
`.apk` and an `.aab` count as carrying Qt, like an AppImage). `Publish the release` adds an
install note when there is an APK.

**When it runs.** A `v*` tag and a manual run (unless its `android` input is unticked). The
weekly rehearsal does not (it would download several gigabytes of Qt kits every Sunday). The
`sign` job accepts a skipped Android job, which is also what a run without the secrets
looks like, and refuses a failed one.

#### What the author does once for Android

1. Make the upload keystore, on your own machine, outside the repository (it prints no
   secret; it refuses a folder inside a git working tree; it needs `keytool`, which any JDK has):

   ```sh
   tools/release/new_android_keystore.sh        # ~/wjs-android-keystore by default
   ```

   It writes the keystore, its password, the alias and the certificate's SHA-256 into the
   folder, opens the keystore again with the password it wrote, and prints the four commands:

   ```sh
   gh secret set ANDROID_KEYSTORE_BASE64   --repo iainchesworthlabs/wordjewelssolver < <folder>/upload-keystore.p12.base64
   gh secret set ANDROID_KEYSTORE_PASSWORD --repo iainchesworthlabs/wordjewelssolver < <folder>/store-password.txt
   gh secret set ANDROID_KEY_ALIAS         --repo iainchesworthlabs/wordjewelssolver < <folder>/key-alias.txt
   gh secret set ANDROID_KEY_PASSWORD      --repo iainchesworthlabs/wordjewelssolver < <folder>/key-password.txt
   ```

   (The key's password is the keystore's: a PKCS12 store has one.) Keep a copy of the keystore and
   the password in a password manager, then delete the files. **A package signed with this key can be
   updated only by a package signed with it**: losing the keystore ends updates to installed copies
   (an upload key registered with Google Play can be replaced by Google on request; a key that
   signed an APK people installed from GitHub cannot).
2. Dispatch `android-build.yml` (and then a `release.yml` rehearsal) before the first tag.
3. Put the APK on a tablet and try it (below).

#### What Android has and has not shown

Verified here, on a Windows machine with the Android SDK, NDK 28.2 (not the r27c the kit was
built with), JDK 21 and the author's Qt 6.12.0 Android kit from the shared tree: the platform
fragment satisfies the contract (`platform.fragments`); the isolation, privacy, toolchain-pin and
Tracy-policy checks pass with the new platform and presets; `platform.apk` (the Android target's checks) and its
self-test; `android-arm64-debuggable` and `android-arm64-release` **configure, compile, link and
package** (the debug-signed APK is 107 MB, the release one 94 MB, the AAB 39 MB), and
`verify_apk.sh` passes on the signed release APK; a signed release build was made with a
throw-away keystore (made by `new_android_keystore.sh`, which ran, and deleted afterwards) to
prove the signing path (`apksigner` and `jarsigner` verify the signatures; the signer is not the
debug key); the manifest in the finished package asks for the camera and nothing else; the
workflow files (actionlint with ShellCheck) and `test_release_tools.sh`.

Not verified, because nothing here has an Android device, and the author's kit has none of the
three optional modules:

- **Anything that runs on a device.** The program has never started on Android. Whether the
  window comes up (Quick 3D on the device's GPU, `QtQuick.Controls` styles and the fonts),
  the data unpacking and the store in the private directory, rotation, the camera's
  permission prompt and live view, the shake, the speech engine, sound, the launcher icon and
  theme, and the settings it shows under *App info*: all unseen. `verify_apk.sh` says what is
  in the package, not that it runs.
- **Qt Multimedia, Qt Sensors and Qt TextToSpeech on Android.** The shared kit was built without
  them, so the local build used the silent camera, motion and speech variants. The real
  backends (`camera-multimedia`, `motion-sensors`, `speech-texttospeech`) compile against
  Android only in CI, and `QtMultimedia`'s QML module has to arrive in the package through the
  import scan: this is the first place either happens.
- **The CI job itself**: aqtinstall resolving `all_os` with `--autodesktop` to
  `.../Qt/6.12.0/android_arm64_v8a` and `.../gcc_64`, `sdkmanager` installing NDK 27.2.12479018
  (the build here used 28.2), the runner's `JAVA_HOME_17_X64`, Gradle downloading its
  distribution and AndroidX in CI, the Ninja build on Linux, and the release job's signing steps
  with the real secrets (the local signing run set the same `QT_ANDROID_KEYSTORE_*`
  variables, not the workflow around them).
- Google Play: nothing was tried, and a listing needs a developer account, a store entry and
  the privacy declarations (docs/design/application.md, "Privacy, built in and checked").

On a device, try in this order: install (`adb install -r WordJewelsSolver-<v>-android-arm64-debug-signed.apk`,
or open the file on the tablet), the window and Home, Solo Play (shake, the box, the path), the
camera prompt and Mode A, rotation and split-screen, the parent area and that its progress
survives closing the app from the recents list, the settings *App info → Permissions* (the camera
alone, no network access listed), and uninstall.

### The SBOM

`tools/release/generate_sbom.ps1` writes a CycloneDX 1.6 document from what the
repository already records, not from a scan of the binaries:

- the application (GPL-3.0-only: it links Qt Quick 3D) and `wjs-cli`
  (GPL-3.0-or-later), with the commit and the repository;
- Qt by module at the version in `.github/toolchain-versions.json`, each under the
  licence it is used under - **only if a ZIP in the release actually carries
  `Qt6Core.dll`, or a package is an AppImage, a disk image or an iOS IPA** (which
  carry Qt by construction); likewise the Microsoft Visual C++ runtime, from a
  ZIP that carries `vcruntime140.dll`. The command-line
  packages claim no Qt;
- the data that ships: the ENABLE word list and each bundled font file, with
  SHA-256 hashes computed from the files in the tree;
- what only built or tested it, with `scope: excluded`: vcpkg (its commit), CMake,
  Ninja and every port vcpkg installed, read from the `vcpkg.spdx.json` files
  vcpkg writes (Catch2 today);
- each release file as a `distribution` reference with its SHA-256.

It states that it is incomplete for third parties (`incomplete_third_party_only`):
Qt contains third-party code of its own, which the SPDX documents of the Qt kit
(`sbom/*.spdx.json`) describe and this document does not repeat. The output is
deterministic for a given tag and commit. `release-tooling.yml` validates a
generated SBOM against the CycloneDX 1.6 JSON Schema.

### Checking a download

```sh
# 1. The key. Check the fingerprint against SECURITY.md in the repository,
#    not against the KEYS file from the same place as the download:
gpg --show-keys --with-fingerprint KEYS

# 2. Everything at once (needs gpg; add --attest with the GitHub CLI):
tools/release/verify_release.sh --dir <folder with the files> \
  --fingerprint <the fingerprint> [--attest --repo iainchesworthlabs/wordjewelssolver]
# Windows: ./tools/release/verify_release.ps1 -Dir <folder> -Fingerprint <...> [-Attest -Repo ...]

# 3. Or by hand:
gpg --import KEYS
gpg --verify SHA256SUMS.asc SHA256SUMS
sha256sum -c SHA256SUMS            # or: shasum -a 256 -c SHA256SUMS
gh attestation verify WordJewelsSolver-0.1.0-win64.zip --repo iainchesworthlabs/wordjewelssolver
```

`verify_release.sh` uses a private temporary keyring, stops at the first failure,
refuses to run without an expected fingerprint (`--unpinned` skips that, and says
so), and takes `--repo <owner/name> --tag <tag>` instead of `--dir` to download
the release itself.

### When a release run goes red

| Message | Cause |
| --- | --- |
| `the tag ... is version X but CMakeLists.txt declares Y` | The release pull request did not change `project(VERSION)` / `vcpkg.json`, or the tag is on the wrong commit. Delete the tag (admin), fix, tag again. |
| `CHANGELOG.md has no '## [x.y.z]' section` | Rename `[Unreleased]` in the release pull request. |
| `... is not on origin/main` | A final release is tagged on `main`. For a try-out, use a `-rc` tag. |
| `Release signing secrets missing` | Step one and two of "What the author sets up once". |
| `The secret key's fingerprint is A but .github/release-signing/FINGERPRINT says B` | The wrong key was uploaded, or the public half was not committed. |
| `GPG_PASSPHRASE does not unlock the key` | `passphrase.txt` is not the passphrase that key was made with. |
| `WJS_ENABLE_TRACY is 'ON'` / `contains the profiler` | A Release configuration or package has Tracy in it. The presets decide; `tests/profiling/check_presets.cmake` should have failed first. |
| `no package ending .rpm` (and similar) | CPack dropped a generator because its tool was missing at configure time. The packaging tools are installed before Configure; read the configure log's `Packaging:` line. |
| `makensis.exe is not in ...` | The NSIS install failed. NSIS is pinned to 3.13.0 from Chocolatey. |

### Not verified until it runs on GitHub

Run locally on Windows: the Release build, the whole ctest suite and the ZIP package
check (and its refusal of TracyClient, an AddressSanitizer runtime and a .pdb), the
executable's version resource, the prerelease naming and the deb on Linux (WSL),
CPack's refusal of a Tracy build, the SBOM against the CycloneDX schema,
`sign_release.sh` / `verify_release.sh` and the key script. Run on GitHub already:
the push legs; `Package (Windows, Release)` (the Release build and the ZIP check, green
on pushes to `develop`); `ios-build.yml` (the unsigned iOS compile); `release-tooling.yml`;
and the `docs.yml` build. `release.yml` has never run: it is not on the default branch, and
no `v*` tag exists. Everything below is
written, linted (actionlint, ShellCheck) and reasoned, and has not run:

- the Linux and macOS desktop **Qt builds** (the desktop application has only ever been
  built on Windows), the **AppImage** (`build_appimage.sh`, linuxdeploy and its Qt plugin,
  and the list of system libraries Qt needs on the hosted image), the macOS
  **bundle**, `verify_bundle.sh`, **signing and notarisation** (no certificate
  exists to try them with) and the `.dmg`;
- the **iOS** signing, archive, export and install, and the application on a device: only
  the unsigned compile has run, and the list of what is likeliest to fail is [in the iOS
  section](#what-ios-has-and-has-not-shown);
- the NSIS installer and its branding (icon, shortcuts, `Program Files` path) and
  its silent install test;
- `actions/attest-*`, `gh attestation verify`, `gh release create`, that GitHub
  accepts the tag ruleset, and the Pages deployment;
- the nightly jobs that scan `develop`, including CodeQL's `ref`/`sha` upload.

The first dry run is expected to need a fix or two; that is what it is for.

## Documentation site

`docs.yml` builds the site (`mkdocs.yml`, `docs/`) with
`mkdocs build --strict --config-file .github/mkdocs-ci.yml`. The CI config inherits
`mkdocs.yml` and raises MkDocs' own checks from information to warnings, which
`--strict` turns into failures: a link to a page, a heading or a target that does
not exist is a red build. Pages that exist but are not in the nav stay
information (`docs/` holds design notes that are not meant for the menu). MkDocs
and the Material theme are pinned in `.github/requirements/docs.txt`, MkDocs below
2.0. The job runs only when something under `docs/` or the docs tooling changes,
so it is not a required check.

Publishing needs one setting, once, that a workflow cannot make for itself: the
repository's Pages source is "GitHub Actions" (Settings, Pages; or
`gh api -X POST repos/iainchesworthlabs/wordjewelssolver/pages -f build_type=workflow`).
That is done (the repository's Pages are set to build from a workflow). Nothing else is
switched on or off: the deploy job runs on every push to `main` that changes what the
workflow's `paths` list, and `docs.yml` is not on `main` yet, so nothing is published yet.

`docs/install-ios.html` is not a page: it is the iOS install link's page
([iOS](#ios)), a static file that MkDocs copies into the site as it is, so it is
deployed with the docs and the iOS install link works only once the site is published.

A push to `main` that changes the docs deploys the site to
`https://iainchesworthlabs.github.io/wordjewelssolver/` (`site_url` in
`mkdocs.yml`), so the merge of `develop` into `main` that precedes the first release
is what makes the site, and the install page, exist: by the time a tag is cut they
are there. (Until the Pages source is set, `Publish to GitHub Pages` is red with
GitHub's message that Pages is not enabled.) A build of a pull request is never
deployed.

## Required checks and their staging

The rulesets from `icl-common-infra` require two checks on `develop`: `CI Status`
and `Scan dependency diff` (`dependency-review.yml`). `CI Status` waits for the
jobs of `ci.yml` by id, so a leg can be renamed or added without touching the
ruleset. The other jobs are not required by name:

| Job | Workflow | Reached through |
| --- | --- | --- |
| `Policy`, `Vendored CMake modules`, `clang-format`, `platform-isolation`, `Plan build legs` | `ci.yml` | `CI Status` |
| `Windows (MSVC)`, `Linux (GCC)`, `macOS (AppleClang)`, `Coverage (Linux, GCC)`, `Performance (Linux, GCC)`, `ThreadSanitizer (Linux, Clang)`, `Package (Windows, Release)` | `ci.yml` | `CI Status`, when the plan asked for the job |
| `iOS (device, unsigned compile)` | `ios-build.yml` | not required: path-filtered, and a macOS minute is billed at a multiple |
| `Android (arm64-v8a, debug-signed APK)` | `android-build.yml` | not required: path-filtered, and the Qt kits are a download of several gigabytes |
| `Analyze (C++)`, `clang-tidy`, `MSVC /analyze`, `Sanitizers (Linux, Clang)` | `codeql.yml`, `analysis.yml` | not required: nightly, informational |

`Build the documentation (strict)` (`docs.yml`), the two `release-tooling.yml` jobs,
`iOS (device, unsigned compile)` and everything in `release.yml` are not required and
cannot be: each is
path-filtered or is a tag, schedule or manual run, and a required check that a
filter keeps from running never reports, which would hold every unrelated pull
request.

The three `analysis.yml` jobs and `Analyze (C++)` are deliberately absent from
the shared rulesets: they are a nightly report, and a required check that
only reports once a day would hold every pull request.

Every workflow that produces a required check also triggers on `merge_group`,
because the merge queue evaluates the required checks on its temporary
`gh-readonly-queue/...` ref and GitHub only runs workflows that opt in there.
A workflow that produces **no** required
check must not carry `merge_group`, or every queue entry burns a run of it for
nothing: that is why `analysis.yml` and `codeql.yml` do not have it.

## The vcpkg binary cache

Dependencies come from vcpkg (the labs' fork, manifest mode): Catch2 behind
the `tests` feature, and Tracy (the repository's own overlay port in
`cmake/vcpkg-ports/tracy`, with its command-line capture tools built for the
host) behind `profiling`, which a Debug build turns on. Every job that runs
vcpkg uses the same two composite actions,
`.github/actions/vcpkg-cache/restore` and `/save`, which wrap vcpkg's `files`
binary-cache provider around `actions/cache`:

| Step | What it does |
| --- | --- |
| restore | Points `VCPKG_BINARY_SOURCES` at `$RUNNER_TEMP/vcpkg-cache` and fills it from the newest entry whose key starts `vcpkg-binary-cache-<triplet>-<ports hash>-` (made for these ports), else the newest that starts `vcpkg-binary-cache-<triplet>-`. |
| *(build)* | vcpkg restores every archive whose ABI hash matches and builds only the rest. |
| save | Runs twice in a build leg: right after Configure, which is when vcpkg has built whatever was missing, and with `if: always()` as the last step, so a red or timed-out leg keeps what it built. Stores the directory under `vcpkg-binary-cache-<triplet>-<ports hash>-<hash of its file listing>`, only if an archive was added since the restore and the key is not already readable from this ref. |

The key carries the **triplet** (`x64-windows`, `x64-linux`, `arm64-osx`) and a
**hash of `vcpkg.json`, `cmake/vcpkg-commit.txt` and `cmake/vcpkg-ports/**`**.
vcpkg's ABI hash already covers what changes the binaries, and every archive
holds both the debug and the release variant, so the hash is not what decides
whether an archive is reused (the second restore key takes the newest entry of
the triplet whatever its hash, and vcpkg reuses every archive that still
matches). It decides which entry is tried first: one made for these ports, from
develop, before an older one a pull request saved into its own scope before a
port changed. `Linux (GCC)` and `Coverage (Linux, GCC)` share `x64-linux`.

An Actions cache entry is readable from its own ref, the base branch of a pull
request and the default branch, nothing else. So `develop` is where the cache
every pull request starts from is made: a push to `develop` (or `main`) is
never cancelled by the next push once it is running (`cancel-in-progress` is
off for them; a run still waiting is replaced by a newer one), and the first
run on a new branch is cold. A cold Windows run builds Tracy's capture tools
from source, which took 13 to 18 minutes of a 75-minute job (the overlay port
now builds them without link-time optimisation: with `/GL` each of the seven
tools' links did the code generation of the same server sources on its own,
side by side; compiled once they take under 4 minutes, 3.9 measured on a hosted
runner), so a pull request that
changes a port, or starts before develop has stored the new archives, pays for
it once; develop's runs run one after the other, so an entry made by a leg that
saved only at its end reached the pull requests an hour or more after the
merge, and that is why the save also runs right after Configure. An entry a
pull request saves is readable only from that pull request, so a pull request
cannot warm develop for the next one.

Two kinds of run never see that cache, because they are neither on `develop`,
nor a pull request into it, nor on `main`: a **merge-queue entry** (its ref is
`gh-readonly-queue/develop/pr-N-<sha>`; an entry's `Cache not found` was seen on
the first queue run after the Windows application was added) and a **manual
dispatch** on a feature branch. Both start with no vcpkg archives, no Qt kit and
no ccache directory, whatever develop holds. Making `develop` the default
branch, or a job on `main` that builds and saves the caches (a scheduled run
always runs on `main`), would fix the queue; neither is done here.

**Tracy is built for any machine of its architecture.** The cache is shared between
hosted machines that need not have the same CPU, and Tracy's own CMake compiles for
the machine it is built on (`-march=native` on x86, `-mcpu=native` on arm). A
`tracy-capture` built on one runner and restored on another with an older CPU died with
an illegal instruction, and `profiling.trace_cli` failed at random on the Linux legs. So
the repository carries the fork's `tracy` port as an overlay (`cmake/vcpkg-ports/tracy`,
named in `vcpkg-configuration.json`, kept byte for byte by `.gitattributes`) that adds
`-DNO_ISA_EXTENSIONS=ON`, and `vcpkg.json` pins `tracy` to the version the overlay is.
`profiling.tracy_portable` holds the overlay, the option and the pin; the platform
isolation check leaves the overlay ports out of its OS-test rule, since they are
vendored third-party build files. When the fork's port passes the option itself, the
overlay, the check and the `.gitattributes` line go ([Profiling](profiling.md)).

vcpkg is not a submodule: the vcpkg-using jobs run the composite action
`.github/actions/vcpkg-setup`, which clones the fork into `$RUNNER_TEMP/vcpkg`
at the commit in `cmake/vcpkg-commit.txt` and exports `VCPKG_ROOT`, which the
presets read. The clone is full, not depth 1: with a `builtin-baseline`, vcpkg
reads historical port versions out of its own git history. The same file is
what a local configure checks a developer's clone against.

## The compiler cache (Windows)

The Windows leg compiles about 480 translation units with AddressSanitizer on
four cores (the library, the Qt Quick application, its qmltc output, the 3D
prototype and every test), 7 to 16 minutes whenever anything was touched. The
`Windows (MSVC)` leg and the `Package (Windows, Release)` job therefore build
through [ccache](https://ccache.dev), with two composite actions that wrap it
the way `vcpkg-cache` wraps vcpkg's provider: `.github/actions/ccache/setup`
(install, point at a directory, restore) and `.github/actions/ccache/save`
(report, save).

| Piece | How |
| --- | --- |
| Install | The pinned release zip (`4.14.1`), checked against its SHA-256 before it goes on `PATH`. |
| Use | `-DCMAKE_CXX_COMPILER_LAUNCHER=ccache` on the CI configure only; a developer's build is unchanged. |
| Debug information | The Debug leg also passes `-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded` (`/Z7`): ccache cannot cache a compile that writes a shared `.pdb` (`/Zi`, the Debug default), only objects that carry their own debug information. The Release job's objects already do (vcpkg's flags). Nothing reads a `.pdb` from the Debug build (the package script forbids them in a Release one). |
| Key | `ccache-<os>-<preset>-<run id>-<attempt>`; the restore takes the newest entry with the `<os>-<preset>` prefix. The cache directory is limited to 600 MB by ccache itself (the Debug directory is about 260 MB, the Release one about 50). |
| Who saves | A **push** or a **manual run** only, right after the Build step (so a superseded run still offers it), and only when the run compiled at least 100 objects: a build always misses the thirty-odd objects that include the version block, which carries the commit. A pull request and a queue entry restore and never save. |
| Who reads | A pull request reads develop's entry (its base branch); a dispatch reads its own branch's. A merge-queue entry reads **none** (below). |
| The compiler | `CCACHE_COMPILERCHECK` is the banner `cl` prints (`Version 19.51.36260`), not cl.exe's modification time, which is the default and differs between builds of the runner image: two images, with two MSVC builds, were in use at the same time, and the release job missed all 232 objects when it landed on the other one. |
| Paths | `CCACHE_BASEDIR` is the checkout, so the workspace path is not in the hash; the generated sources (moc, rcc, qmltc) hash like any other. |

On a hit the Windows build step went from 567-960 s to 115 s (448 of 480 objects
hit; the 32 misses are the objects that include the version block, which names
the commit) and the compiler output is the same objects, so the warning gate
(`/W4 /WX`) is unchanged: a hit replays the stored diagnostics, and nothing
that compiled with a warning was ever stored (a compile that fails is not
cached).

The job summary of every run has a line with the hits, the misses and whether
an entry was saved.

## What CI pins

| | Pinned to | Kept current by |
| --- | --- | --- |
| Every GitHub Action, and the composite actions in `.github/actions` | A full commit SHA, version in a trailing comment | Dependabot, weekly, grouped, 7-day cooldown |
| clang-format, gcovr, clang-tidy | `==` in `.github/requirements/*.txt` | Dependabot (`pip`), weekly |
| GCC (the Linux GCC jobs) | The major version in `.github/toolchain-versions.json` (`gcc`), installed by `setup-gcc` from `icl-common-infra`; the exact version is printed at the start of each leg | By hand |
| CMake and Ninja (`lukka/get-cmake`) | `cmakeVersion` / `ninjaVersion`, exact, and one the pinned action's catalog lists | By hand, with the action |
| Qt (`jurplel/install-qt-action`) | The action's commit SHA, the Qt version, the kit per host (`win64_msvc2022_64`, `linux_gcc_64`, `clang_64`, the iOS kit), the module list (`qtquick3d qtshadertools qtmultimedia qtsensors qtspeech`) and the `aqtsource` commit, in `.github/toolchain-versions.json` | By hand: run `aqt list-qt windows desktop --modules <version> <arch>` **and** a real install before bumping, as the action's README says |
| vcpkg and the ports | `cmake/vcpkg-commit.txt` and `builtin-baseline`; `tracy` is overridden to one version, and built from the overlay port in `cmake/vcpkg-ports` | Dependabot moves `builtin-baseline` (`vcpkg`, monthly); the commit file is by hand, and the two move together; the overlay moves with the override, by hand |
| linuxdeploy and its Qt plugin (the AppImage tools) | A release tag and a SHA-256 each, in `libs/platforms/linux/packaging/linuxdeploy.pins` | By hand, with `pin_linuxdeploy.sh` |
| actionlint, ShellCheck, the SBOM schema validator | `==` in `.github/requirements/workflows.txt` and `sbom.txt`; the CycloneDX schema at the commit of its `1.6` tag, in `release-tooling.yml` | Dependabot (`pip`), weekly; the schema commit by hand |
| NSIS (the Windows installer tool) | `choco install nsis --version 3.13.0`, in `release.yml`, when the image does not carry it | By hand |
| ccache (the Windows compiler cache) | The release zip's version and SHA-256, in `.github/actions/ccache/setup/action.yml` (the digest GitHub lists for the asset) | By hand |
| Runner images | The label (`ubuntu-24.04`, `ubuntu-26.04`, `windows-2025-vs2026`, `macos-26`) | By hand |

The clang-tidy version is in `.github/toolchain-versions.json` as well as in
the requirements file; the `toolchain.versions` ctest holds the two together,
and reads the CMake and Ninja pins out of both `ci.yml` and `analysis.yml`.

Not pinned: the other compilers (each is what its image ships; MSVC and its analyzer are
that image's `cl.exe`, AppleClang is Xcode's, and the Clang sanitizer legs use the image's `clang++`), and
anything an image provides (`pwsh`, `jq`, `git`).

## Commit and PR text policy

**The rule.** Commit messages, pull request titles and pull request bodies
carry *no attribution of any kind*:

- no `Co-authored-by` trailers (or any mention of one);
- no "generated with / by / using ..." lines;
- no mention of a code-generation tool, an assistant, or the vendor behind one
  - its name, a model name, a noreply address, or a phrase crediting it with
  the work.

**Where it is applied.**

- The Policy job of `ci.yml` runs the shared text policy from
  `icl-common-infra` over the PR title, the PR body and *every* commit message
  in the PR range.
- `tools/hooks/commit-msg` applies `tools/policy/check_text_policy.ps1` to the
  message being committed. `tools/policy/test_check_text_policy.ps1` (ctest
  `policy.text`) proves the script still rejects what it must, so a broken
  pattern cannot quietly pass everything.

**The script.** `check_text_policy.ps1` takes text as `-Text`, `-Path` or
stdin and exits `0` (clean), `1` (violations) or `2` (bad invocation). Each
finding is one line, `<label>:<line>: <what> [<rule>]: <the line>`. In
`-GitMessage` mode it ignores `#` comment lines and everything below the
scissors line, as git's own cleanup does. The deny-list is assembled at run
time from short fragments, so the file itself contains none of the names it
forbids and passes its own check (the self-test runs it over itself).

**Rewording.** The generic "generated with/by/using" pattern is deliberately
blunt: a sentence such as "the header is generated by CMake" trips it. Say
"CMake writes the header" instead. The check looks at lines, so a markdown
heading is checked like any other line.

## Local hooks

The hooks live in `tools/hooks/` and are plain POSIX `sh` (they run under Git
for Windows' shell) that call `pwsh` where they need the policy script.
Enable them once per clone:

```sh
git config core.hooksPath tools/hooks
```

| Hook | Does | If `pwsh` is missing |
| --- | --- | --- |
| `commit-msg` | Runs the text policy over the message; refuses the commit on a violation. | Says so and lets the commit through - the Policy job applies the same rule. |
| `pre-push` | Refuses to push any branch whose name is not `main`, `develop` or `feature/`, `bugfix/`, `docs/`, `chore/`, `agent/` or `release/` plus a name. Tags and branch *deletions* are not checked (deleting is how a bad name is removed). | Not needed: it is pure `sh`. |

Both can be bypassed with `--no-verify`; they are a faster place to be told,
not the enforcement. The enforcement is the pull request checks and, once
public, the rulesets.

Two repository details the hooks depend on: they must keep **LF** line endings
(a CRLF checkout breaks the `#!/bin/sh` line), and they must be **executable**
in the index (git ignores a hook that is not, with a warning).

## When the first run goes red

The first run, on the pull request that introduced the build, was green on all
three desktops (Windows with MSVC 19.51, Linux with GCC 15.2, macOS with Apple clang
21: each leg uses whatever its image ships, and the `Show the toolchain` step prints
it). When a later run goes red on one of them, the usual suspects, in order of
likelihood:

1. **Warnings as errors** on a compiler version nobody has tried: `-Werror`
   with `-Wconversion`, `-Wsign-conversion`, `-Wold-style-cast` and friends
   will say exactly which line.
2. **A platform source on a changed image** (`libs/platforms/linux`,
   `libs/platforms/macos`): they compile on every push, so the leg's log names
   the line, and the usual cause is a runner image that moved to a newer SDK.
3. **The sanitizer runtime**: ASan/UBSan on the hosted image (on Windows, the
   ASan runtime DLL that `wjs_deploy_sanitizer_runtime` copies next to the
   tests, found next to the compiler).
4. **The vcpkg clone**: one without enough history for the baseline (the
   setup action clones in full for that reason), or a `cmake/vcpkg-commit.txt`
   the fork does not have.
5. **Coverage**: gcovr refusing a counter, or the gate refusing the tree. The
   report step passes
   `--gcov-ignore-parse-errors negative_hits.warn_once_per_file` for the known
   gcov bug, and the coverage preset updates counters atomically. A failure
   that names a percentage and a floor is the gate, not the tool: the message
   lists the file and line in `summary.json`/the HTML report.
6. **clang-tidy**: a nightly finding names the check and `file:line`; run the
   same command from [Nightly analysis](#running-the-same-analysis-locally)
   against the same preset.
7. **MSVC `/analyze`**: a C6xxx warning is the error; the compile line names
   the SARIF file, and the code-scanning alert names the rule.

## Not here yet

Qt on the push legs of Linux and macOS. `release.yml` builds the application on
both (the Linux AppImage and the macOS bundle are its first non-Windows desktop
builds), but `ci.yml`'s Linux and macOS legs build the library, platform layer, CLI
and scanner alone. The desktop application has therefore never been built on Linux
or macOS on GitHub, and a regression in it would be red only in a release
rehearsal, which cannot start until `develop` has been merged to `main`. They should
get the kit once a rehearsal has built the application on them, so that a regression
is red in the pull request. (The iOS compile check is the exception: it builds the
application against Qt's iOS kit on macOS, [iOS](#ios).) Also not here: Android
(in progress on a separate branch; [Releases](#releases)), the iOS haptics (the
desktops and iOS answer with the shared no-op), a signed iOS build, an install or a
run on a device ([iOS](#ios) says what is unverified), and web. The Windows clang-cl
coverage preset (`windows-clang-coverage`, with `coverage-unit` and `coverage-ui`) is
available for a local run but is not a CI job: the Linux GCC job runs the same unit
gate. Each remaining item is a new job or a flag flip, not a redesign.

SonarQube or SonarCloud is not wired: it needs a server (or a SonarCloud
organisation), a token and a quality-gate policy, and CodeQL, clang-tidy and
MSVC's analyzer already cover the C++ with no account to keep. The C++
compilation-database route is the one to add if a server appears: configure the
same lean `linux-gcc-analysis` database and point the scanner at
`build/linux-gcc-analysis/compile_commands.json`.

Packaging is built by `ci.yml` on the Windows leg only (above), as a Debug
development build, plus the `Package (Windows, Release)` job, and for real by
`release.yml` on all three desktops ([Releases](#releases)): the Windows ZIP and
NSIS installer, the Linux AppImage of the application and the Linux solver's ZIP,
tar.gz, deb and rpm, and the macOS disk image. The Linux and macOS application
builds, the AppImage, the bundle and the signing path have not yet run on GitHub.
iOS has a platform section, an unsigned compile check (which passes on GitHub) and a
signed ad-hoc release job (which has not run); nothing has run on a device
([iOS](#ios)). Android has none of these in this tree.
