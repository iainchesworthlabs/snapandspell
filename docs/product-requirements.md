# Product requirements: WordJewelsSolver

A Boggle game and companion for young children. This is the product document
the [roadmap](roadmap.md) is built from. It records the author's brief as it
stands; it deliberately does not finish the design. The board design, the game
design and the application design are the next piece of work and start from
this page, so section 11 lists what they have to settle.

**How to read it.** Everything in sections 1 to 8 is the author's, as stated in
the brief of 2026-10-15 and 2026-10-16. Text marked **Fact** is something
checked in this repository or its sibling projects, included because the design
has to live with it. Nothing here is marked as a proposal: where a decision is
open it is a question in section 11.

## 1. Overview

WordJewelsSolver is an interactive app for 5- and 6-year-olds (Kindergarten and
Year 1) to build reading and vocabulary skills through the mechanics of Boggle.
It has two functions: a standalone digital game, and a smart companion for
physical family play. The goal is educational engagement: celebrating success
and guiding young readers without doing all the work for them.

The name is **WordJewelsSolver** (decided 2026-10-16). The brief's earlier
working name, WordJuels, is retired.

Text is minimal and the interface is highly visual and audio-driven.

## 2. Platforms

The application is to run on:

| Platform | Role |
| --- | --- |
| Android tablet and iPad | The primary targets |
| Phone (Android and iOS) | A supported layout of the same application |
| Web, as WebAssembly | A browser build |
| Desktop: macOS, Linux and Windows | A desktop application |

Private builds that run only on the author's own devices are acceptable for
the mobile targets, as was done for CountdownSolver.

**Fact.** What exists today: the solver library and command-line solver build
and pass their tests in CI on Windows, Linux and macOS, and a small Qt Quick
window (a grid to type a board into, a Solve button, a word list) builds on
Windows. Nothing yet builds for Android, iOS or the browser: `cmake/Platform.cmake`
refuses those targets until each has a platform fragment under `src/platforms`.
The shared Qt 6.12.0 tree on the author's machine
(`C:\Repos\common\qt\6.12.0`) has the Windows, WebAssembly (multithread) and
Android kits, with Emscripten 5.0.5 beside it; it has no iOS kit. Qt comes from
there and from `aqtinstall` in CI, not from vcpkg.

## 3. Solo Play: "I'm playing by myself"

The child plays the game with no physical board.

1. **The board with its lid.** The child sees a Boggle board with its lid on.
2. **The shake.** The board can be shaken, and the shake is animated.
3. **The drop.** The dice fall into their slots.
4. **The layout.** The board then moves to the side and occupies two-thirds of
   the screen. The remaining third is a panel split into three parts:
   **words I found**, **history**, and **hints**, and that sort of thing.
5. **Tracing.** The child traces a word with a finger over the letter dice on
   the board. Letters light up and speak their phonetic sounds as they are
   traced (original brief).
6. **Playing on.** When the child is done, a button reshuffles and starts a
   new game, and they can play another game and find more words.
7. **Gamification.** There can be high scores and a little gamification, "five
   year old gamification": a new high score or similar, "not crazy".

