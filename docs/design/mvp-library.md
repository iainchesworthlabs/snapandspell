# MVP library: what was built, and what the design left open

Written 2026-10-16, with the library groundwork for Mode B, and updated the same
day with the author's answers to its questions. It records what turned out wrong
or missing in the design ([Game](game.md), [Phonics](phonics.md),
[Board](board.md), [Performance](performance.md), [Path](path.md)) once it was
built, what the author decided, and what is still open.

## What was built

All of it is in `src/lib`, with Catch2 tests, no operating system code and no
phonics content. Details are in [Architecture](../architecture.md) and
[Testing](../testing.md).

| Piece | Where | Tests |
| --- | --- | --- |
| `validate_path`, `are_neighbours` | `path.hpp` | `path_test.cpp` |
| A reader and checker for the two phase file kinds, the review header, the cumulative `PhaseSet` | `phase_data.hpp` | `phase_data_test.cpp` |
| `tools/data/check_phonics.ps1`, its self-test, the `WJS_REQUIRE_REVIEWED_PHONICS` build gate | `tools/data`, `CMakeLists.txt` | `data.phonics`, `data.phonics.checker` |
| `seed_board`, `RecentTargets` | `seeding.hpp` | `seeding_test.cpp` |
| `GraphemeBinder` | `grapheme_binder.hpp` | `grapheme_binder_test.cpp` |
| `detect_shakes` | `shake.hpp` | `shake_test.cpp` |

No phase file was written. Every word, grapheme, sound and clip id in the tests is
written inside the test and says it is test data. The sounds are placeholders.

## Where the design turned out wrong, or did not say

**1. Seeding, steps 4 and 5, do not agree with each other or with the budget.**
Step 4 says to change one filler tile and verify "up to 20 times, then start
again from step 1"; step 5 says "after 20 starts"; the performance page budgets
"up to 20 verifications". Read as written that is up to 400. Built as **20
verifications in all**, as the budget says. The local move (change one filler
tile) is there as an option and **measured no better than planting again**, so
the default is to plant again every time.

**2. "Planting should raise the share well above blind draws" is true only where
the board is nearly empty.** The design measured blind draws at 25% to 30% a
board at their best dead count (3 on a 3x3, 8 to 9 on a 4x4) and said planting
had not been measured. Measured: at those counts a planted board lands in range
2% to 9% of verifications, below the blind share, because a planted board always
holds its targets and the letters round them add words. It lands 40% to 50% of
verifications only where the dead jewels leave about **four letters** (5 on a
3x3, 12 on a 4x4). Numbers are in [Testing](../testing.md#seeding-what-was-measured).

**3. "Three to five words, a full board of letters" is not reachable with an
index of 68 words or more.** A full 3x3 or 4x4 of the phase's letters holds 10
to 19 and 18 to 42 viable words of these indexes on average, and lands in range
6% of the time at best. A board lands reliably only when most of it is dead
jewels. The design recommends dead jewels "subject to the review" but does not
say that this is what the recommendation costs: a 4x4 that is mostly dead
jewels. This is a question for the reviewer and the author (below), and it
depends on the size of the index, which does not exist yet.

**4. The data format could not hold what the design says it holds. Fixed, 2026-10-16.**
(The two gaps below were closed by the author's answer to question 5; the other
points were decisions this page made and still stand.)

- *Split digraphs.* "The graphemes of a word, joined, spell the word" is false of
  a split digraph (`c a_e k` spells `ca_ek`), and "the arc over the K is drawn
  from the authored split" needs the split to be written somewhere. The reader
  refuses a grapheme that is not lowercase letters, by name, so a person does not
  find it read some other way. Proposal, not built: write the halves as `a_` and
  `_e` on the word line (`cake  c a_ k _e`); joined with the underscores removed
  they spell the word, and the grapheme `a_e` is defined in the graphemes file.
- *A grapheme with two sounds* (`ow`, `oo`, `c`, `g`, `a`). A grapheme file line
  is one sound, and a word line does not say which sound a grapheme has in that
  word. Sounding out needs it. Not built; a word line would need to carry the
  choice, for example `ow:2`.
- *Inflections.* The design says a `base` field is "a field on the word line if it
  is wanted". It is the reviewer's, and not built.
- *The marker for a tricky word* is not specified; built as `<word>  tricky`.
- *Focus sounds.* The phonics page says sounds and the dashboard page says "a
  subset of the phase's graphemes". Built as sounds (a sound string as it is
  written in the graphemes file), because two graphemes can share one; the
  application expands a ticked grapheme to its sound.
