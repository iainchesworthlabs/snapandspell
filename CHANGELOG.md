# Changelog

*For people following what has shipped. The project overview is the
[README](README.md).*

All notable changes to this project are documented in this file, in the
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) format. This project
adheres to [Semantic Versioning](https://semver.org/). Release notes are the
section of the version released (`tools/release/changelog_section.sh`); the
detail of how and why is in the git history and the pull requests.

## [Unreleased]

Nothing yet beyond 0.1.0. Add a line here for a change a person using the
program would notice.

## [0.1.0] - unreleased

WordJewelsSolver began in 2013 as a Windows-only Win32 and DirectShow program.
0.1.0 is its rebuild as a portable C++23 CMake project, and the first release.
Nothing has been released yet: the release pull request gives this section its
date. **What is verified and what is not** is in
[Requirements and traceability](docs/requirements.md); the short version is at the
end of this section.

### Added

**The solver**

- A solver library with no GUI and no operating system code (`wjs::lib`): board
  parsing (tiles of one to three letters, 1x1 to 16x16), a trie dictionary,
  Boggle and rarity scoring, and a depth-first search that finds every word.
- `wjs-cli`, a command-line solver: a board and a word list in, the words, their
  scores and the tiles that spell them out; `-d`, `-m`, `-s`, `--repeat`, and
  exit codes for each failure.
- The English word list is ENABLE, unmodified, in `data/dictionaries/` with its
  source and SHA-256 recorded; ten category lists and a core tier of 237 words are
  in `data/lexicon/` (checked against ENABLE; nothing reads them yet).

**Solo Play, a game for a five- or six-year-old (Qt Quick, `WJS_BUILD_APP`)**

- Shake a box of dice (a button, Space, a drag, or the device) and the dice fall
  into a board; trace words with a finger or tap them in order; hear each word; collect
  jewels. The board is the project's own dice at 3x3 to 6x6 (re-rolled until a
  board holds a word), played against the full dictionary: **there is no block
  list, so any ENABLE word can be shown and spoken**. A parent can switch to a
  phonics board (words planted from a phase's data and checked by the solver) when a
  phase directory is given; no phase data ships, because it needs a literacy reviewer.
- The 3D shake and drop (Qt Quick 3D) behind one seam, with real six-face dice,
  rigid-body dice, a staged script and a 2D board it settles into; reduced motion
  keeps the box still.
- Stars by word length (+1, +2, +5), points, a start-tile help (off, the start tile,
  or the start tile then its first two letters), **Finish board**, and a summary
  card when a board ends (the design's calm resting screen was not built). A game
  in progress is saved on every find and found again on the next run. **My jewels**
  shows the child's collection: stars, points, words, the best board for each size
  and the week.
- Sound, speech, motion and touch: placeholder sounds owned by the project, the device's
  own voice for whole words, the accelerometer for the shake (Qt Sensors), and a
  haptics interface (a no-op on the desktops). The sound-it-out sequence is built
  and says only the word until phoneme recordings exist.
- The interface in eight languages (English, French, Spanish, German, Yiddish, Hebrew,
  Chinese, Japanese), with right-to-left layouts for Hebrew and Yiddish. The
  translations are mechanical and unreviewed.
- One design system, the Jewel Box (`src/app/qml/kit`): tokens, text roles, buttons,
  choices, cards, sheets, light and dark, text size 100, 125 or 150 %, reduced motion
  that follows the system, and tests that hold contrast, target sizes and the absence
  of hand-written values.

**The parent area**

- Behind a gate that is one two-second press-and-hold: a dashboard (the week, the
  sounds a child has met and found, the words, the best board), settings (help,
  board mode, phase, focus sounds, board size, sound, celebration sounds, sound it
  out, vibration, shake, motion, colour scheme, language, text size, **a board clock**
  that is off by default, counts up or counts down), About and licences, and a
  two-step reset.
- A local store: one small versioned file in the per-user data directory, written
  atomically, bounded, migrated, kept aside when damaged, left alone when newer. It
  holds no duration, identity, image or sound. No audio is captured, nothing is sent,
  and there is no export; source scans check it in CI.

**Mode A, a real board and a helper**

- A reader that finds a board in a camera frame and reads its letters, a
  confirm-and-correct screen so the solver sees only a confirmed board, a letter picker,
  a typed-board path when there is no camera, and an assistant screen that lights a
  start tile. A desktop camera over Qt Multimedia gives a live picture and one
  capture; the frame is read in memory and never kept. It is reached through the parent
  gate. **It has been tested on drawn frames only**; no real board has been read.

**Platforms, packaging and releases**

- Windows, Linux and macOS desktops, with everything that differs between operating
  systems in `src/platforms` and a check that nothing else does; an iOS platform and an
  ad-hoc build for the author's own devices (written, never run on a device); an Android
  build (an APK and an AAB, built and signed on Windows and in CI jobs, never run on a
  device).
- CPack packages: Windows ZIP and NSIS installer (version resource, icon, shortcut),
  Linux AppImage, tar.gz, deb and rpm (the last two the command-line solver), macOS
  bundle and disk image; `NOTICES.txt` and the GPL text beside every package; a
  Release package is checked for Tracy, sanitizer runtimes and debug symbols.
- A release pipeline (`release.yml`): Release builds of every package, an SBOM
  (CycloneDX 1.6), `SHA256SUMS`, GPG signatures, build-provenance attestations, a source
  archive, a rehearsal mode, and `tools/release/verify_release.*` to check a download
  (the key's fingerprint is in [SECURITY.md](SECURITY.md#verifying-a-release)). **It
  has not run on GitHub yet.**
- A documentation site built strictly by `docs.yml` and published from `main`.

**Quality**

- Tests in four tiers (unit, integration, system, ui) plus a quality tier of checks
  about the tree: platform isolation, privacy, packaging, translations, toolchain pins,
  profiling presets, the requirements matrix; Qt Quick Test suites for the screens; a
  coverage gate (90 % lines, 80 % branches over `src/lib`); a performance tier of exact
  work budgets; a smoke mode for the application.
- Profiling with Tracy in Debug and profiling builds only (it opens a TCP port, so no
  release has it), a macro-free zone wrapper, a registry of required zones, trace checks.
- CI on Windows, Linux and macOS with warnings as errors and sanitizers, nightly
  clang-tidy, MSVC analysis, CodeQL and Clang sanitizer jobs, formatting, a text policy for
  commits and pull requests, and repository rulesets as code.

### Changed

- Hardening before release: overflow-safe frame and board arithmetic, checked options and
  errors in the scanner, a store that checks and bounds what it reads, focus sounds that really steer
  the board, a worker thread for the scan read, honest accuracy targets held against a
  third test split, and many analyser findings fixed.
- The scanner's accuracy figures are bounds derived from tuning counts and are looser than
  the first targets (silent-error bounds of about 1 to 2 %, not 0.3 to 0.5 %), because a
  small synthetic corpus cannot confirm less.
- vcpkg is one shared clone found through `VCPKG_ROOT` (not a submodule), pinned by
  `cmake/vcpkg-commit.txt`; the toolchain versions are pinned in one file and held to the
  workflows.
- Rebuilt as a portable C++23 project: the Visual Studio solution is replaced by CMake,
  Ninja and presets; the program is split into a library, front ends and a platform layer.

### Removed

- The Win32 and DirectShow program (the solution, the engine, the capture and renderer
  code, the Win32 window); the code remains in the git history before the rebuild.

### Known limits

- No real board, camera or device has been used. The scanner is tested on drawn frames,
  the iOS and Android builds have never run, and there is no web build.
- No child has played it. Accessibility is by convention and untested with a screen reader;
  there is no live announcement of a found word.
- The phonics board has no data (it needs a literacy reviewer), so the sound-it-out sequence
  has no sounds and the game plays the standard board.
- The eight translations are mechanical, and about a third of each non-English catalogue is
  still the English text.
- Confetti, sparkle and the Mega burst of the design are not built.
- The parent gate is a brake against accidents, not security; "My jewels" shows a child a
  best score; the board has no block list; a parent-set board clock exists although the
  original design had none. Each is a recorded decision
  ([roadmap](docs/roadmap.md#4-decisions-that-gate-the-plan)).
- The performance work budgets run in the Release leg and the release workflow, not in
  the three required Debug legs.
- On Windows, opening a file by a path longer than `MAX_PATH` fails: the executables are not
  long-path aware.
