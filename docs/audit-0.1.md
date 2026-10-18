# 0.1 audit: the codebase against the requirements

Written 2026-10-18 from a five-agent audit of `src/` and `tests/` against
[the product requirements](product-requirements.md) and the
[design](design/index.md), plus a hands-on attempt to produce the Windows
build. It orders the whole codebase against the requirements: what exists,
what is missing, what was found broken, and the order of work. Priorities:
**P0** blocks a playable or Windows build, **P1** is core functional
behaviour, **P2** is polish or after.

How it is marked: **Fact** is something checked in the repository or on this
machine today; *Plan* is what this document proposes to do.

## 1. The state at a glance

| Layer | State today |
| --- | --- |
| `wjs::lib` — board, trie dictionary, solver, scoring, dice sets, deal, path, shake detection, phase data, seeding, grapheme binder, game rules, game session, store, parent gate, dashboard | Built and unit-tested, including the **standard board** (dice, full dictionary, 3x3–6x6, no phase data needed) |
| `wjs::scan` — board finder, glyph reader, `BoardDraft` confirm-and-correct | Built and held to a numeric accuracy target on **synthetic frames only** |
| `wjs::platform` — paths, console, `camera` and `board_reader` interfaces | Built; every desktop answers the camera `unavailable` (no backend) |
| `wjs-cli` | Built and tested end to end |
| The Qt Quick window (`WJS_BUILD_APP`) — home, `solo.box`, `solo.playing`, `solo.resting`, the panel, history, the parent area (gate, dashboard, settings, about) | Built on Windows only, never in CI, **off by default**, and it plays the **fake game** unless started with `--phase-directory`, of which the repository has none |
| The 3D shake (`wjs_dice_world` sim + Qt Quick 3D scene) | The app's Solo Play scene since 2026-10-19: one `WordJewelsSolver.Scene` module shared with the `ShakePrototype` harness, real faces from `roll_with_placements`, created once behind home, drag-to-shake until Qt Sensors lands; the 2D scene is kept unbuilt for D2 |
| Profiling (Tracy) | Preset, variant headers and zones exist for the library and the scanner; the vcpkg pin is now **0.14.1**; the app, scene and phase loader are uninstrumented |
| Tests | 28+ ctests in CI on Windows/Linux/macOS; app tests only on Windows; no test builds the app in CI |
| Camera (Mode A), audio, haptics, device shake, fonts, packaging, Android/iOS/web | Not built |

## 2. Why the Windows build was failing (found and fixed today)

The build story in [Building](building.md) is right, but this machine had
four concrete blockers, all now understood:

1. **The shared vcpkg clone was in detached HEAD at a commit that does not
   contain the pinned fork commit.** `C:\Repos\vcpkg` was at `02a476b3a4`
   while `cmake/vcpkg-commit.txt` pins `27504013…`; the pin check in
   `CMakeLists.txt:27-47` stops the configure. **Fixed:** `git -C
   C:\Repos\vcpkg checkout master` (master contains the pin). The
   troubleshooting text in `docs/building.md` only suggests `git pull`,
   which would not have helped here; it will be extended.
2. **System git 2.45.2 is below vcpkg's floor (2.55).** The fork's vcpkg
   then tries to download PortableGit and fails on this machine's network.
   **Worked around** by putting the VS 2026 bundled git
   (`…\Common7\IDE\CommonExtensions\Microsoft\TeamFoundation\Team
   Explorer\Git\cmd`, version 2.55.0.windows.5) on `PATH`. The docs will say
   so.
3. **Qt 6.12 is not found automatically.** `cmake/FindQt6.cmake` searches
   `C:/Qt`, `~/Qt` and `QT_ROOT_DIR`; this machine's Qt is at
   `C:\Repos\common\qt\6.12.0` (the shared tree the PRD names), reached only
   through the labs shell `C:\Repos\common\env.ps1` or an explicit
   `-DCMAKE_PREFIX_PATH`. *Plan:* add the common tree's Qt as a Windows
   search root (guarded by existence, after `QT_ROOT_DIR`), so the app
   builds out of the box on the author's machine.
