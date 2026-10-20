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
| Gitflow branch names | `Validate gitflow branch name`, and the `branch-names` ruleset refuses the push | `tools/hooks/pre-push` |
| No attribution in commits and PR text | `Validate commit and PR text`, as a required check | `tools/hooks/commit-msg` |
| Pull request required, no direct push, no force push | the `main-protection` and `develop-protection` rulesets | - |
| Merge queue in front of `develop` | the `merge-queue-develop` ruleset | - |

Install the hooks once per clone with `git config core.hooksPath tools/hooks`.

## Workflows

| Workflow | Purpose | Runs on a private copy? |
| --- | --- | --- |
| `ci.yml` | `clang-format`, `platform-isolation`, the Windows / Linux / macOS build-and-test legs, a coverage report and the performance work budgets | yes |
| `analysis.yml` | nightly `clang-tidy`, MSVC `/analyze` and a Clang AddressSanitizer + UBSan build and test; no required checks | yes (the SARIF upload is skipped) |
| `release.yml` | on a `v*` tag: Release packages for Windows, Linux and macOS, SBOM, checksums, GPG signatures, attestations, a GitHub Release; by hand or weekly: the same without publishing ([Releases](#releases)) | yes (the build, test and signing steps; publishing needs a tag) |
| `docs.yml` | `mkdocs build --strict` of the documentation site, and its publication to GitHub Pages from `main` once the author has enabled that ([Documentation site](#documentation-site)); only when the docs change | yes (the build; the publication needs Pages) |
| `release-tooling.yml` | lints the workflows (actionlint, ShellCheck) and self-tests the release scripts with a throw-away key; only when those files change | yes |
| `branch-name-check.yml` | gitflow branch naming; the source/target pairing | yes |
| `commit-policy.yml` | no attribution in the PR title, PR body or any commit message of the PR | yes |
| `codeql.yml` | nightly CodeQL static analysis of the C++ | no - skipped |
| `dependency-review.yml` | fails a PR that adds a moderate-or-worse vulnerable dependency | no - skipped |
| `scorecard.yml` | OpenSSF Scorecard score and badge, from `main` | no - skipped |
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

## The jobs in `ci.yml`

The job *display names* are the required-check contexts in
`.github/rulesets/*.json`. They are stable on purpose; rename one only together
with the rulesets.

| Job (display name) | Runs on | What a green run proves |
| --- | --- | --- |
| `clang-format` | ubuntu-24.04 | Every `.cpp`/`.hpp` (and sibling extensions) under `src/` and `tests/` is exactly what the repository's `.clang-format` produces, with one pinned clang-format release (`.github/requirements/lint.txt`). Any difference fails; the log names file and line. |
| `platform-isolation` | ubuntu-24.04 | `tools/checks/check_platform_isolation.ps1`: no preprocessor conditional, no OS header or OS-querying API outside `src/platforms/`, no OS test in CMake outside the selector, no `#define`, every platform source wired. Fast, and deliberately not behind the build legs. |
| `Plan build legs` | ubuntu-24.04 | The build matrix was computed. It is a required check only so that a failure here cannot turn the three build legs into "skipped" (which counts as a pass). |
| `Windows (MSVC)` | windows-2025-vs2026 | Preset `windows-msvc-debug` configures, builds warning-free (`/W4 /WX`) with ASan, and all of ctest passes. The leg also builds the Qt application, runs its offscreen smoke, and packages a ZIP whose contents and packaged programs it verifies (the `wordjewelssolver-windows-x64` artifact). |
| `Linux (GCC)` | ubuntu-26.04 | Preset `linux-gcc-debug`: the image's default `g++` builds warning-free (`-Wall -Wextra -Wpedantic -Werror ...`) with ASan and UBSan, and all of ctest passes. |
| `macOS (AppleClang)` | macos-26 (Apple silicon) | Preset `macos-clang-debug`: AppleClang from Xcode builds warning-free with ASan and UBSan, and all of ctest passes. |
| `Coverage (Linux, GCC)` | ubuntu-26.04 | Preset `linux-gcc-coverage` builds and runs the unit tier, and gcovr over `src/lib` **fails the job below the project's minimum standard: 90% of lines and 80% of branches**. The HTML, `summary.json` and the table in the job summary are the `coverage-report` artifact. The same gate is `coverage-unit` locally ([Testing](testing.md#coverage)). |
| `Performance (Linux, GCC)` | ubuntu-26.04 | Preset `linux-gcc-release` (the one leg that builds a Release preset) builds warning-free and passes all of ctest, which in an optimised build includes the [work budgets](testing.md#performance-tests): visits, words and trie nodes over a fixed corpus, and a digest of every solution. The wall-time checks run only in a manual run. **Not a required check** (the rulesets are as they were). |

What a build-and-test leg's ctest contains: the four tiers of
[Testing](testing.md#the-tiers) - the unit suite, the integration suite (the
scanner on every leg; the window's model and the Quick Test suites where Qt is
built), the system suite (`system.cli.*` runs `wjs-cli` itself and
`system.install` runs `cmake --install` into a throw-away prefix and executes
the installed solver on every leg; `system.app.*` runs the application's
start-up where the window is built) - and the repository
checks, which carry the `quality` label: `platform.fragments` (every platform
fragment satisfies the platform
contract, on whichever host this is, including that every interface header in
`src/platforms/interfaces` has an implementation in the fragment's sources),
`platform.isolation` (the same script as the `platform-isolation` job; it needs
`pwsh`, which every hosted image has), `profiling.shipped_presets` (no preset
that ships has Tracy in it), `profiling.zone_names`, `analysis.config` (the
nightly `analysis.yml` presets exist with their settings, and the CodeQL
workflow is nightly), `notices.sections` (the generated third-party notices
state every section a package must),
`package.config` (CPack names this project, and the install rules carry the
programs the build makes, their data and the notices), `data.sounds` (the
checked-in feedback WAVs are what `tools/sounds/make_sounds.ps1` writes), and
the performance tier, which skips itself in a Debug build: the Debug legs prove
it compiles on all three compilers, and the `Performance` leg runs it.

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
as the `wordjewelssolver-windows-x64` artifact. The script is the same one a
developer runs, so a broken install rule is red here on every push rather than
at release time.

That leg also carries the **interface's languages**: each of the eight
catalogues is checked for completeness, the QML screens are loaded in all eight
(including Hebrew and Yiddish, with the layout mirrored), the glyph check runs
on the platform's fonts, and `i18n.catalogues` regenerates the catalogues with
the kit's `lupdate` and fails when a checked-in one differs. The last is the
drift gate: a `qsTr()` added without regenerating `i18n/*.ts` is a red leg, not
a silent untranslated string. It is a ctest, so it skips itself (exit 4) on a
machine without Qt's Linguist tools, but the Windows leg installs them (below).

The same leg runs **the interactive QML at run time**: the eight `quick.*`
suites ([Testing](testing.md#the-interactive-qml-qt-quick-test)) start
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
- **Debug presets.** CI builds the Debug presets because they carry the
  sanitizers. The Release presets are not built in CI yet; a Release-only
  warning (`-Wnull-dereference`, for one) would first show up locally.
- **Qt on the Windows leg.** The Windows leg installs the prebuilt Qt 6.12.0
  kit with `jurplel/install-qt-action` (pinned to v4.4.1 by commit SHA) and
  configures with `-DWJS_BUILD_APP=ON -DWJS_BUILD_SHAKE_PROTOTYPE=ON`, so the
  Qt Quick application, its 3D scene prototype, the app tests, `parent.screens`,
  `scan.screens`, `camera.screens`, `parent.languages`, `scan.languages`, the eight `quick.*`
  suites, `i18n.catalogues`, `app.scene_contract` and `app.qml_lint` all run on
  every push. The install names the modules `qtmultimedia qtsensors qtspeech`
  (see `aqt list-qt windows desktop --modules 6.12.0 win64_msvc2022_64`); the
  base kit itself carries `lupdate`, `lrelease` and Qt Linguist (from 6.12
  `qttools` is an archive of the base package, and naming it as a module makes
  aqtinstall fail), and the modules are the application's sound, motion and speech, so this is
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
  between runs. The other legs build the library, CLI and scanner alone; the
  application has only been built on Windows, so Qt is not installed there yet.
  `QT_QPA_PLATFORM=offscreen` lets the screens run on a runner with no display.
- **The feedback and camera modules are optional.** `src/app` finds Multimedia,
  Sensors and TextToSpeech with `OPTIONAL_COMPONENTS` and selects a silent
  backend for a missing one (the variant pattern), so a contributor's kit still
  configures and builds; the camera uses the same choice, with a `camera-none`
  backend when Multimedia is missing. The Windows CI leg installs them on
  purpose, so the package check can expect `Qt6Multimedia.dll`,
  `Qt6Sensors.dll` and `Qt6TextToSpeech.dll` beside the window, and the camera's
  `plugins/multimedia` and `qml/QtMultimedia` in its package.
- **CMake 4.4.2 and Ninja 1.13.2** from `lukka/get-cmake`; the same pins the
  sibling projects use.
- **One place for the pins.** `.github/toolchain-versions.json` lists CMake,
  Ninja, the Qt version, architecture, modules, the `install-qt-action` pin and
  the `aqtsource` commit, and names the vcpkg pin file. The workflows are held
  to it: the `toolchain.versions` ctest fails if a workflow pins another version,
  if the `install-qt-action` step stops naming the file's action, version,
  architecture, modules or aqtsource, if it stops asking for `cache: true`, if
  the old PyPI `aqtinstall==` line comes back, or if `vcpkg.json`'s
  `builtin-baseline` is not the commit in `cmake/vcpkg-commit.txt`. The two
  vcpkg pins move together (docs/ci.md, "The vcpkg binary cache").
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
`windows`, `linux`, `macos`, `coverage`, `perf`, or `all` (the default).
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

A scheduled run is on the default branch (`main`), and so is the workflow it
runs: GitHub reads a `schedule` and the `workflow_dispatch` listing only from
the default branch. At the time of writing `main` is at the merge of PR #2,
so it has the older `codeql.yml` and `scorecard.yml` (both have run green on
their schedules) and **no `analysis.yml` at all**: the nightly clang-tidy, MSVC
`/analyze` and Clang-sanitizer jobs, like the weekly release rehearsal, start
only once `develop` has been merged to `main`.

**What a scheduled run analyses is `develop`, not `main`.** A schedule runs
`main`'s copy of the workflow on `main`'s commit, but the code worth checking
every night is the integration branch's, so `analysis.yml` and `codeql.yml` check
out `develop` when `github.event_name` is `schedule` (and the dispatched branch
otherwise), and the two jobs that upload to code scanning (`MSVC /analyze`,
CodeQL) pass `ref: refs/heads/develop` and the checked-out `sha` so the alerts
belong to the code that was analysed. To analyse another branch, dispatch
`analysis.yml` or `codeql.yml` on it from the Actions tab (once the workflow is on
`main`).

Every Debug preset has Tracy on, so a Debug configure on Linux has vcpkg build
Tracy's tools for the host, which needs autotools. The composite action
`.github/actions/linux-tracy-tools` installs them: `ci.yml`'s Linux leg and the
nightly `Sanitizers (Linux, Clang)` job (`linux-clang-debug` is a Debug preset) use
it before Configure. `clang-tidy` and CodeQL do not need it: they configure
`linux-gcc-analysis`, which has Tracy off, so there is nothing to install or to
cache, and CodeQL configures before its tracer starts regardless.

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
- **iOS and Android.** There is no platform section for either
  (`src/platforms` has `windows`, `linux`, `macos`), and building them needs
  what this repository does not hold: Qt's mobile kits, an Android keystore, and
  for iOS an Apple Developer Program certificate and provisioning profile held
  as secrets. `docs/roadmap.md` (decision D1) plans them as private builds for
  the author's own devices, taking the Android and iOS pipelines of
  CountdownSolver; until the platform sections and those secrets exist, a job
  here could not be run, so none is added. Releasing to a public store is
  a separate licensing decision (D2).
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
| `Build ...` (4 legs: Windows, Linux solver, Linux application, macOS) | as the table | Configures the Release preset (the application on for the three legs that build it, so a missing Qt is an error and not a silent command-line-only package; Qt 6.12.0 from `install-qt-action`, the kit per host in `.github/toolchain-versions.json`), builds, runs **all** of ctest, then packages. Windows also runs the application's offscreen smoke, `tools/package/check_package.ps1` on the ZIP, and installs the NSIS installer silently and runs the installed `wjs-cli`. The Linux application leg makes the AppImage with `build_appimage.sh` and smokes it. Linux solver and macOS run `tools/release/smoke_package.sh` over every package they made; macOS then runs `verify_bundle.sh` (plist, camera text, Qt inside, no link to the build machine, offscreen start) and `sign_and_notarize.sh`. Every leg ends with `tools/release/check_release_build.ps1` (Tracy and sanitizers off, the expected packages present). |
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
   VERSION x.y.z)` in `CMakeLists.txt` and `"version"` in `vcpkg.json`; rename
   `## [Unreleased]` in `CHANGELOG.md` to `## [x.y.z] - <date>` and put a new
   empty `## [Unreleased]` above it.
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
4. **Apply the tag ruleset**: `tools/repo/apply_rulesets.ps1 -Only release-tags`
   (a dry run), then `-Apply`. It restricts `v*` tags to the admin role.
5. **Get the workflows onto `main`**: merge `develop` there. Until then nothing
   scheduled or dispatched from this repository exists on the default branch
   except `codeql.yml` and `scorecard.yml` (older copies).
6. **Pin the AppImage tools** (once, on a machine you trust):
   `src/platforms/linux/packaging/pin_linuxdeploy.sh`, then commit
   `linuxdeploy.pins`. Until then the Linux application leg downloads linuxdeploy
   by its release tag, prints each file's SHA-256 with a warning and carries on
   (trust on first use); afterwards a download that differs fails the build.
7. **Optional: macOS signing.** With an Apple Developer ID Application
   certificate, set the six secrets `MACOS_CERTIFICATE` (base64 of the `.p12`),
   `MACOS_CERTIFICATE_PASSWORD`, `MACOS_SIGN_IDENTITY` (`Developer ID Application:
   Name (TEAMID)`), `NOTARY_APPLE_ID`, `NOTARY_TEAM_ID` and `NOTARY_PASSWORD` (an
   app-specific password), e.g. `base64 -i cert.p12 | gh secret set MACOS_CERTIFICATE`.
   Without them the disk image is unsigned (the run says so in an annotation).
8. **Rehearse**: `gh workflow run release.yml --ref main -f signing=repository`,
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

### The SBOM

`tools/release/generate_sbom.ps1` writes a CycloneDX 1.6 document from what the
repository already records, not from a scan of the binaries:

- the application (GPL-3.0-only: it links Qt Quick 3D) and `wjs-cli`
  (GPL-3.0-or-later), with the commit and the repository;
- Qt by module at the version in `.github/toolchain-versions.json`, each under the
  licence it is used under - **only if a ZIP in the release actually carries
  `Qt6Core.dll`**; likewise the Microsoft Visual C++ runtime. The command-line
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
`sign_release.sh` / `verify_release.sh` and the key script. Everything below is
written, linted (actionlint, ShellCheck) and reasoned, and has not run:

- the Linux and macOS **Qt builds** (the application has only ever been built on
  Windows), the **AppImage** (`build_appimage.sh`, linuxdeploy and its Qt plugin,
  and the list of system libraries Qt needs on the hosted image), the macOS
  **bundle**, `verify_bundle.sh`, **signing and notarisation** (no certificate
  exists to try them with) and the `.dmg`;
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

Publishing is off until the author turns it on, once:

```sh
gh api -X POST repos/iainchesworthlabs/wordjewelssolver/pages -f build_type=workflow   # Pages from GitHub Actions
gh variable set PUBLISH_DOCS --body true --repo iainchesworthlabs/wordjewelssolver
```

After that a push to `main` that changes the docs deploys the site to
`https://iainchesworthlabs.github.io/wordjewelssolver/` (`site_url` in
`mkdocs.yml`). Until then `Publish to GitHub Pages` is skipped, not red. A build
of a pull request is never deployed.

## Required checks and their staging

The same list is required on `develop` and `main` once the rulesets apply. The
contexts equal the job display names above and in the two policy workflows.

| Check (context) | Workflow | In the rulesets from day one? |
| --- | --- | --- |
| `Validate gitflow branch name` | `branch-name-check.yml` | yes |
| `Validate commit and PR text` | `commit-policy.yml` | yes |
| `clang-format` | `ci.yml` | yes |
| `platform-isolation` | `ci.yml` | yes |
| `Plan build legs` | `ci.yml` | yes |
| `Windows (MSVC)` | `ci.yml` | yes |
| `Linux (GCC)` | `ci.yml` | yes |
| `macOS (AppleClang)` | `ci.yml` | yes |
| `Coverage (Linux, GCC)` | `ci.yml` | yes |
| `Scan dependency diff` | `dependency-review.yml` | **no - staged**: added after it has reported on a pull request on the public repository (see below) |
| `Package (Windows, Release)` | `ci.yml` | no - **candidate**: add after it has been green on a few pull requests (it is the push-time proof that a Release package is clean) |
| `Analyze (C++)` | `codeql.yml` | no - nightly, informational |
| `clang-tidy` | `analysis.yml` | no - nightly, informational |
| `MSVC /analyze` | `analysis.yml` | no - nightly, informational |
| `Sanitizers (Linux, Clang)` | `analysis.yml` | no - nightly, informational |

`Build the documentation (strict)` (`docs.yml`), the two `release-tooling.yml` jobs
and everything in `release.yml` are not required and cannot be: each is
path-filtered or is a tag, schedule or manual run, and a required check that a
filter keeps from running never reports, which would hold every unrelated pull
request. `Package (Windows, Release)` runs on every push and pull request and so
could be; it is not required yet because it is new (`.github/rulesets/*.json` are
untouched by this work, and `tools/repo/apply_rulesets.ps1` stays a manual step).

The three `analysis.yml` jobs and `Analyze (C++)` are deliberately absent from
`.github/rulesets/*.json`: they are a nightly report, and a required check that
only reports once a day would hold every pull request.

Every workflow that produces a required check also triggers on `merge_group`,
because the merge queue evaluates the required checks on its temporary
`gh-readonly-queue/...` ref and GitHub only runs workflows that opt in there.
`branch-name-check.yml` and `commit-policy.yml` skip their job on a queue entry
(there is no head branch name, title or body of its own; a skipped job counts
as a pass for a required check). A workflow that produces **no** required
check must not carry `merge_group`, or every queue entry burns a run of it for
nothing: that is why `analysis.yml` and `codeql.yml` do not have it.

## The vcpkg binary cache

Dependencies come from vcpkg (the labs' fork, manifest mode; the
only one today is Catch2, behind the `tests` feature). Every job that runs
vcpkg uses the same two composite actions,
`.github/actions/vcpkg-cache/restore` and `/save`, which wrap vcpkg's `files`
binary-cache provider around `actions/cache`:

| Step | What it does |
| --- | --- |
| restore | Points `VCPKG_BINARY_SOURCES` at `$RUNNER_TEMP/vcpkg-cache` and fills it from the newest entry whose key starts `vcpkg-binary-cache-<triplet>-`. |
| *(build)* | vcpkg restores every archive whose ABI hash matches and builds only the rest. |
| save | Runs with `if: always()`, so a red leg keeps what it built. Stores the directory under `vcpkg-binary-cache-<triplet>-<hash of its file listing>`, only if an archive was added since the restore and the key is not already readable from this ref. |

The key carries the **triplet and nothing else** (`x64-windows`, `x64-linux`,
`arm64-osx`): vcpkg's ABI hash already covers what changes the binaries, and
every archive holds both the debug and the release variant. `Linux (GCC)` and
`Coverage (Linux, GCC)` share `x64-linux`.

An Actions cache entry is readable from its own ref, the base branch of a pull
request and the default branch, nothing else. So `develop` is where the cache
every pull request starts from is made: a push to `develop` (or `main`) is
never cancelled by the next push (`cancel-in-progress` is off for them), and
the first run on a new branch is cold. With Catch2 as the only port a cold
run costs minutes, not hours; the cache earns its keep mostly on macOS.

vcpkg is not a submodule: the vcpkg-using jobs run the composite action
`.github/actions/vcpkg-setup`, which clones the fork into `$RUNNER_TEMP/vcpkg`
at the commit in `cmake/vcpkg-commit.txt` and exports `VCPKG_ROOT`, which the
presets read. The clone is full, not depth 1: with a `builtin-baseline`, vcpkg
reads historical port versions out of its own git history. The same file is
what a local configure checks a developer's clone against.

## What CI pins

| | Pinned to | Kept current by |
| --- | --- | --- |
| Every GitHub Action, and the composite actions in `.github/actions` | A full commit SHA, version in a trailing comment | Dependabot, weekly, grouped, 7-day cooldown |
| clang-format, gcovr, clang-tidy | `==` in `.github/requirements/*.txt` | Dependabot (`pip`), weekly |
| CMake and Ninja (`lukka/get-cmake`) | `cmakeVersion` / `ninjaVersion`, exact, and one the pinned action's catalog lists | By hand, with the action |
| Qt (`jurplel/install-qt-action`) | The action's commit SHA, the Qt version, `win64_msvc2022_64`, the module list and the `aqtsource` commit, in `.github/toolchain-versions.json` | By hand: run `aqt list-qt windows desktop --modules <version> <arch>` **and** a real install before bumping, as the action's README says |
| vcpkg and the ports | `cmake/vcpkg-commit.txt` and `builtin-baseline` | Dependabot moves `builtin-baseline` (`vcpkg`, monthly); the commit file is by hand, and the two move together |
| actionlint, ShellCheck, the SBOM schema validator | `==` in `.github/requirements/workflows.txt` and `sbom.txt`; the CycloneDX schema at the commit of its `1.6` tag, in `release-tooling.yml` | Dependabot (`pip`), weekly; the schema commit by hand |
| NSIS (the Windows installer tool) | `choco install nsis --version 3.13.0`, in `release.yml`, when the image does not carry it | By hand |
| Runner images | The label (`ubuntu-24.04`, `ubuntu-26.04`, `windows-2025-vs2026`, `macos-26`) | By hand |

The clang-tidy version is in `.github/toolchain-versions.json` as well as in
the requirements file; the `toolchain.versions` ctest holds the two together,
and reads the CMake and Ninja pins out of both `ci.yml` and `analysis.yml`.

Not pinned: the compilers (each is what its image ships; the MSVC analyzer is
that image's `cl.exe`, and the Clang sanitizer leg is its `clang++`), and
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

- `commit-policy.yml` (job `Validate commit and PR text`) runs
  `tools/policy/check_text_policy.ps1` over the PR title, the PR body and
  *every* commit message in the PR range (`base..head`). It first runs
  `tools/policy/test_check_text_policy.ps1`, which proves the script still
  rejects what it must, so a broken pattern cannot quietly pass everything.
- `tools/hooks/commit-msg` applies it to the message being committed.

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
| `commit-msg` | Runs the text policy over the message; refuses the commit on a violation. | Says so and lets the commit through - the pull request check applies the same rule. |
| `pre-push` | Refuses to push any branch whose name is not `main`, `develop` or `feature/`, `bugfix/`, `release/`, `hotfix/`, `support/` plus a name. Tags and branch *deletions* are not checked (deleting is how a bad name is removed). | Not needed: it is pure `sh`. |

Both can be bypassed with `--no-verify`; they are a faster place to be told,
not the enforcement. The enforcement is the pull request checks and, once
public, the rulesets.

Two repository details the hooks depend on: they must keep **LF** line endings
(a CRLF checkout breaks the `#!/bin/sh` line), and they must be **executable**
in the index (git ignores a hook that is not, with a warning).

## When the first run goes red

The first run, on the pull request that introduced the build, was green on all
three desktops: Windows (MSVC 19.51), Linux (GCC 15.2) and macOS (Apple clang
21), 28 tests each. When a later run goes red on one of them, the usual
suspects, in order of likelihood:

1. **Warnings as errors** on a compiler version nobody has tried: `-Werror`
   with `-Wconversion`, `-Wsign-conversion`, `-Wold-style-cast` and friends
   will say exactly which line.
2. **A platform source that has never compiled** (`src/platforms/linux`,
   `src/platforms/macos`): the leg's log is the first compile of it.
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

Qt on the push legs of Linux and macOS: `release.yml` builds the application on
both (the Linux AppImage and the macOS bundle are its first non-Windows builds), but
`ci.yml`'s Linux and macOS legs still build the library, CLI and scanner alone. They
should get the kit once a rehearsal has built the application on them, so that a
regression is red in the pull request; until then the weekly rehearsal is the only
check on them. Also not here: the Android/iOS targets (and with them the real Qt
Sensors and haptics backends; the desktops answer haptics with the shared no-op),
and web. The Windows clang-cl coverage preset (`windows-clang-coverage`, with
`coverage-unit` and `coverage-ui`) is available for a local run but is not a CI
job: the Linux GCC job runs the same unit gate. Each remaining item is a new job
or a flag flip, not a redesign.

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
iOS and Android have no platform section and no job.
