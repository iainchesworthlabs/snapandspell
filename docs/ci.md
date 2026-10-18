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
| `branch-name-check.yml` | gitflow branch naming; the source/target pairing | yes |
| `commit-policy.yml` | no attribution in the PR title, PR body or any commit message of the PR | yes |
| `codeql.yml` | CodeQL static analysis of the C++ | no - skipped |
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
| `Coverage (Linux, GCC)` | ubuntu-26.04 | Preset `linux-gcc-coverage` builds and passes its tests, and a gcovr report of `src/` is produced as the `coverage-report` artifact (HTML, plus `summary.json` and a table in the job summary). **Report only: there is no threshold.** |
| `Performance (Linux, GCC)` | ubuntu-26.04 | Preset `linux-gcc-release` (the one leg that builds a Release preset) builds warning-free and passes all of ctest, which in an optimised build includes the [work budgets](testing.md#performance-tests): visits, words and trie nodes over a fixed corpus, and a digest of every solution. The wall-time checks run only in a manual run. **Not a required check** (the rulesets are as they were). |

What a build-and-test leg's ctest contains: the Catch2 unit suite, the repository
checks `platform.fragments` (every platform fragment satisfies the platform
contract, on whichever host this is, including that every interface header in
`src/platforms/interfaces` has an implementation in the fragment's sources),
`platform.isolation` (the same script as the `platform-isolation` job; it needs
`pwsh`, which every hosted image has), `profiling.shipped_presets` (no preset
that ships has Tracy in it) and `profiling.zone_names`, `notices.sections` (the
generated third-party notices state every section a package must),
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
  every push. The install names the modules `qttools qtmultimedia qtsensors
  qtspeech` (see `aqt list-qt windows desktop --modules 6.12.0
  win64_msvc2022_64`): `qttools` carries `lupdate`, `lrelease` and Qt Linguist,
  and the last three are the application's sound, motion and speech, so this is
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
variant of `wjs/profiling.hpp` is the one in use. A push does not pay for them:
the client opens a network port, and vcpkg builds Tracy's tools for the host
(on Linux that is a chain of ports that needs autoconf and Python's venv). An unknown
name fails the run rather than running nothing. Jobs of a dispatched run are
named `dispatch - <leg>`, deliberately not the names of the required checks: a
run that skips legs would otherwise report "skipped" (which counts as a pass)
under a required check's name on the branch's head commit.

A branch other than `main` and `develop` gets no CI from a push; open a pull
request, or dispatch the workflow on it.

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
| `Analyze (C++)` | `codeql.yml` | no - informational |

Every workflow that produces a required check also triggers on `merge_group`,
because the merge queue evaluates the required checks on its temporary
`gh-readonly-queue/...` ref and GitHub only runs workflows that opt in there.
`branch-name-check.yml` and `commit-policy.yml` skip their job on a queue entry
(there is no head branch name, title or body of its own; a skipped job counts
as a pass for a required check). A workflow that produces **no** required
check must not carry `merge_group`, or every queue entry burns a run of it for
nothing: that is why `codeql.yml` does not have it.

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
| clang-format, gcovr | `==` in `.github/requirements/*.txt` | Dependabot (`pip`), weekly |
| CMake and Ninja (`lukka/get-cmake`) | `cmakeVersion` / `ninjaVersion`, exact, and one the pinned action's catalog lists | By hand, with the action |
| Qt (`jurplel/install-qt-action`) | The action's commit SHA, the Qt version, `win64_msvc2022_64`, the module list and the `aqtsource` commit, in `.github/toolchain-versions.json` | By hand: run `aqt list-qt windows desktop --modules <version> <arch>` **and** a real install before bumping, as the action's README says |
| vcpkg and the ports | `cmake/vcpkg-commit.txt` and `builtin-baseline` | Dependabot moves `builtin-baseline` (`vcpkg`, monthly); the commit file is by hand, and the two move together |
| Runner images | The label (`ubuntu-24.04`, `ubuntu-26.04`, `windows-2025-vs2026`, `macos-26`) | By hand |

Not pinned: the compilers (each is what its image ships), and anything an
image provides (`pwsh`, `jq`, `git`).

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
5. **Coverage**: gcovr refusing a counter; the report step passes
   `--gcov-ignore-parse-errors negative_hits.warn_once_per_file` for the known
   gcov bug, and the coverage preset updates counters atomically.

## Not here yet

clang-tidy (the `.clang-tidy` file exists; there is no job), the Release
presets on Windows and macOS (Linux's is the `Performance` leg), a coverage
threshold, Qt on the Linux and macOS legs (the application has only been built
on Windows so far), the Android/iOS targets (and with them the real Qt Sensors
and haptics backends; the desktops answer haptics with the shared no-op), and
web. Each is a new job or a flag flip, not a redesign.

Packaging is now built on the Windows leg only (above). The Linux `.deb`, `.rpm`
and `.tgz` and the macOS `.dmg` are configured and their generators are selected
where their tools exist, but no leg has built one: the Linux legs do not install
Qt, so the application is not in their packages, and no macOS host has built it
at all. A manual-dispatch Linux packaging leg would be the next step, and is
deliberately left out rather than added untested. NSIS is configured but not
verified in CI: the Windows leg asks for `-G ZIP`, and whether the runner has
`makensis` is not something the result depends on.