- *Phase order* was not defined. Built as ordered by name, byte order, so a phase
  is a single letter or a number with the same digit count.
- *A word of fewer than three letters* has no tier and can never be counted, so
  a words file may not hold one. An addition to the design's list of checks.
- *The reviewer's date.* "reviewed-on" is held to `YYYY-MM-DD`, as the
  repository's other dates are, so a typo fails the gate rather than passing it.

**5. The "reviewed" gate needs PowerShell and a release configuration to be seen.
Settled, 2026-10-16: the `Performance (Linux, GCC)` leg builds `linux-gcc-release`,
so CI runs it (question 6).**
The build gate is a build step that runs the checker, and the release presets
turn it on. CI builds Debug only (the Windows, Linux and macOS legs), so CI does
not run the gate itself; `data.phonics` runs the checker in every leg, without
`-RequireReviewed` there. I exercised the gate locally with a temporary file and
removed it (an unreviewed file stopped the build; a reviewed one passed).

**6. The performance budget for seeding is a desktop figure.** "10 ms typical, 50
ms worst" holds on this desktop (a mean under 7 ms; worst single seed 25 ms), and
the worst case is over the budget on a tablet at three to five times a desktop
core, with an index of a couple of hundred words on a 4x4. A verification costs
0.02 to 0.5 ms, as estimated. The work counter and the Tracy zones of the
performance page are not built; `SeededBoard::tries` and `fallback` are what a
test can count.

**7. The shake page's numbers are all presentation.** The detector reports when
a shake started and ended and whether it may be going on. The 0.7 s minimum and 3
s maximum are for the scene, which composes them. The accelerometer's unit
(`QAccelerometer` is understood to report metres per second squared with gravity
in it) was read, not checked, which is why the unit is an option.

**8. `validate_path` is written.** `game.md` and the roadmap said "not yet
written"; both now say it is.

## Decided by the author, 2026-10-16

The six questions this page first asked, and what was done.

