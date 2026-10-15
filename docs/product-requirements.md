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
2. **The shake.** The board can be shaken, and the shake is animated. The
   author's expectation is a **3D dice shake in a virtual 3D container**, and
   the tablet's **accelerometer** can be used to shake it, which "would be fun
   as well". The shake comes with **haptic feedback** and an **audio cue**.
3. **The drop.** The dice fall into their slots, and a **grid presentation**
   shows the board when the cubes settle.
4. **The layout.** The board then moves to the side and occupies two-thirds of
   the screen. The remaining third is a panel split into three parts:
   **words I found**, **history**, and **hints**, and that sort of thing.
5. **Tracing.** Interaction is by gesture: the child traces a word with a
   finger over the letter dice on the board, and the tiles **illuminate
   dynamically** along the path. Letters light up and speak their phonetic
   sounds as they are traced (original brief).
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

### 7.1 Gamification and child-centric UI requirements

To keep early readers engaged, the interaction design follows strict
kid-centric feedback loops (the author's requirements, 2026-10-16):

- **Celebration mechanics.** When a valid word is completed by tracing, a
  cascade of visual praise is triggered (for example confetti particles or
  floating stars), paired with audio pronunciation. The app speaks the word
  aloud: "C-A-T... Cat! Great job!"
- **Progress tracking.** No negative point penalties and no harsh buzzers for
  invalid words. Simple star counters or collectible virtual badges reward
  successful sessions.
- **Accessible typography.** Large, child-friendly sans-serif fonts in which
  the distinction between lowercase and uppercase is very clear, avoiding the
  ambiguous shapes of "I", "l" and "1".

These sit alongside the high scores and light gamification of section 3, and
mean that nothing in the application subtracts from a child's progress.

### 7.2 Progression and vocabulary growth

Stated by the author on 2026-10-16. To maintain long-term novelty, and to drive
the "aha!" moment when a child discovers big words, the application has a
progression system built around vocabulary growth and personal triumph.

**The big-word triumph mechanic.** Longer or phonetically advanced words are
treated as milestone discoveries rather than just points.

| Word tier | Examples | Visual and audio feedback | Reward |
| --- | --- | --- | --- |
| Spark Words (3 letters) | CAT, FOX, SUN | Soft sparkle animation, standard word pronunciation | +1 star |
| Power Words (4 letters) | RAIN, STAR, MOON | Rainbow trailing effect on the text, cheering sound effect | +2 stars, and unlocks a "Word Sticker" |
| Mega Words (5+ letters) | TRAIN, HOUSE, TRUCK | Screen shake, screen-wide confetti explosion, a voiceover ("Whoa! A 5-letter Mega Word!") | +5 stars, and unlocks a character accessory |

**The word vault ("My Word Vault").** A visual place for the child to see
growing knowledge ("I know how to spell RAIN!"), replacing progress bars with a
collection mechanic.

- **The sticker book.** Every time a new word is found, it is added to a digital
  sticker album. Finding RAIN unlocks a little raincloud sticker; finding STAR
  unlocks a shining star sticker.
- **Tap to talk.** In the sticker book the child can tap any sticker they have
  unlocked, and the application reads it aloud and shows it in a sentence, for
  example "R-A-I-N spells rain! I love jumping in the rain!"
- **Missing outlines.** Words not yet found appear as greyed-out outlines, which
  creates a gentle curiosity to play more and unlock the hidden pictures.

**Levels and novelty ("World Unlock").** To keep the game fresh without making
it complex, worlds unlock by the total stars collected. The board design and the
background environment change as stars accumulate.

| Level | Stars | World | Board look | Words it unlocks |
| --- | --- | --- | --- | --- |
| 1 | 0 to 20 | The Cozy Farm | Tiles look like wood | COW, HEN, BARN |
| 2 | 21 to 50 | Space Explorer | Tiles look like glowing neon planets | STAR, MOON, MARS |
| 3 | 51 and up | Deep Blue Sea | Tiles look like bubbles | FISH, SHARK, BOAT |

**The board companion.** A simple cartoon mascot (for example a little
bookworm or a friendly robot) sits next to the grid. Collecting stars unlocks
simple cosmetic accessories for it, such as sunglasses, a party hat or a cape.

**Dynamic vocabulary scaling (technical note).** To keep the game fun and
prevent frustration, the back end adjusts the game board to the child's current
tier automatically:

- **Early levels:** the grid generation forces a higher concentration of simple
  CVC (consonant, vowel, consonant) letter combinations.
- **Advanced levels:** letters such as R, S, T, A, I and N are allowed to
  cluster together more often, to raise the chance of finding the larger words,
  such as RAIN and STAR.

**Fact.** Nothing here exists yet: the application stores no progress of any
kind. The solver library can already list every word on a given board with its
path and a letter count (the "Qu" tile counts as two letters), so the tier of a
found word and the number of words of each tier on a board are computable. The
category lists in `data/lexicon` (animals, colours, shapes, food and others)
hold words only: there are no pictures, sentences or recordings for them. The
brief's two descriptions of RAIN and STAR differ (see question 20).

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
16. What does the application do and say when a traced path is not a word, or
    is a real word outside the child's vocabulary tier? Section 7.1 rules out a
    penalty and a harsh buzzer; what replaces them?
17. What are the stars and badges earned for (a session, a number of words, a
    long word, a new category), what do they look like, and where are they kept?
18. Which typeface meets the clarity requirement of section 7.1 when the actual
    letters "I", "l", "1", "O" and "0" are set at the sizes used on the dice,
    and does it also suit the on-screen replica of the physical dice in the
    Family Companion? This is checked by looking at the glyphs, not assumed from
    a face's description.

**Progression (section 7.2)**
19. How is a word's tier decided: by its length in letters only (so the "Qu"
    tile makes "QUIT" a 4-letter Power Word), and are inflections such as RAINS
    new words with their own sticker?
20. The tier table makes RAIN and STAR Power Words (4 letters), while the
    technical note calls them Mega Words; which is meant, and is the 5-letter
    threshold the one the board generation aims at?
21. Which words have a sticker, and so what is the closed set behind the
    greyed-out outlines? Who draws the pictures and writes the sentences, and
    under what licence, given that each needs art and text the repository does
    not have?
22. What does a level's "words it unlocks" do: add those stickers to the book,
    make those words likelier on the board, or both? Every valid word is found
    and counted in any case, so what is locked?
23. The thresholds (0 to 20, 21 to 50, 51 and up) against rewards of +1, +2 and
    +5 stars are reached within a session or two: how many levels are there, and
    what is the pace?
24. Biased board generation changes what a "shake" means. How far may the
    letters be steered, how is a board kept fair and varied, and what is
    measured (for example, the number of words of each tier on the board)?
    The Family Companion scans a real board and cannot be steered: what does
    the tier do there?
25. Screen shake, a screen-wide confetti explosion and a loud voiceover are
    strong effects for a young child: what is the reduced-motion and
    reduced-sound behaviour, and who sets it?
26. Whose voice says the voiceover lines and the sentences, recorded or
    synthesised, and in which languages?
27. Whose progress is kept: one child per device, several children, or a
    parent-managed profile; where is it stored (on the device only, as the
    privacy constraints in the roadmap assume); and how is it reset?
28. What is the mascot, who designs it and its accessories, and is it the same
    character in every world?
