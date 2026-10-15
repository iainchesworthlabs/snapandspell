# Roadmap

Written 2026-10-15, at the end of the restructure, from the
[product requirements](product-requirements.md). It says what exists, what each
requirement needs, the decisions that gate the plan, and the order of work.
Claims that were not verified are marked **unverified**.

## 1. What is being built

WordJuels (the repository is `wordjewelssolver`; see decision D6) is a Boggle
game and companion for 5- and 6-year-olds, on tablets first. Two modes:

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
| Windows build (MSVC 19.51, Qt 6.12.0, `/W4 /WX`, ASan) | Verified | 29 ctests pass |
| Linux and macOS builds | **Unverified** | Standard C++23 and small platform files, but never compiled |
| Android and iOS builds of this app | Not started | CountdownSolver has both working: a signed Android APK, and an ad-hoc signed iOS IPA that has been installed and run on your iPhone and iPad (its CHANGELOG records it; `docs/building.md` there is out of date on this) |
| CI, repository rules, documentation | Written, **never run** | Needs a push to GitHub |
| Touch UI, dice, shake, tracing, audio, rewards | Not started | |
| Child-appropriate lexicon and hints | Not started | |
| Camera scan | Not started | |
| An English word list | ENABLE in `data/dictionaries/`; category and tier lists in `data/lexicon/` | `tools/data/check_wordlists.ps1`. The authored lists are **unreviewed** by a teacher or parent |

## 3. The PRD, requirement by requirement

| PRD requirement | Today | What it needs | Milestone |
| --- | --- | --- | --- |
| Migrate the solver to the tablet | Library is portable C++23, Qt-free | Android and iOS builds of the existing app | M1 |
| Filter by syllables, difficulty, themes | Nothing | A word list with metadata: length, frequency band, syllables, phonics pattern, categories (animal, colour, shape) | M2 |
| 3x3 or 4x4 board | `Board` accepts 1x1 to 16x16 | A dice model (real Boggle dice sets, random rotation, Qu face) in the library | M1 |
| Shake, with the accelerometer | Nothing | Shake detection over accelerometer samples (pure logic, in the library, testable); sensor access through Qt Sensors or a platform interface; a desktop fallback (key or drag) | M1 |
| Haptic feedback | Nothing | A `haptics` interface with Android and iOS implementations under `src/platforms`, nothing on desktop | M1 |
| Drop animation, glowing path, jewels, confetti | Nothing | 2D Qt Quick: Canvas/Shapes/Particles. Not Qt Quick 3D (see D2) | M1 |
| Trace a path with loose tolerance | Nothing | A touch handler that snaps to the nearest tile within a radius, plus `validate_path` in the library | M1 |
| Letters speak their *phonetic sounds* | Nothing | Recorded phoneme audio. Speech synthesis says letter *names* ("see"), not sounds ("k"), so this is a recording job (D5) | M1 |
| Read the found word aloud, name its category | Nothing | Speech synthesis or recordings, plus category tags from M2 | M1, M2 |
| Legible early-reader typeface | Nothing | Andika or OpenDyslexic, both open fonts (licences to verify). Comic Sans is proprietary and cannot be bundled | M1 |
| Hide advanced words, give hints not lists | Solver returns everything | A hint engine over solver output and lexicon metadata: "a 3-letter animal starting with C" | M2 |
| Camera scan of a physical board | Nothing | Board finder, tile reader, confirm-and-correct screen; Vision on iPad, OpenCV elsewhere | M4 |

## 4. Decisions that gate the plan

