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

**MVP scope.** The author's categorisation of the first release is section 12.
It narrows this brief, corrects two slips in it, and in places contradicts
earlier statements. The author has said that some of its details are probably
wrong, so it is written to be planned around: each item is classed as adopted,
corrected, a planning assumption or needing expert review, and the
assumptions are questions (58 to 69) to confirm, not decisions.

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
- **School fonts, as a setting.** In Australia schools use specific fonts
  tailored to each state, for example Vic Modern Cursive for Victoria, WA and
  the NT, or the NSW Foundation font. Supporting these would be a good settings
  choice.

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
brief's two descriptions of RAIN and STAR differed; RAIN and STAR are Power
Words (4 letters), as section 12.1 settles.

### 7.3 Phonics mode: Australian synthetic phonics

Stated by the author on 2026-10-16. In Australian Prep and Year 1 classrooms the
curriculum focuses heavily on synthetic phonics, where children learn phonemes
(the sounds) and graphemes (the letters or letter combinations that represent
those sounds, such as "ay", "ai", "sh" or "ch"). Integrating this shifts the
application from a simple letter-matching puzzle to a phonics solver, bridging
the backend solver with the classroom curriculum.

**The grapheme-block system (phonic tracing).** Instead of treating every letter
tile as a standalone sound, the game engine dynamically binds vowel teams and
digraphs together when they are selected.

| Phonic sound (grapheme) | Classroom examples | How tracing handles it |
| --- | --- | --- |
| Long A (/eɪ/) | J-AY, R-AI-N, P-L-AY | When the finger drags from R to A, the A and I tiles link together as a single grapheme block (the vowel team), not the I and N, to highlight the phoneme sound (corrected, section 12.1) |
| Digraphs (/ʃ/, /θ/) | SH-I-P, FI-SH, CH-I-P | The S and H tiles are visually linked with a subtle glow-colour border to show they make one sound together |
| Split digraphs (magic E) | C-A-K-E, L-I-K-E | Tracing C-A-K-E triggers a magical line linking the A and E across the K, to show the E makes the A say its name |

**The "sound it out" audio engine.** When a word is completed, the celebration
does not read the word flatly: it decodes it as teachers do in the classroom.

1. **Segmenting.** Each sound block is highlighted in turn. For RAIN: R (the
   /r/ sound), then AI (the long /eɪ/ sound, corrected from /aʊ/, section
   12.1), then N (the /n/ sound).
2. **Blending.** The letters slide closer together while the audio plays
   "/r/ ... /eɪ/ ... /n/ ... RAIN!"
3. **Success cheer.** "Fantastic! You used the /ay/ sound to make RAIN!"

**Phonics-targeted levels (the decodable approach).** Progress is tracked by
phonics phase, following the sequence a school teaches, rather than only by
word length. No programme is named in this repository; the MVP uses the generic
names Phonics Phase A (fundamental SATPIN) and Phonics Phase B (consolidated
blends), section 12.1. The three-step list below is the author's fuller
sequence.

- **Phase 1 grid (early Prep):** only simple single-letter phonemes (SATPIN
  words: SAT, PIN, TAP, PAN).
- **Phase 2 grid (mid to late Prep):** double consonants and simple digraphs
  (for example CHIP, FISH).
- **Phase 3 grid (Year 1):** complex vowel teams such as AY (JAY, PLAY, DAY) and
  AI (RAIN, TRAIN, SAIL).

**The phonics sticker book (the sound collection).** The progress tracker can be
sorted by vowel sound as well as by theme. The child opens a "Long A page" in the
sticker album and sees outlines for JAY, RAIN and PLAY. Tapping the page header
plays: "This is the /ay/ page! Find words on the board that make the /ay/ sound
to unlock their stickers!"

