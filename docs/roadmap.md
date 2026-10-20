# Roadmap

Written 2026-10-15, at the end of the restructure, from the
[product requirements](product-requirements.md). It says what exists, what each
requirement needs, the decisions that gate the plan, and the order of work.
Claims that were not verified are marked **unverified**.

## 1. What is being built

WordJewelsSolver is a Boggle game and companion for 5- and 6-year-olds, on
tablets first, then phones, the web (WebAssembly) and desktop (macOS, Linux and
Windows). Two modes:

- **Solo Play**: shake a virtual box, dice drop into a 3x3 or 4x4 grid, the
  child traces words with a finger and hears them.
- **Family Companion**: scan a real board with the camera; the app finds the
  words and gives the child hints instead of the list.

The PRD's own phases put Solo Play first and the camera last ("Future/P2").
This roadmap follows that: **Solo Play needs no camera, and the camera is the
riskiest part, so it comes after there is a game worth scanning for.**

## 2. Where it stands

| Area | State | How it was checked |
| --- | --- | --- |
| Solver library (board, trie dictionary, scoring, depth-first search) | Done | Catch2 cases; the 3x3 reference board checked by hand |
| `wjs-cli` | Done | Run end to end on Windows |
| Platform isolation (`src/platforms`, one CMake selector, enforcing check) | Done | `platform.isolation` and `platform.fragments` ctests |
| Qt Quick shell (type a board, Solve, scored list) | Works, **not seen rendered** | Built with Qt 6.12.0; launched offscreen, QML loads; 3 model tests |
| Windows build (MSVC 19.51, Qt 6.12.0, `/W4 /WX`, ASan) | Verified | Locally and in CI; 31 ctests locally (with the window), 28 in CI |
| Linux (GCC 15.2) and macOS (Apple clang 21) builds | Verified | CI, Debug, warnings as errors, sanitizers: 28/28 tests on each. The Qt window is not built on either |
| Android and iOS builds of this app | Not started | CountdownSolver has both working: a signed Android APK, and an ad-hoc signed iOS IPA that has been installed and run on your iPhone and iPad (its CHANGELOG records it; `docs/building.md` there is out of date on this) |
| CI, repository rules, documentation | Running; rulesets applied and enforced | First CI run green (12 checks); branch-name rule verified by a rejected push. The `main` and `develop` protections are applied but not exercised |
| Touch UI, dice, shake, tracing, audio, rewards | Not started | |
| Path validation and shake detection (the library parts of M1) | Done, **on synthetic data**: `validate_path`, `detect_shakes` | Catch2 cases. The detector's thresholds are starting values that have to be tuned on a device |
| Dice for the standard board: a set model, a roll, the project's own dice for 3x3 to 6x6 | Done | Catch2 cases, `data.dice`, boards of 300 rolls a size solved against the dictionary |
| Phonics machinery: phase-file reader and checker, review gate, seeding, grapheme binder | Done, **on test data**. No phase file exists, and none may be added without a reviewer (G7) | Catch2 cases, `data.phonics` and `data.phonics.checker`, the gate exercised on a release build. See [Testing](testing.md#phonics-data-the-checker-and-the-review-gate) |
| Child-appropriate lexicon and hints | Not started | |
| Camera scan | Not started | |
| An English word list | ENABLE in `data/dictionaries/`; category and tier lists in `data/lexicon/` | `tools/data/check_wordlists.ps1`. The authored lists are **unreviewed** by a teacher or parent |

## 3. The PRD, requirement by requirement

| PRD requirement | Today | What it needs | Milestone |
| --- | --- | --- | --- |
| Migrate the solver to the tablet | Library is portable C++23, Qt-free | Android and iOS builds of the existing app | M1 |
| Filter by syllables, difficulty, themes | Nothing | A word list with metadata: length, frequency band, syllables, phonics pattern, categories (animal, colour, shape) | M2 |
| 3x3 or 4x4 board | `Board` accepts 1x1 to 16x16 | A dice model (real Boggle dice sets, random rotation, Qu face) in the library | M1 |
| Shake, with the accelerometer | Detection over samples is written and tested on synthetic traces (`detect_shakes`); nothing reads a sensor | Sensor access through Qt Sensors or a platform interface; the thresholds tuned on a device; a desktop fallback (key or drag) | M1 |
| Haptic feedback | Nothing | A `haptics` interface with Android and iOS implementations under `src/platforms`, nothing on desktop | M1 |
| Drop animation, glowing path, jewels, confetti | Nothing | 2D Qt Quick: Canvas/Shapes/Particles. Not Qt Quick 3D (see D2) | M1 |
| Trace a path with loose tolerance | `validate_path` is written and tested; no touch handler | A touch handler that snaps to the nearest tile within a radius | M1 |
| Letters speak their *phonetic sounds* | Nothing | Recorded phoneme audio. Speech synthesis says letter *names* ("see"), not sounds ("k"), so this is a recording job (D5) | M1 |
| Read the found word aloud, name its category | Nothing | Speech synthesis or recordings, plus category tags from M2 | M1, M2 |
| Legible early-reader typeface | Nothing | Andika or OpenDyslexic, both open fonts (licences to verify). Comic Sans is proprietary and cannot be bundled | M1 |
| Hide advanced words, give hints not lists | Solver returns everything | A hint engine over solver output and lexicon metadata: "a 3-letter animal starting with C" | M2 |
| Camera scan of a physical board | Nothing | Board finder, tile reader, confirm-and-correct screen; Vision on iPad, OpenCV elsewhere | M4 |

## 4. Decisions that gate the plan

| | Decision | Recommendation | Why |
| --- | --- | --- | --- |
| D1 | First platforms and how they are distributed | **iPad and Android tablet, both as private builds for your own devices, using the pipelines CountdownSolver already has.** Iterate on desktop and an Android emulator; take iPad builds from CI. | CountdownSolver ships a signed Android APK and an ad-hoc signed IPA (macOS runner, Apple Developer Program certificate and provisioning profile held as repository secrets, an `itms-services` install page) and both have run on your devices. An ad-hoc build installs only on devices registered in its profile, which is what a family tablet needs. A build that only runs on your own devices is not distributed, so no licence or store rule bites yet. Costs to expect: a new App ID and provisioning profile for this app's bundle identifier, which only you can create in the Apple portal, and a CI round trip for every iPad build if you have no Mac. |
| D2 | Licence, when something leaves the house | Decide **before the first public store submission**, not now. Keep `GPL-3.0-or-later` for the source. Whether the board is drawn with Qt Quick 3D (the Cubeulator look) is a design question (PRD section 11, question 13) with a licence consequence. | This is sole-authored code, so dual-licensing or a store exception is possible. The obstacle is Qt on Apple's stores: Cubeulator's `docs/licensing.md` analyses it and its maintainer chose to skip the Apple stores. Qt Quick 3D is offered only as GPL-3.0-only or commercial, so a build that uses it can be conveyed only under GPL-3.0; a build that does not use it keeps "or later". Private builds and GPL downloads are not affected either way. Not legal advice. |
| D3 | UI toolkit on tablets | Qt Quick everywhere, as now. | One UI, the shell exists, and Cubeulator already ships the mobile build plumbing. The library is Qt-free, so a native iPad UI over it stays possible if D2 forces it. |
| D4 | Source of word metadata | Candidates: a pronouncing dictionary (syllables, phonemes), a lexical database with hypernyms (categories), published sight-word lists. | **Licence of each must be checked before use**; none is assumed. Metadata is data, kept out of the code. |
| D5 | Audio | Record phoneme sounds and the words yourself or with a family voice; use speech synthesis only for words and sentences. | Recordings you make carry no third-party licence. A child hears a phoneme, not a letter name. |
| D6 | Name | **Decided 2026-10-16: WordJewelsSolver**, as the repository, CMake project and CLI already have it. The brief's working name, WordJuels, is retired. | Settled before any public release. |
| D7 | Engine word list | **Done:** ENABLE, unmodified. The child-facing layer is an allow-list (categories and a core tier), not a filter over it, so nothing unsuitable can appear by omission. | See G2. |

## 5. Where the new code goes

The rule stays: no `#if`, no `#define`, no OS code outside `src/platforms`.

- **Library (`src/lib`, Qt-free, platform-free, unit-tested):** dice sets and
  shaking; shake detection over accelerometer samples; `validate_path`; the
  lexicon and hint engine; scoring modes suited to children.
- **Platform interfaces (`src/platforms/interfaces`, one implementation per
  platform):** `haptics`, `board_reader` (Vision on iOS, OpenCV or ML Kit
  elsewhere), `camera` (Cubeulator's `interfaces/camera.hpp` is the model).
  `android` and `ios` fragments are added to the selector, two edits each.
- **App (`src/app`, QML):** screens and animation; no game rules.

## 6. Milestones

**The MVP.** The author's categorisation of the first release is PRD section 12,
as amended by section 12.7. It is narrower than M1 to M2b below in what it ships
(a standard dice board, 3x3 to 6x6, against the full dictionary, with a
hand-filtered SATPIN phonics board as a parent's switch; one help level, no
voice recording, no export, no network capability, no timers, no block list) and
wider in one respect: it includes the camera-to-board scanner (Mode A), which
this roadmap had after Solo Play. The MVP is iPad first. Where a milestone below disagrees with section 12,
section 12 wins; the milestones are the full product's order.

### M1: Solo Play on iPad and Android tablet (private builds)

The whole of Solo Play, nothing else. Exit: the app installs on your iPad and
your Android tablet and a child can shake, see the dice drop, trace a word, hear
it, and collect a jewel.

- Android arm64 build and signed APK, following CountdownSolver (Qt Android kit
  via `QT_ANDROID_ROOT`/`QT_HOST_PATH`, `android-arm64-v8a-*` presets).
- iOS ad-hoc IPA from a macOS CI runner, following CountdownSolver's
  `ios-release` job and install page, with this app's own bundle identifier,
  certificate and profile. Qt's iOS kit for 6.12.0 has to be installed in CI
  (its availability for 6.12 is **unverified**; the labs notes say to check each
  host and module with `aqt list-qt` and a real install before bumping).
- Dice model, shake detection, `validate_path`, with unit tests. All three are
  done: the dice model reads a set of dice and rolls a board (`wjs::roll`), and
  the project's own dice for 3x3 to 6x6 are in `data/dice`, made from a fixed
  Scrabble-style English tile-frequency table.
- Touch tracing with generous tolerance, tested on a real tablet with small
  fingers (the one thing no unit test can settle).
- Phoneme and word audio; haptics; the typeface; confetti and the jewel bank.
  The state school-font setting (PRD 7.1) needs each font's redistribution terms
  checked first, and is **not** part of the exit above until they are.
- Boards from 3x3 to 6x6: the MVP's standard board (PRD 12.7).

### M2: Child-appropriate words and hints

- Lexicon with metadata (D4) and the age-tier filter.
- Hint engine; "nudge, don't spoil" works in Solo Play too: a hint for a word
  not yet found.
- Scoring that rewards a child's first 3-letter word.

### M2b: Phonics mode and the parent dashboard

From the PRD sections 7.3 and 7.4, as narrowed by the MVP scope (section 12).
**Not designed**: the PRD lists the open questions (29 to 69). What is known now
is what each part depends on.

- **Grapheme and phoneme data** for every word a child can meet, per phase, and
  the audio of the sounds in an Australian accent. Neither exists in the
  repository, and the choice of source (authored for a decodable list, or derived
  from a pronunciation lexicon) is open. The dictionary has no pronunciations.
- **Phase word sets.** Deriving them by filtering the dictionary yields entries a
  child does not meet (249 words for the six SATPIN letters, five of them in the
  early-reader tier), so each phase needs an authored, reviewed list.
- **Grapheme-block tracing and the sound-it-out sequence**: a presentation layer
  over the solver's path; the library already takes tiles of one to three
  letters.
- **A local store** for progress and found words, with no network capability,
  behind a parental gate; and the **sound-mastery metrics**, which need the
  grapheme data above. The MVP keeps no voice clips and no time played.
- **No export.** The printable PDF and e-mail are outside the MVP (PRD 12.2). If
  they return, the way a PDF leaves the device on each platform (the system
  share and print facilities, under `src/platforms`) is the first route by which
  a child's data would leave it, and needs the privacy review first.
- **Phase names are generic**: Phonics Phase A and Phonics Phase B (PRD 12.1).
  Where each phonic sound sits is for the literacy review (G7).
- **Built on test data, with the content still blocked on G7**: the reader and
  checker for the phase files, the review header and the release build's
  refusal of a file without it, the seeding of a board, and the binding of a
  trace into grapheme blocks. No phase file exists. The sound-it-out sequence,
  the audio and the dashboard are not built.

### M3: 0.1, the first release

Everything a stranger needs to build and use it, plus the gates below.
Artefacts: source, desktop builds, a signed Android APK on GitHub Releases
(allowed under GPL; a store is a separate decision, D2).

### M4: Family Companion

1. **Capture**: `camera` interface, OpenCV capture per OS (Cubeulator's design),
   camera permission through Qt.
2. **Find the board**: largest convex quadrilateral, perspective warp, split
   into cells. Dice glare on glossy plastic is the hard part.
3. **Read the tiles**: rotated single glyph per cell; template matching or a
   small classifier trained on renders of the dice font. Full-page OCR (the 2013
   Tesseract spike) is the wrong tool. Report confidence.
4. **Confirm and correct** on the editable grid: the solver sees only the
   confirmed board. Imperfect recognition is acceptable because of this step.
5. **Test corpus**: recorded frames kept outside the repository (they show a
   child and a room), plus synthetic renders; a numeric tile-accuracy target
   set before starting.

**Built so far (first slice), on synthetic frames only** ([Scanner design](design/scanner.md)):
the library (steps 2 to 4 above, in `src/scan`, with no third-party dependency), the `camera`
and `board_reader` interfaces (the platform camera has no native desktop backend; the portable
reader), the confirm-and-correct model (`BoardDraft`), a numeric target written first (item 5) and a
synthetic corpus that holds the reader to it, work budgets in the performance tier, and the
privacy scan. **The desktop camera is built too** (2026-10-19): Mode A's ready screen shows a live
picture and Capture reads one frame through the application's `CameraSource` over Qt Multimedia,
with a `camera-none` variant for a kit without the module. **Not done:** anything on a real
camera or real dice — no camera is attached to the machine it was built on; the iOS camera and Vision
reader (they need a Mac and an iPad); recorded frames and a measurement on them; the dice's own
typeface; stand-and-watch scanning. The target is met on the synthetic
corpus and says nothing about real dice.

### M5: store distribution

TestFlight, the App Store and Google Play. Not needed for anything above, and
gated by D2 (licence) and G6 (children's privacy and each store's family
policy). Until then iPad builds stay ad-hoc, on registered devices.

The iOS-specific work of M4 is the Vision-based `board_reader`, which lives
under `src/platforms/ios` as Objective-C++ and is exercised on the iPad.

### M6: reach

The same application on the other targets named in the brief.

- **Phone**: a layout of the same application, in portrait and landscape.
  The two-thirds / one-third arrangement of the PRD has to be answered for it
  (PRD section 11, question 12).
- **Web (WebAssembly)**: a platform fragment for Emscripten. The Qt 6.12.0
  `wasm_multithread` kit and Emscripten 5.0.5 are already in the shared tree
  (`C:\Repos\common`) for Cubeulator; Cubeulator's `web-emscripten-*` presets
  are the pattern. A browser has no camera device the way a tablet does, so
  Solo Play is the natural whole of it there (question 15). Threads need
  cross-origin isolation headers from the host.
- **Desktop application** for macOS, Linux and Windows: the platform fragments
  exist and the command-line solver is built and tested there. Packaging now
  exists on Windows (CPack, with the notices installed with the program and the
  Qt runtime inside the package); what remains is the packaged application on
  Linux and macOS, and the Qt window in CI on those two legs.

### Cross-cutting

Fuzz the board parser and dictionary loader; raise the coverage gate beyond its
90% line / 80% branch baseline (the gate itself is in place: `coverage-unit`,
the CI job and `coverage-ui`); a macro-free route through Tracy's runtime
source-location API is **unverified** (the integration uses CMake-selected
variant headers, so the macro API never reaches a call site); a performance
budget for dictionary load and solve on a low-end tablet. Measured so far, on
Windows, Release build:
the whole `wjs-cli` process (load 172,823 words, solve, print) takes about 76 ms
on 3x3, 4x4 and 5x5 boards, with load dominating and solving below the noise
(a Debug build with AddressSanitizer takes several seconds, which is why
performance is measured in Release). Tablet hardware is **unmeasured**.

## 7. Gates before 0.1 and before any store

The repository is public, so what remains is what stands between it and a
release, and between a release and a store.

- **G1 History. Done.** The repository was created from a rewritten history:
  one author identity, and the vendored build trees, binaries, profiler output
  and a word list of unrecorded origin removed from every commit.
- **G2 A word list we may ship. Done for the engine; review pending for
  children.** ENABLE (172,823 words) is in `data/dictionaries/` with its source
  and hash recorded. The original program's list is not in the repository. The category and
  tier lists in `data/lexicon/` were written for this project and every word is
  checked to be in ENABLE, but a teacher or parent should read them before a
  child sees them. Two machine-assembled lists offered as replacements were
  checked and were unusable (1,879 and 8,758 words, with 55% and 49% invented
  entries such as `chooseing`, `circleed`, `lyeer`, and missing `the`, `house`,
  `happy`). A list built by attaching suffixes to stems produces words that do
  not exist; start from a real list.
- **G3 Linux and macOS compile and pass. Done.** The first CI run was green on
  GCC 15.2 and Apple clang 21. Still open: the Qt window is built in CI on no
  platform, so installing Qt 6.12 there is the next piece.
- **G4 Rulesets applied.** Branch protection, the merge queue and the rule that
  rejects any branch not named in the gitflow scheme are in `.github/rulesets`
  and applied with `tools/repo/apply_rulesets.ps1`; they were unavailable while
  the repository was private (403 on the Free plan). See
  [Branch protection](branch-protection.md).
- **G5 Licence decision recorded** for anything going to a store (D2).
- **G6 Privacy and child-safety review** (section 8) before any store listing.
- **G7 Literacy review.** The author has said phonics is not their field. The
  grapheme and phoneme segmentation, the phase contents, the sound-it-out
  wording, the recorded sounds, the hand-filtered word lists and the wording of
  anything shown to a parent are reviewed by a classroom teacher or literacy
  specialist before any child uses them and before any audio is recorded (PRD
  12.6).

## 8. Children, cameras and privacy

Constraints to decide before a camera ships, not a policy:

- Process frames on the device and do not keep them; no network access; no
  analytics or advertising in the child-facing app.
- A camera pointed at a room, in an app for 5-year-olds, is subject to
  children's-privacy rules in several places (US COPPA, the UK Age Appropriate
  Design Code, GDPR Article 8). Apple's Kids Category and Google Play's Families
  policy each add requirements, such as a parental gate for anything outside the
  game. Get these reviewed before M4 reaches a store.
- Hints and rewards should not nag or push the child to continue. The MVP has
  no timers or limits and ends a cleared board on a calm resting screen (PRD
  12.3). A parent-set limit was in the earlier brief and is deferred.
- A child's progress stays on the device, with no cloud tracking, no network
  capability, no voice capture and no export, as the author has stated for the
  MVP (PRD 12.2). A recording of a child's voice and an emailed PDF summary were
  in the earlier brief and are deferred; each is the kind of feature that needs
  this review before it returns, because the first is the most sensitive data
  the application would hold and the second is data leaving the device.

## 9. Questions for you

1. The MVP assumptions in PRD questions 58 to 69, above all the dictionary scope,
   whether the scanner is in the first release, and who the literacy reviewer is.
2. Is the Apple developer account behind CountdownSolver's profile the one to
   use here, and are the family devices' UDIDs still registered? (A new profile
   for this app is needed either way.)
3. D5: who records the phonemes and words?
4. Who reads `data/lexicon/` before a child sees it, and is the category set
   (animals, colours, shapes, food, body, family, nature, home, clothes,
   transport) the one you want for the hints?