| # | Question | Answer | Done |
| --- | --- | --- | --- |
| 1 | Roadmap M1: mark `validate_path` and shake detection done? | Yes | The roadmap's state table, requirement table and M1 list say so, with "on synthetic data" |
| 2 | Roadmap M2b: mark the phonics machinery done "on test data"? | Yes | The roadmap says so, with the content still blocked on G7 |
| 3 | Is a board that is mostly dead jewels acceptable? | Not decided: try the solver-guided fill first, to understand it | Built and measured; see below |
| 4 | Is the review header the right attestation, and is `source` required? | `reviewed-by` is fine; `source` is required | A file that claims a review needs `reviewed-by`, `reviewed-on` and `source`; test data needs none |
| 5 | Split digraphs and graphemes with more than one sound, before the first phase file? | Yes | `cake  c a_ k _e` and `snow  s n ow:2`; reader, checker, tests and [Phonics](phonics.md#the-data) |
| 6 | Should CI build a release preset, so the review gate runs there? | Yes | Already so: the `Performance (Linux, GCC)` leg, added with the profiling tier, configures, builds and tests `linux-gcc-release`, which sets the switch and runs `data.phonics` with `-RequireReviewed`. Not a required check |
| 7 | Is the MVP's Mode B the phonics board alone, or the standard dice board by default with phonics as a parent's switch? | **The standard board is the default; phonics is the parent's switch.** Section 12.4 of the brief reads as the former; the author chose the latter | Recorded in the design pages (index, game, phonics, dashboard, performance). The brief and the roadmap's MVP paragraph still say the former and are not edited |
| 8 | May a phonics phase be planted only with 3-letter words? | Yes. They are letters, not sounds, so a word such as `the` or `cow` (a digraph and a letter) is one | `max_target_letters = 3` is in `phonics_board_options` |
| 9 | May a phonics board have letters outside the phase on it? | Yes | The guided fill prefers the phase's letters and uses others only where a letter of the phase would make too many words; no dead jewels |
| 10 | Should the performance pages name the board mode each budget is for? | Yes | [Performance](performance.md) has a table of what each board mode solves against and loads, and labels the seeding rows |

## The guided fill, tried (question 3)

Built as two options, both off by default: `guided_fill` and
`max_target_letters`. The measurements are in
[Testing](../testing.md#fuller-boards-a-guided-fill-and-short-targets). The
short version:

- **The guided fill alone does nothing.** The planted words already hold too
  many words before any filler is placed; a fill cannot take a word away.
- **Short targets, kept apart, are what work.** Targets of three letters at most
  land 90% or more of seeds on a 4x4 and larger, with about a quarter of the
  tiles dead jewels, against four letters on the board for the random fill.
  With letters outside the phase in the filler, none are dead at all.
- **The price is the targets.** Only 3-letter words (Spark words) are planted;
  longer words still count if found. The author accepted it (question 8); the
  literacy reviewer may still revisit it.

## Board modes, decided

There are two **game modes** (playing alone; playing with other people, which is
Mode A, the scanner) and, separately, two **board modes** (the author, 2026-10-16):

- **Standard**: letters on cubes, 3x3, 4x4, 5x5 or 6x6, as in the real game. The
  **default**.
- **Phonics**: the seeded, grapheme-bound board that a parent switches on to help
  a child who is learning phonics join sounds into words. The audience is a
  child towards the end of Prep and moving to Year 1, who knows most of the
  sounds already.

What follows from it, and what the library already does:

- **Seeding and dead jewels belong to the phonics board only.** A standard board
  is a roll of dice, has no dead jewels and no target of three to five words. The
  seeding library is not on its path.
- **`phonics_board_options(index, size)`** is the phonics board as decided:
  targets of three letters at most, the guided fill, every letter but one as
  filler (the first that no word of the phase uses, which is the dead letter), so
  that untaught letters appear only where a letter of the phase would make too
  many words. Measured on the proxies in
  [Testing](../testing.md#fuller-boards-a-guided-fill-and-short-targets): the
  phase's letters are 72% to 82% of the tiles, no tile is dead on a board that
  lands, and 90% or more of seeds land on a 4x4 and larger.
- **Standard mode loads the full word list**, so the dictionary load (55 to 65 ms
  and 18 MB measured) is on its path to the first board, which the performance
  design had said it was not. [Performance](performance.md) now says which mode
  each budget is for.
- **Brief 12.4 and 12.5** still describe Mode B as the seeded phonics board and
  list "open dictionary solver for early levels" as out. The author's choice
  makes a standard board the MVP's default, so both lines are out of date. They
  have not been edited, as the brief is the author's text.

## What the standard board needs, and is not built

The library has the solver, the path check, the shake detector and the scanner's
seam (`Board::parse`). A standard board needs three things it does not have, and
all three are now MVP scope, where the design put them after it:

1. **A dice model.** Boards of 3x3 to 6x6 from dice, with a `Qu` face. The design
   recommended dice made by this project from the dictionary's letter counts
   (`data/dice/<n>x<n>.txt`, its source and seed in the header), because the
   faces of a printed set are a legal question and a list found online has no
   origin. That recommendation has not been put to the author.
2. **The standard board's word list.** The brief's 12.1 and 12.5 have the
   child's words as a hand-filtered phase index. A standard board has no phase.
   The candidates are the child-facing lists in `data/lexicon` (a core tier and
   ten categories, 689 words, unreviewed) and the full dictionary behind a block
   list. This decides the load time and memory (see the performance note), what a
   child is told about a word they trace, and how many words a 4x4 holds that
   count (measured: 31 on a 3x3 and 120 on a 4x4 of any dictionary word).
3. **The size range.** The brief gives the MVP 3x3 and 4x4 and the rest after it.
   The author's account is 3x3 to 6x6. Whether the MVP's standard board offers
   all four sizes is not said.

## Questions for the author (open)

| # | Question | Default |
| --- | --- | --- |
| 11 | **Brief 12.4 and 12.5 and the roadmap's MVP paragraph.** May they be amended to say the MVP's board is the standard board by default, with the phonics board as the parent's switch, and the open-dictionary line moved out of "left out"? | Not edited |
| 12 | **The standard board's word list.** The child-facing lexicon, or the full dictionary behind a block list? Neither has been reviewed for a 5-year-old | Not chosen; nothing reads either |
| 13 | **Dice.** May the project make its own dice from the dictionary's letter counts, as the design recommends, rather than use the faces of a set the author owns? | Not built |
| 14 | **Sizes.** Does the MVP's standard board offer 3x3 to 6x6, or 3x3 and 4x4 as the brief says? | The library takes any size to 16 |

## What was measured

All on one Windows 11 desktop, MSVC 19.51, Release. Counts and rates only; no
word list is reproduced. The seeding measurement used throw-away programs that
are not committed, on proxy indexes made from the dictionary (the words of three
or more letters spelled with S, A, T, P, I and N: 249, those not ending in S:
122, and those of three or four letters not ending in S: 68). The tables are in
[Testing](../testing.md#seeding-what-was-measured). The shake detector was
exercised on synthetic traces only; nothing was recorded from a device.
