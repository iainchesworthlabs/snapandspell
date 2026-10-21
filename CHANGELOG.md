# Changelog

*For people following what has shipped. The project overview is the
[README](README.md).*

All notable changes to this project are documented in this file, in the
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) format. This project
adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

WordJewelsSolver began in 2013 as a Windows-only Win32 and DirectShow program.
This is its rebuild as a portable C++23 CMake project. Nothing has been
released: the first release will be 0.1. The repository is public from before
that release, so that its rules (branch protection, required checks) can apply.

### Added

- **An iPhone and iPad build, installed ad hoc on the author's own devices.**
  iOS is a platform now (`src/platforms/ios`, selected by `cmake/Platform.cmake`
  before `APPLE`): a flat application bundle with its own `Info.plist` (the
  bundle identifier `org.iainchesworth.wordjewelssolver`, the macOS bundle's camera
  usage text, every orientation, a launch colour that follows the theme), an
  icon made from `assets/icon` (`tools/package/make_ios_icon.py`), and the
  `ios-release` preset (an Xcode project for an arm64 device with Qt 6.12's iOS
  kit, iOS 18 as the deployment target, no tests, no `wjs-cli`, Tracy off).
  `ios-build.yml` compiles it unsigned when a pull request touches what iOS
  reads; the `release.yml` job `Build the iOS ad-hoc IPA` stops at once, naming
  them, if any of the five Apple secrets is missing, checks the provisioning
  profile against them, archives and exports an ad-hoc IPA with Xcode, checks the
  IPA (`verify_ipa.sh`) and writes the over-the-air `manifest.plist`; the IPA and
  the manifest then get the same checksums, GPG signatures, attestations and SBOM
  entry as every other release file. Release notes link to `docs/install-ios.html`,
  a page of the documentation site that makes the `itms-services` link GitHub
  would strip. **None of it has run on a Mac, a hosted runner or a device**; the
  first run is expected to need fixes (see [CI, "iOS"](docs/ci.md#ios) for what is
  unverified, and [Installing on my iPhone/iPad](docs/ios-install.md) for the
  install, adding a device and the profile's expiry on 2027-07-16). The haptics are
  still the desktops' no-op, and Mode A reads the camera through Qt Multimedia.
- **A release pipeline: Release packages for every desktop, an SBOM, and GPG
  signatures.** `release.yml` turns a `v*` tag into a GitHub Release. It builds
  the Release presets on Windows (the Qt Quick application and `wjs-cli`, as a ZIP
  and an NSIS installer), Linux (ZIP, tar.gz, deb and rpm) and macOS (ZIP and disk
  image; Linux and macOS are the command-line solver until Qt is on those legs),
  runs all of ctest and a smoke test of every package, fails if Tracy or a
  sanitizer runtime is in a build or a package, writes a CycloneDX 1.6 SBOM and
  `SHA256SUMS`, signs every file with a detached GPG signature, attests build
  provenance and the SBOM with `actions/attest-*`, and publishes the release as a
  draft that is verified from the public side before it stays published. By hand
  or weekly the same workflow is a rehearsal that signs with a throw-away key and
  publishes nothing (`signing=repository` tests the real secrets). The key is made
  on the author's machine by `tools/release/new_signing_key.sh` (or `.ps1`), which
  prints the `gh secret set` commands; the workflow stops with those steps if
  `GPG_PRIVATE_KEY` or `GPG_PASSPHRASE` is missing or if the key is not the one
  whose fingerprint is committed in `.github/release-signing/`. Users check a
  download with `tools/release/verify_release.sh` (or `.ps1`), or by hand with
  `gpg`, `sha256sum` and `gh attestation verify`. A `release-tags` ruleset
  restricts who can create `v*` tags. `release-tooling.yml` lints the workflows
  (actionlint, ShellCheck) and self-tests the scripts. See
  [CI, "Releases"](docs/ci.md#releases).
- **Packages that ship with their own branding, and a Linux and macOS application.**
  The Windows executable has a version resource and the NSIS installer an icon, a
  desktop shortcut, a 64-bit install path and the links Programs and Features shows.
  The Linux application is distributed as an AppImage (linuxdeploy and its Qt
  plugin, a desktop entry, AppStream metadata and icons in
  `src/platforms/linux/packaging`); the deb and rpm are now the command-line
  solver's, no longer declaring a Qt 6.8 dependency that could not describe what
  the application needs. The macOS application is a bundle with its own `Info.plist`
  (identifier, languages and the camera usage text macOS requires), an icon and a
  signing path (`sign_and_notarize.sh`, gated on six Apple secrets) with
  `verify_bundle.sh` to check it. A prerelease is packaged under its own version
  (`WJS_PACKAGE_PRERELEASE`), a source archive is attached to every release, and a
  development package carries Tracy's licence in its notices.
- **cpack refuses to package a Tracy build, and a Release package is checked for
  development leftovers.** `cmake/PackagePreflight.cmake` stops cpack when the tree
  has the profiler in it (a development package has to be asked for with
  `-D CPACK_WJS_ALLOW_DEV_PACKAGE=ON`); the Release presets now build the
  application and turn the sanitizers off; `check_package.ps1` fails a Release
  package holding a `TracyClient` library, an AddressSanitizer runtime or a `.pdb`;
  and a `Package (Windows, Release)` job in `ci.yml` does that check on every push.
  The Debug leg's artifact is now `wordjewelssolver-windows-x64-debug-dev`.
- **A documentation site.** `docs.yml` builds the site with
  `mkdocs build --strict` and a configuration that makes broken links to pages and
  headings failures, and publishes it to GitHub Pages on every push to `main` that
  changes the docs (the repository's Pages source must be GitHub Actions, once). `.github/release.yml` gives GitHub's generated
  release notes their categories.

- **The test suite as four tiers, with a coverage gate and a UI coverage
  report.** The Catch2 and CTest entries are now selected as `unit`
  (`ctest -L unit`), `integration` (`ctest -R "^integration\\."`), `system`
  (`ctest -L system`) and `ui` (`ctest -L ui`), with `quality` for the checks
  about the tree itself; test presets (`windows-msvc-debug-unit`, ...) make each
  one word. The system tier is new: `wjs-cli` is run as a program (help,
  version, the README board, every option and every exit code), the application's
  start-up and headless smoke run are run as a program, and
  `cmake --install` is run into a throw-away prefix where the installed `wjs-cli`
  solves a board from the installed dictionary. Coverage is no longer report-only:
  `coverage-unit` (gcovr on GCC, llvm-cov on clang-cl) holds `src/lib` to **90%
  of lines and 80% of branches** and fails below either, and `coverage-ui` runs
  the Qt Quick Test suites under the same instrumentation and reports `src/app`,
  the C++ the QML drives. The Windows coverage preset is
  `windows-clang-coverage`; the Linux gate is the `Coverage (Linux, GCC)` CI job.
  See [docs/testing.md](docs/testing.md#coverage).
- **Mode A's camera on the desktop: a live preview and one capture.** The
  Family Assistant's ready screen now shows the camera's own picture in its
  frame and its **Capture** reads one frame on the device. The camera is a new
  `CameraSource` QML element (`src/app/camera`) over a `CameraBackend` seam with
  the repository's optional-module variant pattern: `camera-multimedia` builds
  `QCamera` → `QMediaCaptureSession` → `QVideoSink` where the kit has Qt
  Multimedia, and `camera-none` answers `no_camera` where it does not, with no
  `#if` in a source and the same screens either way. The newest video frame is
  converted on capture with `QVideoFrame::toImage()` and a tested conversion to
  the scanner's owned RGB24 frame (R, G, B, packed rows); it is read through the
  same `board_reader` and dropped, and the camera is stopped before the read.
  `ScanModel` uses the source when the application supplies one, keeps its
  patience for a camera that never delivers (`no_frame`, back to `ready`), and
  maps no device, a refusal and a failure to the wording file's three sentences
  with "Type in your board" beside them; no frame is ever faked, saved, sent or
  kept. `Main.qml` makes the source and hands it to the model; `ScanScreen`
  shows a `VideoOutput` only while it runs. A kit without Qt Multimedia builds
  the none variant and the no-camera screen. The camera's Qt Multimedia plugins
  and `qml/QtMultimedia` are deployed into the package, and the package check
  requires them. Tested without a camera: the conversion, the availability,
  refusal, failure and no-frame states, the model with a fake source, the
  offscreen `camera.screens` walk with a synthetic board, and the QML preview
  path; **this machine has no camera**, so a live device, real dice and the
  iPad's AVFoundation/Vision backend remain unverified. See
  [docs/design/scanner.md](docs/design/scanner.md#the-qt-multimedia-desktop-backend),
  [docs/building.md](docs/building.md#the-qt-quick-window) and
  [docs/testing.md](docs/testing.md#the-camera).
- **The real feedback engines: sound, speech, motion and haptics.** The
  `Feedback.cue(kind, detail)` seam the screens and models already emitted now
  reaches a `FeedbackEngine` (`src/app/feedback`) that plays a short asset
  through Qt Multimedia, says a whole word or a picture's name through the
  device's own Qt TextToSpeech engine, and asks the platform's haptics
  interface for a tap. The mapping is a pure policy, `wjs::plan_cue`
  (`src/lib/include/wjs/feedback.hpp`), tested without a speaker: the master
  **Sound** switch silences every asset and spoken word, **Celebration sounds**
  full/quiet/off decides the cheer, chime and cleared notes, **Sound it out**
  full/word/off decides whether a found word is said, and **Vibration** off
  drops every haptic. A build without the three Qt modules selects silent
  backends by CMake (the variant pattern); the application still configures,
  builds and plays. The eight cues are placeholder WAVs owned by the project
  (`assets/sounds`, written deterministically by `tools/sounds/make_sounds.ps1`;
  `data.sounds` holds them to it), and the real phoneme clips and cheers wait
  for the reviewer's recording job: `say_letter_sequence()` is the seam and is
  deliberately silent. Motion comes from `MotionSource` over Qt Sensors
  (accelerometer and gyroscope at the platform's highest rate), fed to the
  scene's `pushMotion` and through the pure `wjs::MotionProcessor`; a real shake
  rolls the box like the button, and on the desktops the source is unavailable
  and the drag stands in. Haptics is a new platform interface
  (`src/platforms/interfaces/wjs/platform/haptics.hpp`) with a shared no-op on
  the three desktops; Android and iOS implementations wait for those targets.
  No microphone, no capture: speech is output only. See
  [docs/design/application.md](docs/design/application.md#audio-and-touch-feedback),
  [docs/building.md](docs/building.md#the-qt-quick-window) and
  [docs/testing.md](docs/testing.md).
- **The Qt install on the Windows CI leg moved to `install-qt-action`.** The
  PyPI `aqtinstall==3.3.0` line could not install Qt 6.12 on Windows: Qt 6.11+
  split the Windows repository per architecture, and 3.3.0 cannot read it. The
  leg now uses `jurplel/install-qt-action` (pinned by SHA) with `aqtsource` set
  to the aqtinstall master commit that carries PR #1000, the modules `qttools
  qtmultimedia qtsensors qtspeech`, and `cache: true`. The pins live in
  `.github/toolchain-versions.json` and `toolchain.versions` holds the workflow
  to them, including that the old pip line cannot come back. See
  [docs/ci.md](docs/ci.md#toolchain-choices).
- **The toolchain pins in one file, held to the workflows.** `.github/toolchain-versions.json`
  lists CMake, Ninja, the Qt version, architecture, modules, install action
  and aqtsource, and names the vcpkg pin file. A new `toolchain.versions` ctest
  (`tests/toolchain/check_versions.cmake`) reads it and fails if
  `.github/workflows/ci.yml` pins another CMake or Ninja, installs another Qt
  or aqtinstall, or if `vcpkg.json`'s `builtin-baseline` stops matching
  `cmake/vcpkg-commit.txt`. It runs on every host in every leg, since it only
  reads the files. See [docs/ci.md](docs/ci.md#toolchain-choices).
- **The interactive QML, tested at run time: eight Qt Quick Test suites,
  headless.** The house standard's remaining gap: `qmllint` and the two
  offscreen stub-shell walks held the QML to its text and its flow, but nothing
  pressed a button. `wjs_quick_tests` (`tests/qml/`) now drives the application's
  own kit and screens under Qt's offscreen platform with the software renderer,
  so no suite needs a display or Qt Quick 3D. The QML under test is the
  application's files aliased into the test's QML module, as the stub shells do;
  the board scene is a stub of the same interface (`tests/qml/scene_stub`) that
  records the `play(board, seed)` it is given, because the real one is 3D. The
  harness sets the platform before the application is created, gives the suites
  the application's own models (`SoloModel` over `FakeSoloGame`,
  `ParentController` over the demo store and phases with a clock the test
  advances, `ScanModel`, `AssistModel` and `LanguageManager`) as context
  properties, and fails a suite on any Qt warning. The suites: **the kit**
  (AppButton's 48/64/96 targets, release-inside, keyboard, press cues;
  HoldButton's hold; TileFace's states; Jewel, Icon and Feedback), **the theme**
  (light and dark tokens, the colour scheme, the text scale, reduced motion),
  **Home**, **the box** (the sizes that fit, Shake and Space, the scene's
  `play(board, seed)`), **the panel** (the tray, a word found again, the hint,
  Done, the history), **RTL** (Hebrew and Yiddish: the bars' way back and the
  panel's leading edge mirror while the board's geometry and the check screen's
  replica do not), **the scan screens** (no camera, the typed board, a
  correction, doubtful tiles, Yes play) and **the parent area** (the gate's
  two-spot hold, the dashboard's numbers, the settings rows, the reset). One
  ctest entry per suite, label `gui`, TIMEOUT 300 and `QT_QPA_PLATFORM=offscreen`,
  so `ctest -R quick` is the tier and the Windows CI leg runs it with everything
  else. See [docs/testing.md](docs/testing.md#the-interactive-qml-qt-quick-test)
  and [docs/ci.md](docs/ci.md).
- **Install rules, CPack packages and third-party notices.** A build now has an
  install step: `install()` puts `wjs-cli`, `WordJewelsSolver`, the English
  dictionary, the standard board's dice (`dice/3x3.txt` to `6x6.txt`),
  `NOTICES.txt` and the GPL text (`LICENSE.txt`) into an install tree, and
  `cmake/Packaging.cmake` makes one CPack package of both programs (no
  per-program components: the window cannot run without the dictionary and the
  dice, and the command-line solver is small). A **ZIP is always offered**; the
  platform's own formats are added by the fragments in `src/platforms/<os>/`
  only where their tools are installed - **NSIS** on Windows when `makensis` is
  found (never a failed configure without it), **DragNDrop** on macOS, and
  **TGZ, DEB and RPM** on Linux when `dpkg-deb`/`dpkg-shlibdeps`/`rpmbuild`
  are - so a machine with no extra tools still gets a package. The installed
  window is made self-contained with Qt's QML-aware install-time deploy script
  (`qt_generate_deploy_qml_app_script`): its Qt DLLs, plugins and QML modules,
  including the Quick 3D plugin, are deployed beside it (Linux keeps the
  system's Qt; macOS carries Qt in the bundle). New **`package-windows-msvc-release`**,
  `package-linux-gcc-release` and `package-macos-clang-release` presets run
  `cpack --preset`, and the packages land in `build/<preset>/packages/`.
  **Third-party notices** are assembled at configure time by
  `cmake/Notices.cmake` from `notices/fragments/` into `NOTICES.txt`: the
  project's GPL-3.0-or-later licence and the **application binary's
  GPL-3.0-only** licence because it links Qt Quick 3D, Qt 6.12 under the
  LGPL-3.0/GPL-3.0 options with the LGPL text and the Quick 3D section, the
  public-domain ENABLE word list, the translations, the project's own light
  probe, no bundled font, and Catch2 (BSL-1.0) as build-and-test only. The file
  is installed beside the programs and embedded as `:/notices/NOTICES.txt`; the
  About screen's **Licences** section now keeps its two sentences and adds a
  scrollable view of the full notice. New checks: `notices.sections` (the
  notice states every required section and leaves no placeholder),
  `package.config` (CPack names the project and its licence, and the install
  rules carry both programs, the data and the notices), the packaged resource
  and notice asserted by `parent.screens`, and
  `tools/package/check_package.ps1`, which makes the ZIP, checks its required
  files, writes `SHA256SUMS`, solves a board with the packaged `wjs-cli` and
  runs the packaged `WordJewelsSolver --smoke 10` offscreen. The Windows CI leg
  runs that script after its tests and smoke and uploads the ZIP. See
  [docs/building.md](docs/building.md#packaging),
  [docs/ci.md](docs/ci.md) and [docs/licensing.md](docs/licensing.md#notices).
- **The interface's languages: English, French, Spanish, German, Yiddish,
  Hebrew, Chinese and Japanese, with right-to-left and left-to-right layouts.**
  A new **Language** row in the parent area offers the eight, each named in its
  own script, plus System (the device's own language when it is one of the
  eight, English otherwise); the choice is kept in the store as `language=` and
  read before the window is built. `LanguageManager`
  (`src/app/language_manager.*`) installs the Qt catalogue, sets
  `QGuiApplication::setLayoutDirection` and calls `QQmlEngine::retranslate()`
  when the parent changes it, so the screens change at once. One Qt Linguist
  catalogue per language is in `i18n/`, generated from the QML with `lupdate`
  (`wjs_update_translations`) and compiled by `lrelease` at build time into
  `:/i18n/wjs_<code>.qm`; English is the source language and has a catalogue
  too. Every child- and parent-facing string moved into its area's wording file
  (`ParentWording`, `ScanWording` and the new `SoloWording` and `HomeWording`).
  Hebrew and Yiddish mirror the window (`LayoutMirroring`), the bars, the parent
  rows, the gate and the two-thirds/one-third split; the board, the dice, the
  trace path and the 2D/3D scenes never mirror, and the board's letters, the
  word list and the phonics content stay English. Fonts use Qt's per-glyph
  fallback: Andika is still not bundled, and no CJK faces are (system fallback,
  documented). Checks: `i18n.catalogues` (lupdate drift), the catalogue
  completeness and direction cases and the glyph check in `wjs_app_tests`, and
  `parent.languages` / `scan.languages`, the screens loaded offscreen in all
  eight. **The translations are mechanical and unreviewed**: they are listed
  with the phonics content for the literacy/reviewer pass, which will replace
  them.
- **Mode A, the Family Companion: the scan, check and assistant screens.** The
  family game's picture on `home` is now shown and opens the camera-assisted
  mode: `scan.ready` (a frame to put the real board in, Capture and "Type in
  your board"), `scan.reading`, `scan.check` (the replica with the reader's
  doubtful tiles ringed and a `?`, a letter picker that offers the reader's
  runners-up first, Turn, the board size, Read again and Yes, play) and
  `assist.playing` (the replica in the leading two-thirds and the panel — Words
  I found, Earlier boards, Show me a start, New board). `src/app/scan` holds the Qt-free port
  `AssistGame` (a confirmed `wjs::Board` over `wjs::classify_path`, `FoundWords`,
  `wjs::start_tile_help`, and the board recorded to the store on leaving, in the
  standard board's form with `viable_total` and no phase) and the QML models
  `ScanModel` (the `BoardDraft`, its doubts and alternatives, the size, and the
  camera's lifecycle) and `AssistModel` (tiles, the traced path, the tray, the
  start tile, the cues). The screens are the QML module
  `WordJewelsSolver.Scan` (`src/app/qml/scan`, with every sentence in
  `ScanWording.qml` for the later i18n pass); `Main.qml` shows them as the
  `scan` and `assist` screens, Back goes where the person came from and New
  board starts a new scan. A captured frame goes through the platform's
  `board_reader` and is dropped at once; the camera is started only while
  `scan.ready` is on screen, and no frame is kept or sent. **The camera backend
  is still `unavailable` on every desktop and the iPad backend is not written**,
  so the typed-board fallback is the path a person takes there: Type in your
  board opens the check screen with an empty 4x4. Tests: `assist_game_test.cpp`
  (viable, real-not-today's and not-a-word, the hint's tile, the tray and the
  store record), `scan_model_test.cpp` (the empty draft, corrections, a full and
  an incomplete grid, the doubtful tiles, and no camera), and `scan.screens`, an
  offscreen stub shell (`tests/app/scan_ui`) that walks the flow and fails on any
  QML warning; `app.qml_lint` now lints the new module too.
- **Qt performance and profiling: the window, QML and the 3D and 2D scenes.**
  The application now measures its own frames and the parts that can cost one.
  `src/app/frame_profiler.cpp` sends the frame mark `UI`, the plots
  `ui.frame_ms`, `ui.first_frame_ms`, `render.frame_ms` and `ui.sync_ms`, names
  the render thread, and keeps a rolling p50/p95/p99/max report. New zones cover
  `app.startup`, `app.load_qml`, `app.scene_create` (from QML), `data.load_phase`,
  `scene.step` (the dice world, carrying its simulation steps), `scene.prepare`
  (the atlas, carrying its cells), `scene.play`, `scene.show`, the per-frame
  `scene.shake`/`scene.drop`/`scene.settle`, `scene.celebrate`, `ui.trace`,
  `ui.screen`, and the 2D fallback's `scene2d.*`. A QML singleton `Profiler`
  (`src/app/scene/profiler.cpp`, in `WordJewelsSolver.Scene`) opens and closes a
  named zone from QML over the wrapper's new run-time `RuntimeScope` (a
  transient zone, so the location says `QML`): empty without Tracy, real zones
  with it. `--smoke
  [frames]` runs the window as a capture workload, prints the frame report and
  leaves with 0 (a profiling build gives a profiler that was started first up to
  five seconds to attach, so the start-up zones are captured; a window that
  cannot present a frame is said, not faked). The performance tier gains the
  scene's work budgets (a 4x4 play's simulation steps and an atlas's cells, in
  `tests/perf/budgets.hpp` and `tests/app/shake_script_test.cpp`), an opt-in
  frame check (`tests/app/frame_timing_test.cpp`, `WJS_PERF_FRAME_TIMING=1`),
  and `tests/profiling/check_zone_names.cmake` now checks the QML seam's names
  and the plots' and frame sets' too. See
  [docs/profiling.md](docs/profiling.md) and
  [docs/design/performance.md](docs/design/performance.md).
- **The real 3D dice shake in Solo Play (Mode B).** The prototype's scene is now
  the application's: the QML module `WordJewelsSolver.Scene` (`src/app/scene` and
  `src/app/qml/scene`) holds `BoardScene.qml`, `Die.qml`, `FaceAtlas.qml`,
  `ShakeScript.js`, the baked light probe and the dice objects (`DiceSim`,
  `DieGeometry`, and the `wjs_dice_world` simulation), and both `WordJewelsSolver`
  and the `ShakePrototype` harness link it, so there is one scene and one script
  and not a copy. The 2D placeholder is replaced as the scene Solo Play uses and
  is kept, not built, at `src/app/qml/scene/fallback/` for D2. `SoloBoard` gains
  the six faces of each tile's die and the face that is up: `RealSoloGame::board()`
  takes them from `wjs::roll_with_placements` with the dealt board's seed on the
  standard board, so the dice tumble to the faces the roll gave and land on the
  one that is up; the phonics board, a board the store brought back and the fake
  game carry none, and the scene draws the tile's letter on the face that is up.
  The scene gains `prepare(board)` (declared in `app.scene_contract` with
  `overdraw`), which draws a board's faces a frame before `play`/`show` of it;
  `Main.qml` keeps `SoloScreen` alive from start-up, so the `View3D` is made once
  behind home and not on the Shake press. A fast drag on the box stands in for the
  accelerometer (Qt Sensors is not in the tree): it sends the drag's acceleration
  and spin to the scene's `pushMotion` and starts a play unless the parent's Shake
  setting is off, and the Shake button and Space are always there. Reduced motion
  keeps the box still and cross-fades the lid and the dice, with no tumble. The
  application now links Qt Quick 3D and so is **GPL-3.0 only**, which the docs
  record in the places that said only the prototype did. See
  [docs/design/shake-prototype.md](docs/design/shake-prototype.md) and
  [docs/architecture.md](docs/architecture.md#the-qt-quick-window).
- **The GPL-3.0-only Quick 3D profiler plugin kept out of the deploy.** On
  Windows, `windeployqt` was copying the QML debugger's
  `qmldbg_quick3dprofiler` plugin (and `Qt6Quick3DUtils.dll`, which it depends on)
  beside every program it deployed, whether or not the program linked Quick 3D.
  The platform fragment now passes `--skip-plugin-types qmltooling`, so the whole
  debugger plugin type is left out; a program that does link Quick 3D still gets
  `Qt6Quick3DUtils` as part of that module's own runtime, which the application
  now does and may, being GPL-3.0 only. See
  [docs/design/shake-prototype.md](docs/design/shake-prototype.md#deploy).
- **A Colour scheme preference: System, Light or Dark.** `wjs::Settings` gains
  `colour_scheme` (`src/lib/include/wjs/settings.hpp`, written to the store as
  `colour=` by `store_format.cpp`), the parent area's settings screen offers it
  (`SettingsScreen.qml`, `ParentWording.qml`), and `Main.qml` binds it to
  `Theme.colourScheme`: System (the default) follows the operating system's
  light or dark setting, Light and Dark are the parent's choice for every
  screen's chrome. `Theme.dark` is derived from it, so the Solo Play screens,
  the box and the parent area are all in the same scheme; `ParentTheme` gains
  dark values (the kit's measured pairs) so the parent area follows too.
  `SystemColourScheme` gains its tests (`tests/app/system_colour_scheme_test.cpp`),
  and the parent area's board-size choice offers 3x3 to 6x6 on the standard
  board. See [docs/design/application.md](docs/design/application.md#colour-and-contrast).
- **The window plays the real standard board by default.** `SoloGameSetup`
  (`src/app/solo/solo_game_setup.*`) now always loads the English dictionary and the
  project's dice (`data/dice`, deployed beside the executable by `src/app/CMakeLists.txt`),
  and loads a phase directory only when the program is started with `--phase-directory
  <dir>`; `main.cpp` always builds the real game and never the fake. `RealSoloGame`
  plays the session's mode as given: the standard board, a roll of the dice at 3x3 to 6x6
  with no grapheme blocks and no dead jewels, by default, and the phonics board when the
  parent chose it and a phase was loaded. The fake (`fake_solo_game.*`) stays for the
  tests. The size the box offers is the mode's: 3x3 to 6x6 on the standard board, 3x3 and
  4x4 on the phonics board, and `Settings::normalized()` (`src/lib/src/settings.cpp`)
  holds the parent's board-size setting to the same two ranges. A bad `--phase-directory`
  still stops the program with exit code 2. See
  [docs/architecture.md](docs/architecture.md#the-qt-quick-window).
- **The store keeps a standard game in progress.** `Store::save_session`
  (`src/lib/src/store.cpp`) accepts a saved board whose record lists no words and carries
  the count (`viable_total`), which the store's file (format version 2) writes as the
  saved board's `-` list; a version 1 file is migrated (the step rewrites nothing, as a
  version 1 file could not hold a standard saved board) and kept as `store.wjs.v1` beside
  the new one. A standard game is brought back by `GameSession::restore`, which re-solves
  the board and checks the count. See [docs/design/parent-area.md](docs/design/parent-area.md#the-store).
- **The standard board's dice are shipped with the window.** A build of `wjs_app` copies
  `data/dice/3x3.txt` to `6x6.txt` into a `dice` directory beside the executable, as it
  already does for the dictionary, and `SoloGameSetup` reads them with
  `DiceSets::load_directory`.
- **Dice with six real faces, from the library to the scene.**
  `wjs::roll_with_placements` returns the die on each tile and the face of it
  that is up, with the board (`wjs::roll` is the same roll and gives the board it
  always gave). The 3D scene prototype is given the six faces of each die and
  which is up: a die carries its six different letters, shows other faces as it
  is shaken, and lands with the rolled one up, the letter upright. A dead jewel
  is a die of six jewels. Tested in the library (the faces named are the tiles'
  letters, for 300 boards of each size) and in the scene's script. See
  [docs/design/shake-prototype.md](docs/design/shake-prototype.md#real-faces).
- **A cheaper new board in the 3D scene prototype.** The frame in which a board
  is drawn with its six faces to a die was two to three and a half times what it
  had been. The faces' texture is now drawn once for all the dice that read it,
  its cells are kept from one board to the next, the jewel is drawn once, and an
  optional `prepare(board)` draws a board's faces a frame before `play()` or
  `show()` of it. On the author's desktop, `show()` of a 6x6 board to its first
  frame went from 61 ms to 19 ms with the library's dice (78 ms to 19 ms with
  the stand-in board), and every size is under the 33 ms budget; the first board
  of a size is the dearest (31 ms at 6x6). Nothing was measured on an iPad. See
  [docs/design/shake-prototype.md](docs/design/shake-prototype.md#the-real-faces-made-cheaper).
- **The Solo Play screens of the game** in Qt Quick (`WJS_BUILD_APP=ON`):
  `home`, and `solo.box`, `solo.playing` and `solo.resting` with the moments
  between them, the board and the panel split two thirds to one third at any window
  shape, trace and tap in order both always on with a loose tolerance, a button for
  every shake, 48, 64 and 96 dp targets, a way back on every screen, theme tokens
  in place of any colour or size in a screen (light, dark, reduced motion), a calm
  resting screen with nothing that asks the child to go on, and no text drawn into
  a picture. The board scene is one component behind its interface with a 2D
  placeholder; the game is `SoloGame`, a Qt-free port, answered by the library's
  session or, without a phase, by a fake. It replaces the window that typed a board into a
  grid; `SolverModel` stays with its tests. There is no sound, no device shake and
  no bundled font yet. See [docs/architecture.md](docs/architecture.md#the-qt-quick-window).
- **The Solo Play screens over the library's session**: `RealSoloGame` (`src/app/solo`) is the
  port over `wjs::GameSession`, handing the session's records to the store after each find and
  when the lid goes on, and reading the best, the first-time sticker and the history from it.
  The program plays it only when started with `--phase-directory <dir>` (with `--phase`,
  `--store`, `--seed`): the repository has no phase, because the phase files have to be written
  and reviewed by a person qualified to do it, so without one the fake is played as before.
  See [docs/architecture.md](docs/architecture.md#the-qt-quick-window).
- **A Solo Play run begins where the last one ended**, when the real session is played: the game
  in progress (a board on the table with its finds and the lit start tile, or the resting
  screen) is kept in the store after each change and brought back by
  `GameSession::restore()`, and a record that cannot come back (another phase, a changed
  board, a bad snapshot) quietly starts a new game in the box. The screens open on a board
  already down, with no shake, and the size the child chooses on the box is written back to
  the settings as the last used. See
  [docs/architecture.md](docs/architecture.md#the-qt-quick-window).
- **The parent area, reachable from the Solo Play window**, on one store: the corner button
  on home and in the box goes to the gate (the parent area's own, which nothing in the window
  opens), then the dashboard, the settings and about, and Back returns to where the person
  came from. The program opens one `wjs::Store` (`--store <file>`, or the per-user file, with a
  phase or without) and the game and the parent area work on it, so a setting changed there is
  what the next game is made from (the help switch, the size to offer first, the sounds in
  focus) and a find the game recorded is on the dashboard. The Motion setting and the Text
  size are applied to every screen through `Theme`: Reduced forces reduced motion on, and
  System follows the operating system's own reduced-motion preference (as Qt reports it, and
  while the program runs). The system's text size is not read, because Qt 6.12 does not expose
  it, and a change of the Phase setting is seen by the next run. See [docs/design/parent-area.md](docs/design/parent-area.md#integration).
- **The parent area, on the device and nowhere else**, inside the first release's
  frozen privacy rules (no voice, no export, no network, no time played, no limit):
  a small versioned **store** (`wjs/store.hpp`) of finds, the last twenty boards, stars,
  bests, the vault, day tallies, settings and the game in progress, kept in one file in
  the per-user data directory (`wjs::platform::user_data_directory()`), written whole and
  renamed into place, migrated from older versions with the old file kept, and set aside
  whole and never overwritten if damaged; the **parental gate** (`wjs/parent_gate.hpp`):
  two fingers on two circles for two seconds, with an accessibility route and Space and
  the mouse on a desktop; the **dashboard's numbers** (`wjs/dashboard.hpp`): this week,
  the sounds of the active phase (met, found, unassisted), the vault and the best; and the
  **screens** (`WordJewelsSolver.Parent`: the gate, the dashboard, the settings, about,
  licences and privacy) with every sentence a parent reads in one file for the literacy
  review. Privacy checks run in CI: a scan of the source for audio capture, the network
  and export routes, and a held-to-cases checker for it; the store's size cap and what
  it may hold; the wording. The copy between the game session's values and the store's
  (`wjs/session_store.hpp`) brings a stored board back in the form `GameSession::restore()`
  needs. See [docs/design/parent-area.md](docs/design/parent-area.md).
- **The first working slice of Mode A's board reader** (`wjs::scan`, `src/scan`): finds a
  board in a camera frame (the largest convex quadrilateral of a lattice of dice, drawn
  square, its cells counted and centred), reads each die's one rotated letter by matching it
  against the project's own stroke font, and answers a letter, a confidence, runners-up and a
  doubt for each tile. `BoardDraft` is the confirm-and-correct model the check screen binds to
  (the solver sees only a board a person has confirmed). It has no dependency of its own, and
  the `camera` and `board_reader` interfaces under `src/platforms` (no camera on the desktops
  yet; the portable reader). Held to a numeric accuracy target on a synthetic corpus drawn by a
  generator in `tests/scan/synth`; no recorded frame is or will be committed. **Measured on
  synthetic frames only: nothing has been run on a camera, an iPad or real dice.** The iOS camera
  and Vision reader are written down and not built. See
  [docs/design/scanner.md](docs/design/scanner.md).
- **A prototype of the 3D board scene**, built only with
  `-DWJS_BUILD_SHAKE_PROTOTYPE=ON` (and `WJS_BUILD_APP=ON`): the shake, the lid
  and the drop in Qt Quick 3D behind a one-component interface. The dice are
  rigid cubes in a box, simulated from first principles by a first-party C++
  library (`wjs_dice_world`, no Qt, checked against closed-form physics by
  20 test cases), driven by the device's accelerometer and gyroscope samples
  (a seeded stand-in without a device) and guided to the cells of a board that
  was decided before the play. A reduced-motion variant, a mouse drag standing
  in for the accelerometer, frame timing, and a light probe baked by
  `tools/lightprobe`. The program links Qt Quick 3D and so is GPL-3.0 only. See
  [docs/design/shake-prototype.md](docs/design/shake-prototype.md).
- **A solver library with no GUI and no operating system code** (`wjs::lib`).
  It parses a board (one letter per tile, or tiles of one to three letters
  separated by spaces or commas so that the Boggle "Qu" face works, up to 16 by
  16), holds a word list as a trie, scores words, and finds every word on the
  board by a depth-first search that reports the tiles each word is spelt by.
  Two scoring schemes: the printed Boggle scores, and the letter-rarity score
  of the original program. See [docs/architecture.md](docs/architecture.md).
- **`wjs-cli`, a command-line solver.** `wjs-cli [-d <file>] [-m <n>]
  [-s boggle|rarity] <row>...` prints each word, its score and its path, and
  the totals. Without `-d` it uses the English dictionary beside the
  executable. See [docs/index.md](docs/index.md#using-the-command-line-solver).
- **A small Qt Quick window**, built only with `-DWJS_BUILD_APP=ON` and Qt 6.12:
  a grid to type a board into, a Solve button and a scored word list. The model
  behind it has unit tests; the window itself has been started offscreen and
  not otherwise tested. See [docs/building.md](docs/building.md#the-qt-quick-window).
- **Per-operating-system code in one place** (`src/platforms`): the path of the
  running executable for Windows, Linux and macOS, and a UTF-8 console for
  Windows. `cmake/Platform.cmake` is the only place the target system is
  decided; each platform's `platform.cmake` says what it compiles. See
  [docs/platform-isolation.md](docs/platform-isolation.md).
- **A platform-isolation check**, `tools/checks/check_platform_isolation.ps1`
  (also the `platform.isolation` test): no preprocessor conditionals or
  `#define`s in first-party C++, no OS headers or OS-querying calls outside the
  platform section, no OS tests in CMake outside the selector, and every
  platform file wired into a `platform.cmake`. A second test,
  `platform.fragments`, holds every platform's `platform.cmake` to the contract
  on whichever system runs it.
- **Unit tests** (Catch2) for the board, the dictionary, scoring, the solver and
  the platform layer. See [docs/testing.md](docs/testing.md).
- **CMake presets** for Windows (MSVC), Linux (GCC, with a coverage preset) and
  macOS (Clang); the dependency provider, vcpkg, is a shared clone found through
  `VCPKG_ROOT`, checked against `cmake/vcpkg-commit.txt`. Every
  first-party target builds with warnings as errors, and Debug builds run under
  AddressSanitizer (and UBSan on GCC and Clang). See
  [docs/building.md](docs/building.md).
- **The licence and the project documents**: the GPLv3 text (`LICENSE`; the
  project is GPL-3.0-or-later), `README.md`, `CONTRIBUTING.md`,
  `CODE_OF_CONDUCT.md`, `SECURITY.md` and the documentation under `docs/`, built
  with mkdocs-material. See [docs/licensing.md](docs/licensing.md).
- **An English word list** (`data/dictionaries/en.txt`): ENABLE, unmodified, with
  its source and SHA-256 in [data/README.md](data/README.md). A build copies it
  beside the program, so `wjs-cli` and the window work with no flag.
- **Child-facing word lists** (`data/lexicon/`): ten categories (animals,
  colours, shapes, food, body, family, nature, home, clothes, transport) and a
  237-word early-reader tier, each a subset of the dictionary and held to that by
  `tools/data/check_wordlists.ps1` (also the `data.wordlists` test). Nothing uses
  them yet, and they have not been reviewed by a teacher or a parent.
- **Continuous integration and repository rules**: workflows for formatting,
  platform isolation, Windows/Linux/macOS builds and a coverage report; a
  commit and pull-request text policy; repository rulesets as code; and local
  hooks. None has run yet. See [docs/ci.md](docs/ci.md).
- **Profiling with Tracy, off by default** (`wjs/profiling.hpp`). The
  repository allows no `#define` or `#if`, and Tracy's interface is macros, so
  the library has a macro-free wrapper in two variants (`zone<"name">()`,
  `frame_mark()`, `plot()`, `message()`, `thread_name()`), one of which has
  Tracy behind it and one nothing, selected by CMake's include path. The
  `*-profiling` presets turn it on (`WJS_ENABLE_TRACY`, a vcpkg `profiling`
  feature with the client and the capture and export tools); nothing that ships
  has it, which `profiling.shipped_presets` holds. The solver and the
  dictionary load carry the first zones, and `wjs-cli` has `--repeat <n>` so
  that a profiler has time to attach. See [docs/profiling.md](docs/profiling.md).
- **The game session of Solo Play** (`wjs::GameSession`, `game_session.hpp`): the
  state machine of Mode B from the closed box to the resting screen, with no Qt,
  no clock and no platform code. It deals a seeded board (seeding again, up to
  five times, to hold three to five words of the phase), takes a traced path and
  answers it (a word of today's, a real word that is not today's, or not a word,
  with no word named for the last two and a real word never called "not a
  word"), awards stars by tier, lights the start tile on request, reports the
  board cleared and then resting, and hands the store a `BoardRecord` and a
  `SessionSnapshot` it can restore. Persists nothing. See
  [docs/architecture.md](docs/architecture.md#game-session).
- **The game session plays the standard board**, the default of the first release
  (decided 2026-10-16), as well as the phonics board: `SessionOptions::mode` (the
  parent's switch, `wjs::BoardMode`, default standard) and `SessionData`, which holds
  the dictionary, the new `DiceSets` (the project's `data/dice`, one set for each side
  from 3 to 6) and the phase. The standard board is a roll of the dice at 3x3 to 6x6,
  rolled again until it holds a word (`wjs::deal_standard_board`), against the full
  dictionary, so **every dictionary word of three letters or more counts and is named**,
  with no block list, as the author accepted for the MVP. The states, trace, stars, help,
  record and snapshot are the same; `BoardRecord` and `SessionSnapshot` say the mode, and a
  standard record counts its words (`viable_total`) instead of listing them. `restore`
  refuses the other mode (`wrong_mode`). `GameSession::create` and `restore` take a
  `SessionData` in place of a phase and a dictionary. The screens still play the phonics
  board only, and `Store::save_session` does not yet keep a standard game in progress.
  See [docs/architecture.md](docs/architecture.md#game-session).
- **A performance-test tier** (`wjs_perf_tests`, `tests/perf`): exact work
  budgets on what the solver does (visits, words, trie nodes, and a digest of
  every result) over a fixed corpus of 800 random boards and six built ones,
  which run in every optimised build and skip in Debug, and wall-time checks as
  a ratio to a calibration workload, which run only on request. The solver has a
  work counter (`wjs/solver_work.hpp`) for it. A Linux Release leg in CI runs
  it. The code that makes a board has its zones (`board.roll`, `board.seed`,
  `board.verify`) and work budgets too, and so has the grapheme binder
  (`phonics.bind`, `wjs/binder_work.hpp`): each is for one board mode, the
  standard board (dice) being the default and the phonics board a parent's
  switch. See [docs/testing.md](docs/testing.md#performance-tests) and
  [docs/design/performance.md](docs/design/performance.md#the-game-rows).

### Changed

- **The nightly jobs install Tracy's Linux tools and use the vcpkg cache.** Every
  Debug preset has Tracy on, so the nightly `clang-tidy`, `Sanitizers (Linux,
  Clang)` and CodeQL jobs, which configure Debug presets, need the autotools
  packages vcpkg builds Tracy's host tools with. They now run the same composite
  action as the `Linux (GCC)` leg. `clang-tidy` and CodeQL now configure a new
  `linux-gcc-analysis` preset (Debug, no tests, no application, Tracy and sanitizers
  off), which installs nothing through vcpkg, and a scheduled run scans `develop`
  rather than `main`.

- **The second post-overhaul pass: the board's clock, the full-height panel, a
  resume, the About and Licences pages, and the camera's build-tree label.** The
  parent area gains a **Timer** setting (Off, Count up, Count down, with a
  **Time on the board** row of whole minutes for a count-down; new store keys
  `timer=` and `timer_minutes=`, both defaulted so an old file reads Off).
  Solo Play shows the clock with the actions at the foot of the right-hand
  column, which now fills the window's full height; a count-up is shown on the
  finish summary too, and a count-down ends the round with that summary at zero.
  Back from a round in progress leaves it in progress (the session is saved on
  every find, so the same board is found on the next visit), and **My boards**
  opens the earlier-boards sheet from home. The play panel keeps Clear and
  Done: a traced word hands itself in when the finger lifts, and Done remains
  the way a word made by taps, the keyboard or a screen reader is handed in.
  The About page now has one privacy section (the summary card was the
  duplicate), shows the build's release, commit, branch and target (stamped by
  CMake into `wjs/version.hpp`, with a new `wjs::version_details()`), and links
  to a separate **Licences** page that carries the full notices in their own
  scroll. Screen titles sit on the app bar's middle line. The Settings' Board
  row says why Phonics is not offered when the build has no phase data. The
  parent's **Help** switch is a level now: Off, **A start tile** (the first
  press lights it, the next press hides it) or **Two letters** (the design's
  two rungs), applied at once and kept as `help=off|start|two` in the store (a
  file from before the levels reads `help=1` as Two letters and `help=0` as
  Off). The
  start-tile help now answers a find made from its start: the rings are put
  away whatever word was found there, and the hint card says whether more of
  the board's words begin with the tiles that were lit (a count from the
  library's new `wjs::words_starting_with`, never a word); a find away from
  the lit start leaves the help where it was. The board-finished summary is
  rebuilt for the child: the stars and points are the headline, the words
  scroll with one jewel each inside the card (which is clipped and
  responsive, so the actions never move however many words were found), Play
  again leads the actions, a count-down finish says the time ran out and that
  everything found is kept, the title speaks to one word, an empty board is
  said gently, and the card takes the focus and speaks the reward when it
  opens; its words header carries the count, and the focus stays in the
  card (Tab walks the two ways on, the board behind the scrim is silent).
  First shows are faster too: the application QML module is built with
  the type compiler (qt_add_qml_module ENABLE_TYPE_COMPILER), so the kit's
  frequent types - buttons, icons, jewels, the box, the resting screen, the
  board thumbnails - are C++ classes with compiled bindings instead of
  interpreted objects; the files qmltc cannot take in the shared aqtinstall kit
  (cross-module screens, LayoutMirroring users, Timer/Connections users, the
  translated wording singletons) are skipped per file and keep bytecode-compiled
  bindings, and the kit's private headers and qmltc runtime are linked by hand
  (cmake/FindQt6.cmake). The screen loader is asynchronous, and the 3D scene
  moved behind an asynchronous Loader that makes it when Solo Play is first
  shown - the closed box, while the child reads - and keeps it for the run, so
  the first shake pays no scene cost; Qt Quick 3D's persistent shader caches
  warm the rest across runs. The debug smoke's first frame: 14 676 ms to
  7 848 ms; a release smoke reports 582 ms, and the first windowed frame
  report (WJS_PERF_FRAME_TIMING=1, release) holds 60 Hz: mean 16.69 ms, max
  22.36 ms, no frame over 33 ms, with the p50 19.7 ms landing on Windows'
  timer quantisation - recorded in docs/design/performance.md.
  The parent area dashboard is two parts now: the one thing a parent
  came here to do (Play with a real board) stands alone at the top, and the
  child's records - the week, the sounds, the vault, the best - sit under a
  heading of their own (How the play is going) below it, with the week a card
  like the sounds' rows and the privacy line closing the page.
  scan screen's camera now works on this machine: the build tree's Low
  mandatory-integrity label (a low-trust session stamps its files Low) is what
  stopped Qt 6.12 from opening the StreamCam, and raising it to Medium
  (`icacls <build> /setintegritylevel "(OI)(CI)M" /T`, documented in
  docs/building.md) makes Qt enumerate the camera; the application's behaviour
  is unchanged - a camera it cannot see is said so, and the typed board remains
  the way on. The board's input band (the forgiving slack around the dice) no
  longer reaches over the app bar or the panel now that the board area is
  taller. The parent-area checks now hold the single-hold gate to its own
  wording, and the gate title is translated in the seven non-English
  catalogues.
- **The post-overhaul design pass: the screens now hold at the minimum window,
  the board is traced again, and the game has a visible progression.** Home and
  Solo are laid out from the room they are given rather than fixed heights, so
  the privacy line, both home cards, the box panel and the play action bar stay
  on screen at 900 by 620. The jewel box fits its own dice and its picture
  follows the chosen board size. The path is a gold ribbon over the dice and
  under the letters again; the 1-2-3 badges are gone. A found word earns points
  by length and letter value, the tray shows the running points and stars, a
  board can be finished deliberately (the standard board holds too many words to
  clear in play), and the clear/finish summary carries the words, points and
  stars. A new **My jewels** screen from home shows the lifetime stars, points,
  words, longest word, this week, the word shelf and the earlier boards; the
  play panel no longer hides the history. The help has two rungs (the start tile,
  then its first two letters) and a third press hides it. The parent gate is one
  two-second hold instead of two circles; a failed save names the problem and
  offers Retry, and the store now uses Qt's per-user application data directory.
  The scan screen has correctly turned corner brackets, a typed-board **Start
  over** and a camera **Scan again**, a camera-only **Turn**, and a **Try camera
  again** action. The 3D dice use the flat dice's cream, edge, ink, Andika face
  and warm light; the settle is orthographic and 1:1 with the flat board. The
  project dice are drawn once more from a Scrabble-style English letter table
  rather than the dictionary's own letter counts, which was over-weighting S and
  rare letters. The app and unit tests cover the new points, hints and box
  picture, and the Qt Quick suites cover the new panel, gate and layout.
- **The Qt Quick application is a first-class build target on every platform.**
  `WJS_BUILD_APP` now defaults to `AUTO`: a preset builds `WordJewelsSolver`
  when it finds a Qt 6.12 kit and builds only the command-line program and the
  tests when it does not, so nobody has to know the option to get the
  application. `ON` makes a missing kit a configure error and `OFF` skips the
  window deliberately. `cmake/FindQt6.cmake` also searches the labs' shared
  `C:/Repos/common/qt` tree on Windows, and the presets pass `AUTO` so an old
  build tree that cached the previous `OFF` default picks the new behaviour up
  on its next configure. The executable targets are named after the programs
  they make (`WordJewelsSolver`, `wjs-cli`, `ShakePrototype`), IDEs group the
  targets into Application, Libraries, Tests and Tools folders, and the
  corpus renderer `wjs_scan_render` left the default build: it is built by
  target when wanted.
- **Rebuilt as a portable C++23 project.** The Visual Studio solution is
  replaced by CMake (4.0 or newer), Ninja and the presets above, and the
  program is split into a library, front ends and a platform layer, with tests.

### Removed

- **The Win32 and DirectShow program**: the Visual Studio solution and its
  sources (the engine, the DirectShow capture and renderer code, and the Win32
  window), and the empty per-language word list placeholders. The code remains
  in the git history before the rebuild. The rebuilt program has no camera
  input yet.

> **Pre-release.** The first continuous-integration run built and tested the
> library, the command-line solver and the platform layer on Windows (MSVC
> 19.51), Linux (GCC 15.2) and macOS (Apple clang 21), all green with warnings
> as errors and sanitizers on; the Qt window is built only on Windows so far.
> There is no camera input, no reading of letters from a picture, and no
> installer or package. It has not been tried with children. Opening a file by a
> path longer than `MAX_PATH` fails on Windows, since the executables are not
> long-path aware. `wjs-cli` itself is checked by hand, not by a test.