| | Decision | Recommendation | Why |
| --- | --- | --- | --- |
| D1 | First platforms and how they are distributed | **iPad and Android tablet, both as private builds for your own devices, using the pipelines CountdownSolver already has.** Iterate on desktop and an Android emulator; take iPad builds from CI. | CountdownSolver ships a signed Android APK and an ad-hoc signed IPA (macOS runner, Apple Developer Program certificate and provisioning profile held as repository secrets, an `itms-services` install page) and both have run on your devices. An ad-hoc build installs only on devices registered in its profile, which is what a family tablet needs. A build that only runs on your own devices is not distributed, so no licence or store rule bites yet. Costs to expect: a new App ID and provisioning profile for this app's bundle identifier, which only you can create in the Apple portal, and a CI round trip for every iPad build if you have no Mac. |
| D2 | Licence, when something leaves the house | Decide **before the first public store submission**, not now. Keep `GPL-3.0-or-later`; avoid Qt modules offered only as GPL-3.0-only (Quick 3D). | This is sole-authored code, so dual-licensing or a store exception is possible. The obstacle is Qt on Apple's stores: Cubeulator's `docs/licensing.md` analyses it and its maintainer chose to skip the Apple stores. A 2D shake animation keeps Quick 3D, and so that problem, out of this app. Not legal advice. |
| D3 | UI toolkit on tablets | Qt Quick everywhere, as now. | One UI, the shell exists, and Cubeulator already ships the mobile build plumbing. The library is Qt-free, so a native iPad UI over it stays possible if D2 forces it. |
| D4 | Source of word metadata | Candidates: a pronouncing dictionary (syllables, phonemes), a lexical database with hypernyms (categories), published sight-word lists. | **Licence of each must be checked before use**; none is assumed. Metadata is data, kept out of the code. |
| D5 | Audio | Record phoneme sounds and the words yourself or with a family voice; use speech synthesis only for words and sentences. | Recordings you make carry no third-party licence. A child hears a phoneme, not a letter name. |
| D6 | Name | Pick one: WordJuels (PRD) or WordJewelsSolver (repository, CMake project, CLI). | A rename is cheap now and expensive after a public release. |
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
- Dice model, shake detection, `validate_path`, with unit tests.
- Touch tracing with generous tolerance, tested on a real tablet with small
  fingers (the one thing no unit test can settle).
- Phoneme and word audio; haptics; the typeface; confetti and the jewel bank.
- A 3x3 mode and a 4x4 mode.

### M2: Child-appropriate words and hints

- Lexicon with metadata (D4) and the age-tier filter.
- Hint engine; "nudge, don't spoil" works in Solo Play too: a hint for a word
  not yet found.
- Scoring that rewards a child's first 3-letter word.

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

### M5: store distribution

TestFlight, the App Store and Google Play. Not needed for anything above, and
gated by D2 (licence) and G6 (children's privacy and each store's family
policy). Until then iPad builds stay ad-hoc, on registered devices.

The iOS-specific work of M4 is the Vision-based `board_reader`, which lives
under `src/platforms/ios` as Objective-C++ and is exercised on the iPad.

### Cross-cutting

Fuzz the board parser and dictionary loader; clang-tidy job; a coverage gate
once a baseline exists; Tracy through the opt-in vcpkg feature (its macro API
conflicts with the no-`#define` rule: Cubeulator resolves it with macro-only
variant headers chosen by CMake; a macro-free route through Tracy's runtime
source-location API is **unverified**); a performance budget for dictionary
load and solve on a low-end tablet. Measured so far, on Windows, Release build:
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
- **G3 Linux and macOS compile and pass.** First CI run; expect small fixes.
- **G4 Rulesets applied.** Branch protection, the merge queue and the rule that
  rejects any branch not named in the gitflow scheme are in `.github/rulesets`
  and applied with `tools/repo/apply_rulesets.ps1`; they were unavailable while
  the repository was private (403 on the Free plan). See
  [Branch protection](branch-protection.md).
- **G5 Licence decision recorded** for anything going to a store (D2).
- **G6 Privacy and child-safety review** (section 8) before any store listing.

## 8. Children, cameras and privacy

Constraints to decide before a camera ships, not a policy:

- Process frames on the device and do not keep them; no network access; no
  analytics or advertising in the child-facing app.
- A camera pointed at a room, in an app for 5-year-olds, is subject to
  children's-privacy rules in several places (US COPPA, the UK Age Appropriate
  Design Code, GDPR Article 8). Apple's Kids Category and Google Play's Families
  policy each add requirements, such as a parental gate for anything outside the
  game. Get these reviewed before M4 reaches a store.
- Hints and rewards should not nag, time-limit or push the child to continue.

## 9. Questions for you

1. D6: the name.
2. Is the Apple developer account behind CountdownSolver's profile the one to
   use here, and are the family devices' UDIDs still registered? (A new profile
   for this app is needed either way.)
3. D5: who records the phonemes and words?
4. Who reads `data/lexicon/` before a child sees it, and is the category set
   (animals, colours, shapes, food, body, family, nature, home, clothes,
   transport) the one you want for the hints?