4. **The app is off by default and had never been configured in the
   existing build trees** (`WJS_BUILD_APP=OFF` in both caches). A configure
   with `-DWJS_BUILD_APP=ON -DWJS_BUILD_SHAKE_PROTOTYPE=ON` and the Qt kit at
   `C:\Repos\common\qt\6.12.0\msvc2022_64` (which has Quick3D) is being
   verified as this document is written; the first vcpkg install succeeded
   (catch2 restored from the machine's cache).

**Fact:** the shared Qt tree has `QtQuick3D` but **no** `QtMultimedia`,
`QtSensors` or `QtTextToSpeech`, as the design index notes. Sound, haptics
and the accelerometer therefore need Qt modules installed into the shared
tree before they can be wired (section 8).

## 3. The house standard (CountdownSolver and the iclforge Qt apps)

The two sibling repositories set the bar for a class Qt application.
CountdownSolver ships signed Android APKs and an ad-hoc iOS IPA; the iclforge
Qt apps (`apps/forge/gui` and the `hearth`/`crucible` UI) are the house
pattern. Both were audited against this repository on 2026-10-18. What they do
that this repository must match:

| Standard | CountdownSolver / iclforge evidence | Here |
| --- | --- | --- |
| The Qt app is built and tested in CI on every push | CountdownSolver `ci.yml:118-170`; iclforge `pr-gate.yml:387-414` | **Done 2026-10-19:** the Windows leg installs Qt 6.12 (aqtinstall), builds the app and prototype, runs the app tests and lints the QML (`ci.yml`), and runs the app's offscreen smoke |
| Runtime QML tests (Qt Quick Test), headless | CountdownSolver 7 per-page suites (`tests/CMakeLists.txt:183-299`); iclforge ~40 `tst_*.qml` | **Done 2026-10-19:** eight `quick.*` suites (kit, theme, Home, box, panel, RTL, Mode A, parent) with a recording scene stub |
| A headless smoke mode in the app binary | iclforge `apps/forge/gui/src/main.cpp:30-72` (`--smoke`) | **Done 2026-10-19:** `--smoke [frames]` reports p50/p95/p99 and runs in CI |
| Packaging: CPack installers and licence notices | CountdownSolver `Packaging.cmake:34-102`, `release.yml`; iclforge `cmake/Packaging.cmake`, `notices/` | **Done 2026-10-19:** install rules, CPack presets, assembled and embedded notices, CI ZIP with SHA256; the NSIS/DEB/macOS generators are configured but unbuilt here |
| A coverage gate (CountdownSolver fails the PR under 80/80) | CountdownSolver `ci.yml:368-376` | report only (`ci.yml:347-350`) |
| Localization with a CI drift gate | CountdownSolver 6 languages (`src/app/CMakeLists.txt:311-326`, `ci.yml:159-191`); iclforge `qt_add_translations` + lupdate gate | **Done 2026-10-19:** eight languages, RTL/LTR, lupdate drift in CI |
| A light/dark/**system** preference, not just system-follow | CountdownSolver `Theme.qml:8`; iclforge palette preference | system-only today (`Theme.qml:27`) |
| The toolchain pinned in one file | iclforge `.github/toolchain-versions.json` | **Done 2026-10-19:** the file exists and `toolchain.versions` holds the workflows and the vcpkg baseline to it |
| Security breadth (OSV-Scanner, zizmor, fuzzing) | CountdownSolver CI (OSV, zizmor, ClusterFuzzLite) | CodeQL, Scorecard, dependency-review only (still open) |

Where this repository is **ahead**: Tracy 0.14.1 pinned, with tested zone
conventions and per-platform presets (CountdownSolver has no Tracy at all);
the platform-isolation check; the commit-text policy; the QML scene contract;
the performance tier as a CI gate; the design-document suite; and the
Quick3D licence rule.

The two audits agree on the order: **the app in CI first** (it unblocks the
QML suites and the smoke gate), then **Quick Test suites**, then **packaging
with notices**, then the coverage gate, localization and the colour-scheme
preference. The last is also the author's own complaint of 2026-10-18
("light mode, dark mode is missing"): the house standard is a preference with
System, Light and Dark, which this repository does not have.

## 4. The requirements, one row each

| Requirement (PRD / design) | Where it lives | State | Gap | Priority |
| --- | --- | --- | --- | --- |
| Solver library, CLI (phase 1) | `src/lib`, `src/cli` | Done, tested | none | — |
| Standard board: dice, full dictionary, 3x3–6x6 (12.7) | `dice_sets`, `deal`, `GameSession` | Library done and tested | the app cannot play it (`solo_game_setup.cpp:90-93`, `real_solo_game.cpp:95-99` force phonics; `main.cpp` demands `--phase-directory`); sizes are clamped to 3x3/4x4 in the app and settings | **P0** |
| Phonics board: seeded 3x3/4x4, phase index (12.7) | `phase_data`, `seeding`, `GameSession` | Mechanism done, tested on test data | no phase file exists (a reviewed one is gated on the literacy reviewer, G7); release review-gate bypasses exist on multi-config generators | P1 |
| Shake: virtual 3D container, physics feel (3, 12.4; board.md decided 3D behind a seam) | `src/app/scene`, `src/app/qml/scene` | The app's scene; faces come from the roll | a standard board restored from the store has no roll seed (letters on the up face); Qt Sensors (accelerometer) is not in the tree, so a drag stands in | P1 (sensor) |
| Shake by accelerometer + haptics + rattle audio (3) | `detect_shakes`, `MotionSource`, `haptics` | **Done 2026-10-19:** Qt Sensors feeds the scene and the detector, a haptics interface has a desktop no-op, the rattle plays; hardware untested | the Android/iOS sensor and haptic backends wait for those targets | P1 (devices) |
| Trace with a finger, tiles illuminate, tap-in-order (3.5, game.md) | `PathTracer`, `TraceLayer`, `PlayingPanel` | Built and tested (20+20 cases) | none found | — |
| Words: viable / real-not-today's / fade (game.md, 12.1) | `classify_path`, session `trace` | Done, surfaced to QML | the soft tick and fade have no sound behind them (audio missing) | P1 |
| Scoring: stars by tier, first-time sticker, best silently (7.2) | `game_rules`, store | Done end to end | the fake game never awards a sticker; a standard-board real game will fix this | P0 (with the game) |
| Start-tile help, one level (12.4) | `start_tile_help`, session, `PlayingPanel` | Done and tested | none | — |
| History (last 20 boards) | store, `HistorySheet` | Done | fake game's history is in-memory; real game fixes it | P0 (with the game) |
| Resting screen, no pull to continue (12.3) | `SoloResting`, `HoldButton` | Done | none | — |
| Parent gate, dashboard, settings, about (7.4, 12.2) | `wjs::Store`, `Gate`, `build_dashboard`, `WordJewelsSolver.Parent` | Done, tested (29 cases + offscreen walk) | **light-only**: `ParentTheme.qml:11-12` has no dark values and nothing binds the system scheme | P1 |
| Preferences complete and applied | `Settings`, `SettingsScreen.qml` | All design settings stored and shown | `sound`, `vibration`, `celebration_sounds`, `sound_it_out`, `shake` are inert (no audio/sensor engine); `board` is ignored (standard game missing); school font never offered (list empty, correct per licence); text size lacks the system factor and clamp | P0/P1 |
| Light/dark mode (7.1, application.md "Colour and contrast") | `Theme.qml`, `SystemColourScheme` | Solo Play chrome follows the system scheme, live | the parent area is hardcoded light; `SystemColourScheme` has no test | P1 |
| Mode A: camera scan, check screen, hints (4, 12.4) | `wjs::scan`, `BoardDraft`, `camera`/`board_reader` interfaces | Library done on synthetic frames | no camera backend on any desktop; no `scan.*`/check screens in the app; the home tile is gated off | P1 |
| Sound: phonemes, words, celebration (7.3, application.md table) | `Feedback.qml`, `src/app/feedback` | **Done 2026-10-19:** Qt Multimedia plays the cues from placeholder assets, TextToSpeech reads whole words, the settings gate it; the phoneme clips await the reviewer | no audio device on this machine, so nothing was heard here | P1 (review) |
| Sound-it-out sequence (7.3) | — | the speech seam is wired and silent | build the segmentation/timeline with the reviewer's clips (allowed placeholder clips, phonics.md) | P1 (review) |
| Typeface: Andika + school fonts (7.1) | `Theme.qml` names Andika | not bundled; platform fallback silently | licence read + glyph check + bundling, and the school-font list | P2 |
| Performance: Tracy throughout, budgets (8) | `wjs/profiling.hpp`, 15 zones, perf tier | Library and scanner zones built; budgets run | Tracy pinned to **0.14.1** (done 2026-10-18); app/scene/phase-loader uninstrumented; no frame marks/messages | P1 (zones) |
| Windows build that produces the app | presets, `windeployqt` | Works once section 2 is applied; no install/package step | CPack/installer is not built; `WJS_BUILD_APP` off by default and absent from CI | P1 |
| Privacy: no network, no capture, no export, small store | store, CI scans | Done and checked | none | — |
| Android/iOS/web/phone (2) | `src/platforms` | Not started (the PRD's own fact) | after the MVP | P2 |

## 5. The P0 work (a playable Windows build, the real game, Tracy 0.14.1)

1. **Finish the Windows build story** — Qt search root for the common tree,
   `docs/building.md` troubleshooting for the three machine states found
   today, an `app` build verified end to end (configure is running as this
   is written; then build, `windeployqt`, launch).
2. **Play the standard board by default.** Load `data/dice` into
   `SessionData.dice`, stop forcing phonics in `solo_game_setup.cpp` and
   `real_solo_game.cpp`, make `--phase-directory` only switch on the phonics
   board, offer 3x3–6x6 on the standard board, and teach
   `Store::save_session` a standard snapshot (the session's restore already
   takes it; the store refuses the count-only record today,
   `architecture.md:937-949`). The dice faces (`roll_with_placements`) join
   the `SoloBoard` so the scene can draw real dice.
3. **Pin Tracy 0.14.1** (done 2026-10-18). `vcpkg.json` `builtin-baseline` →
   `2750401336fb7c95f6619657a46a7e798661341c` (the commit already in
   `cmake/vcpkg-commit.txt`; its baseline maps tracy to 0.14.1#1), plus an
   explicit `overrides` entry, and the doc pages that said 0.13.1 updated.
   The profiling preset is configured as verification.
4. **Wire the 3D dice world into Solo Play behind the scene seam**, accepting
   the recorded GPL-3.0-only consequence for builds that link Quick3D (the
   seam stays; the licence decision is the author's at D2). This is the
   "dice in the Boggle container with a real simulation feel" requirement.
   *Done 2026-10-19:* the prototype's scene moved into the app
   (`WordJewelsSolver.Scene`), the app links Quick3D (GPL-3.0-only, recorded),
   the real faces come from `roll_with_placements` through the port, the seam
   gained `prepare`/`overdraw`, the scene is created once behind home, a drag
   feeds `pushMotion` until Qt Sensors lands, and `windeployqt` no longer
   deploys the Quick3D profiler plugin. The 2D scene is kept, unbuilt, for D2.
   A standard board restored from the store has no roll seed and so draws the
   letter on its up face; storing the deal seed in the record is a follow-up.
   In the same change: `windeployqt --skip-plugin-types qmltooling`, because
   today's deploy silently copies the GPL-3.0-only `Qt6Quick3DUtils.dll` and
   the Quick3D profiler plugin beside the app even when nothing links
   Quick3D ([shake-prototype.md](design/shake-prototype.md#how-it-was-measured)).

## 6. The P1 work

5. Parent area dark theme + a `SystemColourScheme` test (done 2026-10-18;
   the theme test is written and awaits registration).
6. **A Colour scheme preference: System, Light, Dark** — the house standard
   from both sibling repositories, and the author's own complaint. It needs a
   `Settings` field, the store format and its migration, a screen row, the
   `Theme`/`ParentTheme` override and tests.
   *Done 2026-10-18:* the setting, the store key, the screen row, the theme
   override for both kits, the parent area's dark tokens and the tests.
7. Apply the inert settings where their engines exist (sound/haptics once
   Qt modules land; shake switch once the sensor exists), and build the
   sound-it-out sequence timeline with placeholder clips in the library.
   *Done 2026-10-19:* the engines are wired - Qt Multimedia sound with
   placeholder assets, Qt TextToSpeech for whole words, Qt Sensors for the
   accelerometer/gyroscope, a haptics platform interface with a desktop no-op -
   each with a silent variant and pure policies. The settings now have effects;
   the sound-it-out seam waits for the reviewer's phoneme clips.
8. Mode A in the app (done 2026-10-19): the `WordJewelsSolver.Scan` module
   and the `scan.ready → scan.reading → scan.check → assist.playing` flow,
   the check screen over `BoardDraft` (doubtful rings, letter picker, turn,
   resize), the assist screen with the start-tile hint and a store record,
   the "Type in your board" desktop path, and the camera lifecycle. Left: a
   camera backend (desktops answer `unavailable`; the iPad camera and Vision
   reader are unwritten), real-frame accuracy work, and audio for the cues.
9. **Qt performance and profiling — the whole UI, 2D and 3D (a work
   package of its own).** Today the zones stop at the library and the
   scanner; nothing in the app, the QML, the 2D scene or the 3D scene is
   measured, so "is it smooth?" has no answer. This package delivers:
   - **Frame marks and a frame-time plot** on the UI thread
     (`QQuickWindow::frameSwapped` → `frame_mark("UI")` and
     `plot("ui.frame_ms", …)`), with p50/p95/p99 reported, not just an
     average.
   - **Zones around every part that can cost a frame**: `app.startup`,
     `app.load_qml`, `app.scene_create`, the scene's shake/drop/settle/
     celebrate moments, the dice world's per-frame step, the face-atlas
     draw, the 2D scene's per-frame script, the trace layer's path work,
     and each screen's transition.
   - **A QML-side instrumentation seam** so QML/JS moments (a binding
     storm, a view rebuild, a property cascade) are visible in Tracy
     rather than only as a slow frame: a QML-visible scope object over the
     same variant-header no-macro wrappers, empty when profiling is off.
   - **Capture recipes for the app**: a headless, deterministic run
     (`--smoke`-style, N frames offscreen) so `tracy-capture` has a
     workload it can attach to, documented in `profiling.md`, plus an
     interactive recipe for the shake and the screens.
   - **Budgets with tests**: a frame budget (16.7 ms at 60 Hz, no frame
     over 33 ms in the shake) and work budgets for the scene frames, so a
     regression fails a test in the performance tier, as the solver's do.
   - Both scenes are covered: the **3D** Qt Quick 3D scene (prototype and
     the app's version once wired) and the **2D** placeholder
     (`qml/scene/BoardScene.qml`, `SceneScript.js`), so the seam can be
     swapped without losing measurement.
   *Done 2026-10-19:* 18 C++ zones (`app.startup`, `app.load_qml`,
   `data.load_phase`, `scene.step`, …), a QML `Profiler` seam with 10 runtime
   scope names (transient Tracy zones) and 5 plots (`ui.frame_ms`,
   `ui.first_frame_ms`, `render.frame_ms`, `ui.sync_ms`, plus the frame set
   `UI`), thread names, a `--smoke <frames>` mode with a p50/p95/p99 report,
   exact scene work budgets and an opt-in frame check. A captured trace
   (1,517 zones, profiling build) shows `app.load_qml` 251 ms, the dictionary
   70 ms, `scene.prepare` 1.0 ms, `scene.play` 2.1 ms, and the shake/drop/
   settle frames at 0.7–1.0 ms each. Two findings: Tracy 0.14.1 removed
   `tracy::Profiler::Message`, so the enabled wrapper had not compiled (fixed);
   and this machine's session cannot present a window, so no desktop 3D frame
   report was obtainable — the mechanism is verified offscreen and the budget
   is for a desktop run. The atlas layers are now enabled after component
   completion, which is the fix for the debug-Qt
   `QQuickItemLayer::componentComplete` assertion the agent saw; it could not
   be re-verified here because this session cannot present a window, so it is
   first on the list for a desktop check.
10. Release review-gate enforcement on all generator kinds (done 2026-10-18:
    multi-config generators now default the gate ON).
11. **The app in CI**, with the qmllint/scene-contract tests, a headless
    smoke mode and Qt Quick Test suites (the house standard's first gap).
    *Partly done 2026-10-18:* the Windows CI leg now installs Qt 6.12 with
    aqtinstall and builds the application, the shake prototype and their
    tests; the headless smoke mode and Qt Quick Test suites remain.
12. **Packaging with licence notices** (CPack, the Qt/font notices, install
    rules) and a release workflow; both sibling repositories ship this and
    this repository has none.
13. The coverage gate (a floor, as CountdownSolver's 80/80) and the
    toolchain-versions file (iclforge's single source of pinning).
    *Partly done 2026-10-18:* `tests/profiling/check_presets.cmake` now
    asserts the Tracy override (0.14.1, port-version 1); a
    `.github/toolchain-versions.json` and a coverage floor remain.

## 7. Both modes, complete

Both modes are in scope and neither is a demo: the author's requirement of
2026-10-19 is that Mode A **and** Mode B are fully implemented. This is the
definition of done for each, and what is left of it.

**Mode B (Solo Play), done when** a child can do all of this with no phase
directory and no developer flags: choose 3x3 to 6x6 on the box; shake it (the
button, the device, or a fast drag); watch the dice tumble inside the
container and drop into the grid; trace a word with the tiles lighting and
sounding as the finger passes; hear the word and see the jewel, the stars and
the tier's celebration; ask for the start tile; open the history; clear the
board or put the lid back; and rest. On the phonics board, the parent's switch
seeds a 3x3 or 4x4 from the phase, the vowel team lights as one grapheme
block, and the sound-it-out sequence segments and blends with clips in step.

Left after 2026-10-19: the sound-it-out sequence's phoneme clips (the seam is
wired and silent until the reviewer's recordings), the vault/sticker screen and
confetti, the school-font pipeline, and the drag-the-box gesture is in. The 3D
scene, the dice faces, device shake (Qt Sensors), haptics and the audio cues
(rattle, letter, word, cheer, cleared) are built.

**Mode A (Family Companion), done when** a parent opens the scan, a live
camera view appears, Capture reads the board, the check screen shows the
replica with doubtful tiles ringed, a person corrects a tile and presses Yes,
play, the board loads, the child traces words and gets the start-tile hint,
and a new round starts from a new scan.

Left (after 2026-10-19): a capture backend per host (Media Foundation or Qt
Multimedia on Windows; AVFoundation + Vision on the iPad, which needs a Mac),
the real-frame accuracy work (synthetic-only today), and audio for the cues.
The screens, the flow, the check screen and the hint are built.

**Shared, and not duplicated**: one board component, one panel, one store, one
start-tile hint, one set of screens' tokens; the mode is chosen before the
scan or the shake. The privacy rule stands in both (no frame leaves the
device; the camera is on only while scanning).

**Blockers to name**: Qt Multimedia, Sensors and TextToSpeech are absent from
the shared Qt tree (install them, or implement audio/haptics/sensors per
platform); the iPad build needs a Mac; the recorded phonemes and reviewed
phase data need the literacy reviewer. Development-only phase data and
placeholder clips outside `data/phonics` are allowed and are how the phonics
board gets exercised before the review.

## 8. Languages, RTL and i18n

Stated by the author on 2026-10-19: **English, French, Spanish, German,
Yiddish, Hebrew, Chinese and Japanese**, with **RTL and LTR fully supported
and implemented**. Mechanical translation is acceptable for now, with a full
linguistic review as a later task.

*Done 2026-10-19:* the Language setting (store key `language=`, system default),
a `LanguageManager` applying the translator and direction before the engine and
live via `retranslate`, eight complete catalogues (138 messages each, generated
by lupdate and released at build), RTL mirroring for Hebrew and Yiddish checked
in both directions, per-glyph font fallback with a recorded Windows glyph check,
and the catalogue-completeness, direction, glyph and lupdate-drift checks. The
translations are mechanical and join the reviewer's list.

**Mechanism (Qt Linguist, as the sibling projects do it).** `qt_add_translations`
with one `.ts` catalogue per language (the kit has `Qt6LinguistTools`); `lrelease`
at build; a `QTranslator` installed on the application **before the engine
loads**; a `Language` setting in the store (default: the system locale when it
is one of the eight, else English) with a picker in the parent area and a
first-run choice; a `LanguageManager`-style class that applies the language,
the layout direction and the font in one place.

**RTL.** Hebrew and Yiddish are right-to-left. The window sets
`layoutDirection: Qt.RightToLeft` and `LayoutMirroring.enabled` with
`childrenInherit` for those languages; every screen is checked for mirroring
(navigation back chevrons, the panel's leading edge, the gate's two spots, the
parent area's rows). The board, the 2D and 3D scenes and the word content do
**not** mirror: a Boggle grid and its English tiles read the same way in every
language. Numbers and dates use the locale.

**Fonts.** Bundled and notices-recorded (OFL): Noto Sans for the Latin
languages, Noto Sans Hebrew for Hebrew and Yiddish. Chinese and Japanese use
system fallback (Microsoft YaHei / Yu Gothic on Windows, PingFang / Hiragino on
the iPad) unless the author wants Noto Sans CJK bundled (about 16 MB); either
way a build check proves every glyph the catalogues use resolves, and the
licensing page records what is bundled.

**Strings.** Every parent-facing sentence is already in
`src/app/qml/parent/ParentWording.qml`; each becomes a `qsTr()` with a
translator comment, so the catalogues are generated from one file. Child-facing
labels (`Shake`, `Done`, the panel) are translated in the same catalogues.
Library error text stays English (developer-facing); the screens translate what
they show of it. Nothing is drawn into a picture.

**What is translated, and what is not.** The UI chrome is translated; the
**word list (ENABLE), the dice letters and the phonics content stay English**,
because the game is an English word game. A translated UI over an English board
is the scope; translating the game content is a separate decision for the
reviewer and the word-list work.

**Tests and CI (the house standard).** A catalogue-completeness test (every
`Message` translated, no `unfinished`, `vanished` or `obsolete` entries; the
message count is the source's), a direction-mapping test (he/yi → RTL, the rest
LTR), a per-language offscreen QML smoke run (the parent screens and the Solo
screens load and lay out in each), a glyph-coverage check for the sample strings
of each language, and an `lupdate`-drift gate in CI (regenerating the
catalogues leaves no diff). The translated wording is **not reviewed** and is
listed with the phonics content as needing the reviewer (the screenshots and
the phrases a professional must check).

## 9. The P2 work

11. 5x5/6x6 reachable, text-size system factor, school-font pipeline, the
    vault screen, drag-the-box shake gesture, recorded device shake traces,
    CPack installers, a Tracy version assertion in the presets check (added
    2026-10-18).

## 10. The order of work (waves)

Both modes are lanes of equal weight; the last column is the house standard
and the platform work that both need.

| Wave | Mode B (Solo Play) | Mode A (Family Companion) | Cross-cutting |
| --- | --- | --- | --- |
| 1 | the standard board's real game (dice, full dictionary, 3x3–6x6, no phase; in flight) | — | Windows build story, Tracy 0.14.1, the app in CI |
| 2 | the 3D dice scene behind the seam; the real faces through the port | — | — |
| 3 | — | scan/check/assist screens, typed-board fallback, start-tile hint, camera lifecycle (no device camera yet) | the colour-scheme preference, the dark parent area |
| 4 | device shake, haptics and audio (after the Qt modules); the sound-it-out sequence; the vault, stickers and confetti | camera backends (Windows, iPad), real-frame accuracy work | the **Qt performance and profiling package** (section 6 item 9) |
| 5 | school fonts, the drag-the-box gesture, the text-size system factor | — | **languages, RTL and i18n** (section 8) — its own agent's job; then packaging and notices, Quick Test suites, the smoke mode, the coverage gate |

Each wave lands as code with tests and docs on `develop`, following the
repository's rules (no `#if`/`#define`, warnings as errors, the text policy,
the privacy scans).

## 11. Decisions the author must make

1. **Qt modules.** Install Qt Multimedia, Sensors and TextToSpeech into the
   shared tree (`C:\Repos\common\qt\6.12.0`), or leave sound, haptics and
   the accelerometer stubbed behind the existing seams for now.
   *Resolved 2026-10-19:* the three modules were installed into the shared
   kit (the Qt repository's per-architecture layout, archives verified by
   SHA-1); the app now links them where present and keeps silent variants
   where a kit lacks them. The CI Qt install moves to
   `install-qt-action` with the pinned aqt master that reads Qt 6.11+
   repositories.
2. **The 3D scene in the app** makes the shipped binary GPL-3.0-only (Quick3D
   licence). The design recorded this and deferred it to D2; wiring it now
   follows the design's own "3D behind a seam" decision.
3. **Camera on Windows desktop**: the design's committed fallback (answer 15)
   is "Type in your board" on desktops; a real backend (Qt Multimedia — not
   in the shared tree — or Media Foundation/OpenCV) is a decision for the
   author. Mode A's iPad camera is the MVP's stated platform and needs a Mac.
4. **"Boxes"** — the box screens (`solo.box`) exist; if the brief meant
   anything else (a preferences page outside the parent area, an option
   control style), say so and it is added to the plan.