**Fact.** Nothing here exists yet. The library takes tiles of one to three
letters, so a digraph tile (SH, AI) is expressible and the solver reads such a
tile as its letters in order; and the path of a found word is returned as tile
positions, which is what grouping tiles into a grapheme block would build on.
There is no grapheme or phoneme data of any kind in the repository, and the
dictionary has no pronunciations. Speech synthesis says letter names and whole
words; isolated phonemes ("/r/", not "ruh") are a recording job. Filtering the
dictionary to words of three or more letters spelled only with S, A, T, P, I
and N gives 249 words (43 of three letters, 70 of four, 54 of five, up to ten
letters), including entries a child does not meet (AIS, AIT, ANA, ANTI, PIA,
PSI, TAS, TIS); of those 249, five are in the early-reader tier of
`data/lexicon/core-words.txt` (NIP, PAT, SAT, SIT, TAP). The 249 is the count
before any stripping: it includes entries unsuitable for a child (for example
ASS, TIT and PISS), and 127 of the 249 are plurals ending in S. The brief's
example words RAIN, TRAIN, JAY, PLAY, CHIP and FISH need letters outside S, A,
T, P, I and N, so they are not in that set; PAIN, PAINT, SAINT and STAIN, which
carry the same "ai" vowel team, are. Every example word in the brief is in the
full dictionary.

### 7.4 Parent dashboard

Stated by the author on 2026-10-16. The parent dashboard turns the application
from a casual game into an insightful educational tool. For an Australian Prep
or Year 1 context it bridges home play and classroom learning, tracking phonics
progress without making the child feel they are taking a test.

**MVP scope.** For the first release the "read aloud" voice recording, the
printable summary and the time limit are outside the MVP, and the dashboard is
shown on the device and nowhere else (section 12). They stay here as the
author's brief until it is confirmed whether they are removed or only deferred
(question 62).

**The sound mastery heatmap.** Instead of basic percentage scores, the dashboard
visualises which phonemes and graphemes (sounds and letter teams) the child is
identifying on the board and which they are overlooking.

| Focus | Metrics tracked | Parent-actionable insight (example copy) |
| --- | --- | --- |
| Mastered sounds | Found 3 or more words containing "ay" (JAY, PLAY) or "ai" (RAIN) unassisted | "They completely understand the Long A vowel team. Ready for trickier sounds!" |
| Emerging sounds | Selected the starting letter of a "sh" word but did not finish the digraph block | "They see the letters but are still learning to blend them together." |
| Blind spots | Completely missed available "ch" words over multiple boards | "Next time you play the physical Family Mode, point out a 'ch' word to support them." |