From the original brief, kept: shaking uses the device's accelerometer with
haptic feedback and rattling sound effects; success shows vibrant rewards
(jewels, confetti, stars) and reads the found word aloud ("C-A-T. Cat! You
found an animal!"); found words become colourful jewels collected in a bank; a
selected path is a glowing line; type is a clear, highly legible sans-serif
designed for early readers (OpenDyslexic or Andika).

## 4. Family Companion: "I'm playing with the family"

The family plays with a real Boggle board and the app is an aid for the child,
so they can compete fairly with adults and teenagers who have a much larger
vocabulary.

1. **The setup.** The tablet is on a stand and its camera is on.
2. **The real game.** A parent shakes the real board and puts it down in view
   of the camera, then lifts the lid off.
3. **The scan.** The camera scans the board and runs OCR on all the letters.
4. **The preloaded board.** The board in the app is now loaded with the real
   one. The child can pick words, and the things they can find, while playing
   against the other players.
5. **A helper, not a game.** The app acts as a helper. It provides
   **tailorable levels of help**, tuned to the child's age and to how much
   assistance the family wants to give.
6. **A new round.** A new game starts with a new scan: the board is shaken,
   put down and scanned again, so there is some form of "go" for a new game.

From the original brief, kept: a smart filter so the app hides advanced words
and indexes only age-appropriate ones (for example 3- to 4-letter words, sight
words, simple phonics); and a **nudge, not a spoiler**: instead of listing the
words, it highlights a starting letter on a digital replica of the scanned board
and asks, for example, "Can you find a 3-letter animal starting with 'C'?"

## 5. Board sizes

- **Standard: 4x4.**
- **Optional: 5x5 or 6x6**, selectable, where 6x6 is a "Super Big Boggle"
  board. The original brief also named a simplified 3x3 for the youngest
  players.
- The dictionary would need to be updated for the larger boards. That is a
  separate problem, to be worked on separately.

**Fact.** The solver library already takes any board from 1x1 to 16x16, with
tiles of one to three letters (the "Qu" face is a two-letter tile). It does not
yet have dice sets: the letter distributions of the physical dice for each size
are data to be sourced.

## 6. Help

Help is tailorable, in both modes: in Solo Play it is the hints part of the
panel; in the Family Companion it is the whole point of the application. The
levels are set to the child's age and to how much assistance is wanted.
What the levels are, and what each does, is open (section 11).

## 7. Look, feel and responsiveness

- The **animation has the same style as the Rubik's cube in Cubeulator**, and
  the ethos and design pattern of the application are to be similar to that
  application.
- It is for a child, so it has **big buttons**.
- It must be **responsive**.

**Fact, from Cubeulator.** It draws its cube with Qt Quick 3D. Qt Quick 3D is
offered only under GPL-3.0-only or a commercial licence, which is why
Cubeulator recorded, and deferred, the Apple-store question. Its design
documents set a 48 px minimum touch target, 56 px on the scan screen and 96 px
on the solve screen. See [Licensing](licensing.md).

## 8. Performance

- Performance is tracked with **Tracy**.
- There are **performance tests** around the application.

**Fact, from Cubeulator.** Tracy is in an opt-in build (a `profiling` preset
and a vcpkg feature), runs in on-demand mode so an idle instrumented build does
no work, and is instrumented through the camera thread, the Qt Quick frame and
the solver. Its documents give budgets (a 33 ms camera frame, a 16.7 ms UI
frame at 60 Hz), and its `tests/perf` tier holds two kinds of threshold: a
*work* budget counted exactly (search nodes visited), which can be tight, and a
*time* budget set as a ratio to a calibration workload, which is deliberately
loose so a busy machine never fails it.

**Fact, here.** Tracy's macros collide with this repository's no-`#define` rule.
Cubeulator resolves it with macro-only variant headers chosen by CMake; a
macro-free route through Tracy's runtime source-location API has not been tried.
The solver measures about 76 ms for a whole `wjs-cli` run (loading 172,823 words
and solving) in a Release build on a desktop; nothing has been measured on a
tablet.

## 9. Implementation phases

From the original brief, updated for the platform scope above.

**Phase 1: core engine.** Migrate the word solver to the application (done: a
portable library). A dictionary filter that categorises words by syllable
length, difficulty and child-friendly themes (animals, colours, shapes): the
word lists in `data/lexicon` are a first cut.

**Phase 2: input and interaction.**
- Gesture recognition: a smooth touchscreen path-drawing mechanic over the
  grid, with loose tolerances so small fingers are not frustrated by straying
  slightly off a tile.
- Computer vision: identify a grid layout and read text from angled physical
  dice faces (for example with Apple's Vision framework on iOS), for the Family
  Companion.

**Phase 3: audio and animation.**
- Text-to-speech or pre-recorded audio on a successful word.
- The shaking simulation, using the accelerometer, and the dice falling into
  their slots.

## 10. Settled constraints

These are decided elsewhere and the design has to fit them.

- Licence **GPL-3.0-or-later**; see [Licensing](licensing.md).
- Everything that differs between operating systems lives under
  `src/platforms`, chosen only by `cmake/Platform.cmake`; no `#if` and no
  `#define` in portable code ([Platform isolation](platform-isolation.md)).
- The solver library is Qt-free and platform-free, and the application is a
  thin layer over it.
- Qt 6.12.0, from the shared tree or `aqtinstall`; other dependencies from the
  vcpkg submodule.
- Camera frames are not committed to the repository, and the application is for
  a game played with children ([Licensing](licensing.md#camera-input-and-letter-recognition)).

## 11. Questions for the design work

The board design, the game design and the application design have to settle
these. They are questions, not recommendations.

**Game**
1. What does **history** hold: the words of this game in the order they were
   found, the words of earlier games, or both?
2. What are the **help levels**: how many, what each reveals (a count, a first
   letter, a length, a category, the word itself), and how a level is chosen (a
   setting a parent sets, a dial, or both)?
3. What counts as a word the child may find? Should a valid word outside the
   child's vocabulary tier be accepted, with only the hints drawn from the tier?
4. What scores, and what is a "new high score" scored against: per board size,
   per child, per device?
5. In the Family Companion, does the app keep the child's score against the
   other players, or only help?

**Board**
6. Which physical dice distributions does each size use, and where do they come
   from (the licence of the data matters)?
7. How is a 3x3 board related to the others, if it exists?
8. How are the 5x5 and 6x6 layouts shown in the two-thirds area on a phone?

**Scanning**
9. What starts a scan: the lid coming off and the board being still, a button,
   or both?
10. How are recognition errors corrected, and how visibly?
11. How is the board size determined: from the picture, or chosen first?

**Application**
12. How does the two-thirds / one-third layout behave in portrait, on a phone
    and in a narrow browser window?
13. Is the shake animation a rendered 3D scene (the Cubeulator look, with the
    licence consequence in section 7), something drawn in 2D, or scripted?
    Does it need a physics engine at all?
14. What is the frame-time and solve-time budget on the lowest tablet and phone
    to be supported, and which parts of it are measured by a test?
15. What does the application do without a camera (the browser, a desktop with
    none): is Solo Play the whole of it there?