**Classroom alignment (curriculum sync).** Australian schools introduce sounds
in explicit, systematic sequences (a school's phonics programme), and the
dashboard lets a parent match the application to what is happening at school.

- **The "focus sound" selector.** A simple toggle screen where a parent ticks the
  specific sounds the child is learning at school this week (for example "ai",
  "ay" and "ee").
- **Targeted board seeding.** Once sounds are selected, the virtual Boggle
  backend dynamically forces the digital dice to generate grids with an
  abundance of those target sounds, reinforcing school homework organically.

**The weekly report card.** At the end of each week the dashboard generates a
punchy, positive summary that gives the parent concrete things to praise.

- **Milestone callouts.** "This week, they discovered their very first 5-letter
  Mega Word: TRAIN!"
- **The "read aloud" shared moment.** Saves a small audio snippet of favourite
  word successes. The parent taps a button in the dashboard to hear the
  recording: "T-R-A-I-N... Train!"
- **Family play insights.** In the physical Family Companion mode, it tracks
  which hints were tapped the most, showing where reading confidence sits when
  playing together.

**Time played.** A "Time Played" tracker with a gentle, parent-configured
screen-time limit.

**The printable summary.** The dashboard generates a printable PDF summary that
can easily be emailed to the child's Prep or Year 1 classroom teacher for
parent-teacher interviews.

**Privacy and local storage (technical note).** Because this involves a child's
learning data, absolute data privacy is required.

- **Zero cloud tracking.** All dashboard metrics, progress history and voice
  clips are stored locally on the device, not on a remote cloud server.
- **A parental gate.** Reaching the dashboard requires a simple maths
  verification lock or a long-press action, so the child does not wander into
  the data screens by accident and break the gamification illusion.

**Fact.** None of this exists: the application stores no progress, has no
microphone input, and has no code to produce or share a PDF. Which words were
available on a board, and which were found, are computable from the library's
output; the sound-level metrics depend on the grapheme data of section 7.3,
which does not exist. The "read aloud" snippet is a recording of a child's voice.
The printed summary and the email are the first route by which a child's data
would leave the device, started by a parent. The roadmap's privacy constraints
(section 8 there) assumed local-only storage; this section now states it.

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
  shared vcpkg clone (`VCPKG_ROOT`).
- Camera frames are not committed to the repository, and the application is for
  a game played with children ([Licensing](licensing.md#camera-input-and-letter-recognition)).
- A child's learning data stays on the device: all dashboard metrics, progress
  history and voice clips are stored locally, with no cloud tracking (section
  7.4), and reaching them takes a parental gate.
- For the MVP (section 12): no voice capture, no export of data, no network
  capability, no timers or limits, and a calm resting screen when a board is
  cleared. The privacy architecture is frozen before any visual assets or
  layouts are designed.

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
20. **(Settled, section 12.1: Power Words.)** The tier table makes RAIN and STAR Power Words (4 letters), while the
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

**Phonics and school fonts (sections 7.1 and 7.3)**
29. Is phonics a mode, a setting or the default, and how does it relate to the
    word-length tiers, stars and worlds of 7.2: two axes of progress, or one?
30. Does tracing bind adjacent single-letter dice into a grapheme at selection
    time, or do dice carry multi-letter faces (the library takes tiles of up to
    three letters)? What happens when the letters are traced in an order that
    is not the grapheme?
31. **(Settled, section 12.1: A and I link, and the sound is /eɪ/.)** Two details in the brief need confirming. The RAIN tracing example lights
    "the I and N" together, where the vowel team of RAIN is A and I. And the
    audio example gives the sound of AI as /aʊ/, the vowel of "cow", where
    /eɪ/ ("ay"), as the brief says elsewhere, is meant.
32. Where does the segmentation of each word into graphemes and phonemes come
    from: hand-authored for a decodable list, or derived from a pronunciation
    lexicon? How are tricky words that do not follow the pattern (said, the,
    one) shown, and do the sounds follow Australian English?
33. **(Reworded, section 12.1.)** No programme is named in the repository now.
    The MVP uses the generic names Phonics Phase A (fundamental SATPIN) and
    Phonics Phase B (consolidated blends). What exactly does each contain, in
    particular where do the vowel teams such as AI and AY sit (the brief
    elsewhere places them in a third step), and is there a Phase C? This is for
    the literacy review of section 12.6.
34. How is each phase's word set made: by filtering the dictionary to words
    spelled with the taught graphemes (249 words for SATPIN, including entries
    a child does not meet), or by an authored decodable list, and who reviews it?
35. Does a phase constrain board generation to its graphemes, what happens when
    a board then has too few words, and how does that combine with the steering
    of 7.2 and the focus sounds of 7.4?
36. Is the phonics sticker book the same album as the word vault of 7.2,
    arranged two ways, or a second book, and does a word sit on several pages?
37. Who records the phonemes and graphemes, in what accent and in what set, and
    how are the blending animation and the audio kept in step?
38. Which school font belongs to which state and territory (the brief gives Vic
    Modern Cursive for Victoria, WA and the NT and the NSW Foundation font for
    NSW; the others are to be confirmed), and may each be redistributed in the
    application? A commercial font cannot be bundled without a licence, and a
    tablet cannot be assumed to have it installed.
39. What does the font setting change: the dice letters, the word panel, the
    stickers, the captions? How are joined cursive letters shown when the dice
    are print capitals?
40. Where a school font's letterforms are ambiguous (the "I", "l" and "1" of
    7.1), which requirement wins, and is the font setting then limited?

**Parent dashboard (section 7.4)**
41. **(Outside the MVP: no voice is captured; section 12.2 and question 62.)** What exactly is recorded for the "read aloud" moment: the child saying a
    word, when, prompted by what, under which microphone permission, kept for
    how long and deleted how? A recording of a child's voice is the most
    sensitive data the application would hold, and the children's-app policies
    of the stores should be read before it is designed.
42. How are "mastered", "emerging" and "blind spot" defined (the brief gives 3
    or more words unassisted, an abandoned trace, and missed across multiple
    boards): the thresholds, the time window, whether a sound can fall back, and
    how "unassisted" is known (no hint used on that word)?
43. How is an abandoned trace told apart from a stray touch, and is that
    inference reliable enough to show a parent as a finding?
44. How are the findings phrased (as observations, not assessments), and what
    guards against reading a gap in play as a deficit?
45. How do the focus sounds relate to the phases of 7.3 (a phase is a set of
    sounds, the selector picks individual ones), and what does seeding do when
    too few words contain the chosen sounds?
46. Seeding steers a virtual shake (question 24); the Family Companion board is
    real and cannot be seeded. Does the dashboard there only track and advise,
    as the "point out a 'ch' word" example suggests?
47. When is the weekly report produced and shown (when the dashboard is opened,
    or by a local notification, which needs a permission), and where does the
    week begin and end?
48. Which hints count as "family play insights", and who taps them, the child or
    an adult, and how is that told?
49. What is the parental gate (a maths question, a long press, or both), what
    age is assumed unable to pass it, and how does it meet each store's rules
    for children's apps?
50. "On the device" means different things on a tablet, in a browser (storage
    the user can clear) and on a desktop. What happens when the family changes
    device, and is an export or backup wanted, which would be data leaving the
    device and so a decision of its own?
51. Are there several children with separate heatmaps on a shared device, or
    one?

**Time played and the printable summary (section 7.4)**
52. What counts as time played (the application open, the screen on, active
    play), per child or per device, per day or per session, and when does the
    day end?
53. **(Outside the MVP: no timers or limits; section 12.3 and question 62.)** What does "gentle" mean at the limit: a warning beforehand, a wind-down that
    lets the current word finish and saves progress, what the child sees and
    hears, and can a parent extend it? Does it end play for the rest of the day?
    A time limit also sits against the requirement that nothing pushes the child
    to continue.
54. How does the application's limit relate to the operating system's own
    screen-time controls, and what does it do in a browser, which has none?
55. **(Outside the MVP: no export; section 12.2 and question 62.)** What does the printable summary contain (the heatmap, the weekly highlights,
    time played, a name), what is deliberately left out (voice clips, anything
    not wanted by a teacher), and does the parent see a preview before it goes?
56. **(Outside the MVP: no export; section 12.2 and question 62.)** How does the PDF leave the application on each platform (the system share
    sheet, a print dialog, a download in the browser)? It is the first route by
    which a child's data leaves the device, so what does the parent confirm, and
    is the parental gate required?
57. **(Outside the MVP: no export; section 12.2 and question 62.)** Does the application know the teacher's address? Storing a third party's
    contact details is data of its own; the alternative is to hand the PDF to
    the system's mail and share facilities and keep no address.

**The MVP scope (section 12): assumptions to confirm**
58. *Dictionary scope.* Assumed: the Phase A index is a hand-filtered, reviewed
    list derived from the SATPIN-spelled dictionary words, and so has fewer
    than the 249 counted before stripping; and each board offers three to five
    words from it. Confirm, and say who writes and reviews the list.
59. *The five.* Assumed: NIP, PAT, SAT, SIT and TAP are the starting subset and
    the index grows beyond them. If they are the whole accessible set, every
    board offers the same five words: is that wanted?
60. *RAIN.* RAIN needs R, which is not a SATPIN letter, so the MVP's own Solo
    example is a Phase B word. Assumed: Phase A shows grapheme blocks with
    SATPIN-only words (PAIN, SAINT and STAIN carry "ai"). Is Phase B in the MVP,
    and what is in it?
61. *A real word that is not on the board's list.* Assumed: a gentle, neutral
    cue, no penalty and no word called wrong (section 7.1). What does the child
    see and hear when they trace TAP and only SAT and SIT count?
62. *Removed, or deferred.* Assumed: the voice recording, the printable summary,
    the timers and the parent-set limit are deferred beyond the MVP and none is
    designed now, not deleted from the product; and a passive time-played
    figure is not in the MVP either. Confirm, or say any of them is removed for
    good.
63. *After a board is cleared.* The solo flow of section 3 has a reshuffle
    button and high scores; the MVP has a calm resting screen that encourages
    putting the device down. Assumed: the resting screen keeps one quiet way to
    play again, started by the child or a parent and never automatically. What
    does it show, and how are high scores kept without pulling the child back?
64. *Platform.* The categorisation says the iPad throughout. Assumed: the MVP
    is the iPad first, as a private ad-hoc build, and the platform list of
    section 2 is unchanged as the product's scope. Confirm.
65. *Zero network capability.* How is it kept true, and checked, on each target
    (the Android permission, the iOS build, the browser build, and the
    profiling build that listens on a port and is never shipped), and is a CI
    check that the shipped targets link no network module wanted?
66. *Programme names in history.* The names were removed from the documents on
    this branch, but earlier commits of the unmerged branch still contain them
    and a pull request would carry them. Is a rewrite of that branch wanted (a
    force-push to a branch only the design session uses), or only the working
    tree?
67. *The scanner in the MVP.* Mode A is the camera-to-board pipeline, the
    riskiest piece, which the roadmap had after Solo Play. Is it in the first
    MVP? Assumed: a single capture with a confirm-and-correct step, since an
    OCR error would otherwise feed a wrong board to the solver, rather than the
    stand-and-watch flow of section 4.
68. *Help in Mode A.* The MVP highlights only the starting tile. Assumed: that
    is the one help level in the MVP, with the tailorable levels of sections 4
    and 6 after it. Confirm.
69. *The reviewer.* Who is the literacy specialist or classroom teacher who
    reviews the phonics content (section 12.6), and at what point: before any
    audio is recorded, or before the first child uses it?

## 12. MVP scope

Stated by the author on 2026-10-16 as a categorisation of the first release,
with the caveat that some of its details are probably wrong: phonics is not the
author's field, so the plan assumes errors and corrects for them. Each item is
classed as one of:

- **Adopted**: taken as stated.
- **Corrected**: changed, with the reason, and the author's wording quoted so
  the change can be seen.
- **Planning assumption**: a resolution adopted so work can go on where the
  categorisation contradicts itself or an earlier statement. It stands until
  the author says otherwise (questions 58 to 69).
- **Needs expert review**: whether it is right depends on a literacy
  specialist, not on the author or on engineering (section 12.6).

### 12.1 Corrections and dictionary scope

| Element | As categorised | Class | MVP specification |
| --- | --- | --- | --- |
| Grapheme binding | "When tracing RAIN, the A and I tiles link together as a single grapheme block, not the I and N" | Adopted | The vowel team of a traced word is the unit that lights |
| Phonetic notation | "The long 'A' sound found in JAY and RAIN is accurately encoded as the /eɪ/ phoneme" | Adopted | /eɪ/ throughout, in text and in audio labels |
| Word tier | "RAIN and STAR are strictly classified as Power Words (4 Letters)" | Adopted | Power Words; Mega Words begin at five letters |
| Dictionary scope | "a hardcoded, hand-filtered index of exactly 249 SATPIN / Early CVC words, stripped of all obscure vocabulary. Only the 5 standard early-reader tier words are accessible initially" | **Corrected** | A hand-filtered Phase A index, written and reviewed, derived from the SATPIN-spelled dictionary words by removing what is unsuitable. Its size is whatever remains: fewer than 249, because 249 is the count *before* stripping. Each board offers a handful (three to five) of viable words drawn from it, so boards differ while each is quickly cleared. NIP, PAT, SAT, SIT and TAP, the five that are in the early-reader tier, are the subset to start from and not the whole index: used alone, every board would offer the same five words |
| Curriculum names | "Strip all specific mentions of proprietary programs ... Use generic ... terms: Phonics Phase A (Fundamental SATPIN), Phonics Phase B (Consolidated Blends)" | Adopted and applied | The names are removed from this repository's documents; Phase A and Phase B are the names |
| What is in Phase B | "Consolidated Blends" | **Needs expert review** | "Blends" usually means consonant clusters (as in TRAIN and STAR), while AI and AY are vowel teams, which the earlier brief placed in a third step. Where each sits is for the review |

**Fact.** The index does not exist. The dictionary has 249 words of three or
more letters spelled only with S, A, T, P, I and N, and that set holds entries
no child should meet (including ASS, TIT and PISS) and 127 plurals. The
dictionary data and the early-reader tier are described in section 7.3.

### 12.2 Privacy architecture

Frozen before any visual assets or layouts are designed. All three are
**adopted**.

- **No audio recording.** No child voice is captured, processed or stored by the
  application. Pronunciation audio is output only, from the device's native
  text-to-speech or from pre-rendered audio assets. (Isolated sounds are not
  something speech synthesis does well, so the sounds are pre-rendered
  assets, section 7.3, question 37.)
- **An air-gapped data model.** Progress metrics, found words and sticker
  milestones are stored in a local, sandboxed database on the device. The
  application has no network capability, no external API hooks and no cloud
  analytics.
- **Zero export.** The printable PDF summary is removed from the MVP. Data does
  not leave the device; to share progress with a teacher, a parent shows the
  dashboard on the device itself at the parent-teacher interview.

**Planning assumption.** These are constraints on the MVP. The brief's earlier
voice recording, PDF and e-mail are deferred, not designed, and would each need
the privacy review before they came back (question 62).

### 12.3 Screen time and game loops without coercion

**Adopted.** Nothing micro-manages the child or pushes them to continue.

- **No timers or limits.** No ticking countdown clocks, locked session windows
  or sudden "time's up" interruptions: forcing a child off an application with a
  hard gate invites friction.
- **Natural fatigue.** A standard "shake and play" board holds only a handful of
  viable early-reader words from the curated list. When the three to five are
  found the board is cleared; the child is gently celebrated and meets a calm,
  non-stimulating resting screen that encourages putting the device down.

**Conflicts with the earlier brief, and the planning assumption.** The earlier
brief asked for a parent-set time limit (section 7.4), for a button to reshuffle
and play again, and for high scores (section 3). The MVP removes the limit.
Assumed: no limit and no passive time-played figure in the MVP; one quiet way to
play again on the resting screen, never started automatically; high scores to be
worked out without a pull to continue (questions 62 and 63).

### 12.4 Play modes

Section 12.7, decided later, says which board these modes play on: Mode B below
describes the **phonics board**, which is a parent's switch, and the **standard
board** is the default.

**Mode A: Family Assistant and Scanner.**

1. The parent uses the iPad camera to snap the physical Boggle board.
2. Local OCR processes the grid text.
3. The engine cross-references the grid against the word list of the board mode
   in force (section 12.7): the full dictionary for the standard board, which is
   the default, and the curated early-reader index (section 12.1) for the
   phonics board. *Amended after 12.7; this step first said "only against the
   curated early-reader index".* A physical board cannot be seeded, so the
   phonics board's seeding does not apply to Mode A, and until a reviewed phase
   index exists the assistant plays the standard board's word list.
4. The screen highlights only the starting tile of a discovered word, leaving
   the blending work to the child.

**Mode B: Solo Play (virtual sandbox).**

1. The child shakes the tablet to roll a virtual 3x3 or 4x4 grid.
2. The engine uses a targeted seeding algorithm so that valid SATPIN
   combinations land adjacent to one another.
3. The child traces the adjacent tiles with a finger. For R, then AI, then N
   the vowel team lights as a single phonic unit, and the audio segments play:
   /r/ ... /eɪ/ ... /n/ ... RAIN!

**Planning assumptions, from reading the two modes against the rest.**

- Mode A is a single capture with a confirm-and-correct step (question 67),
  with one help level, the starting tile (question 68). It contains the
  camera-to-board pipeline, which is the riskiest part of the whole product and
  has not been started; the MVP includes it.
- Mode B's phonics board offers 3x3 and 4x4, where it was designed and measured;
  the standard board offers 3x3 to 6x6 (12.7).
- Mode A offers the same 3x3 to 6x6 on its check screen, so that a board typed in
  can be any size the game plays (12.7). A camera reads a physical board, which
  in practice is a 4x4, 5x5 or 6x6 set; the scanner's accuracy has been measured
  on those three sizes, on synthetic frames only. The MVP target is the 4x4.
- RAIN needs R, which is not a SATPIN letter, so Mode B's demonstration word is
  a Phase B word and cannot come from the Phase A index. Phase A would show the
  same effect with SATPIN-only words such as PAIN, SAINT or STAIN (question 60).
- A real word that is not among the board's viable words gets a gentle,
  neutral cue and no penalty (question 61).
- The shake in Mode B is the theatre of rolling the dice; "targeted seeding"
  means the board is not a plain random shuffle (question 24), and Mode A, which
  scans a real board, cannot be seeded at all.

### 12.5 What the MVP leaves out

As read; each is a deferral under the assumption of question 62.

| From the full brief | In the MVP |
| --- | --- |
| The "read aloud" voice recording (7.4) | Out: no voice capture |
| The printable PDF and e-mail to a teacher (7.4) | Out: no export |
| Time played and the parent-set limit (7.4) | Out: no timers or limits |
| Open dictionary solver for early levels | **In, as the standard board's word list** (12.7): the full dictionary, unmodified. Out: a block list and a child-appropriate cut of it. The phase index is the phonics board's |
| Boards of 5x5 and 6x6 (5) | **In, for the standard board** (12.7). The phonics board is designed at 3x3 and 4x4 |
| Tailorable help levels (4, 6) | One level: the starting tile |
| Camera on a stand, watching the lid come off (4) | A single capture (assumed) |
| Worlds, mascot, stickers by theme (7.2) | Not mentioned in the categorisation; to confirm whether any are in the MVP |
| School fonts (7.1) | Not mentioned; to confirm |

### 12.6 Review by a literacy specialist

Because the phonics detail is not the author's field, the following are
**needing expert review** by a classroom teacher or literacy specialist before
any child uses them, and before audio is recorded (question 69):

- the grapheme and phoneme segmentation of every word in the index;
- which graphemes and words belong to Phase A and which to Phase B, and what
  follows;
- the order and wording of the sound-it-out sequence and of any spoken praise;
- the sounds themselves, and the accent they are recorded in;
- the hand-filtered word lists, for suitability as well as for correctness;
- the wording of anything shown to a parent as a finding about a child.


### 12.7 Board modes

Decided by the author on 2026-10-16, after the categorisation above. It changes
12.4 and 12.5 where they say so.

There are two play modes (12.4: alone, or with other people) and, separately,
two **board modes**:

| | Standard board (the default) | Phonics board (a parent's switch) |
| --- | --- | --- |
| Letters | Dice, as in the real game | Seeded so that a board holds three to five words of the active phase (12.3) |
| Words | The full dictionary (ENABLE, unmodified) | The hand-filtered phase index (12.1) |
| Sounds | None | Grapheme blocks and sounding out (7.3) |
| Sizes in the MVP | 3x3, 4x4, 5x5 and 6x6 | 3x3 and 4x4 |

- The standard board is the game. The phonics board is for a child learning
  phonics, towards the end of Prep and into Year 1, and a parent turns it on.
- **The dice are the project's own**: made from a fixed Scrabble-style English
  tile-frequency table, with the table, method and seed recorded, and not copied
  from a product's distribution. The faces of a set the family owns are for
  Mode A, where a physical board is on the table: the app does not mix letters
  there, it reads the mixed board with OCR so that everyone plays on the real
  one.
- **No block list in the MVP.** A standard board accepts every dictionary word of
  three or more letters and shows and says the words a child finds, and the
  dictionary has entries no child should meet (12.1). The author has put block
  lists, and a child-appropriate cut of the dictionary, after the MVP. That is
  accepted for the MVP, a private build, and remains the gate to wider use
  (roadmap G2).
- The phonics board's targets are words of three letters at most (a digraph and
  a letter, as in a word such as `the`, is three letters), and letters the phase
  has not taught may appear on it where a taught letter would make too many words.
