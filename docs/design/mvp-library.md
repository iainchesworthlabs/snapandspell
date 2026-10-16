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
  longer words still count if found. Whether that is acceptable for a phase is
  the reviewer's, and it is a question below.

## Board modes: what this page assumed, and what the author said

This page, and the MVP brief it reads (12.4), treat Mode B as the phonics board:
seeded so that it holds three to five words of a phase's index. The author's
account of the game is wider (2026-10-16): there are two **game modes** (playing
alone; playing with other people, which is Mode A) and, separately, two **board
modes**:

- **Standard**: letters on cubes, 3x3, 4x4, 5x5 or 6x6, as in the real game. This
  is the **default**.
- **Phonics**: the seeded, grapheme-bound board, which a parent switches on to
  help a child learning phonics join sounds into words. An alternate mode, "a
  nice idea", whose place the author is unsure of.

What this changes, as far as it can be seen from here:

- **Seeding and dead jewels belong to the phonics board mode only.** A standard
  board has no dead jewels and no target of three to five words; it is dice, and
  a board holds dozens of words. The seeding library is not on its path.
- **Standard mode loads the full word list.** The performance page's premise
  that "loading the dictionary is not even on the critical path" holds only for
  the phonics board. In the default mode the dictionary load (55 to 65 ms, 18 MB
  measured) and the solve of a 5x5 or 6x6 board (up to 0.85 ms measured) are the
  work. The performance budgets, and the profiling tier's cases, were written for
  the other premise and should say which mode each is for.
- **The MVP brief (12.4) gives Mode B the seeded board and the phase index.**
  That is the phonics board mode. Whether the MVP ships the standard board as
  well, or the phonics board alone, is not said by the brief; the dice are listed
  after the MVP in [Board](board.md).
- **Sizes.** The phonics board was designed and measured at 3x3 and 4x4. The
  guided fill lands 5x5 and 6x6 easily, so a phonics board is not limited to the
  MVP's sizes by the library.

## Questions for the author (open)

| # | Question | Default |
| --- | --- | --- |
| 7 | **Board modes.** Is the MVP's Mode B the phonics board alone, or the standard dice board as the default with phonics as the parent's switch? The brief (12.4) reads as the former; the answer decides what the first iPad build plays and where dictionary loading sits | The library supports both; nothing in the application exists |
| 8 | **Targets of three letters.** May a phonics phase be planted only with 3-letter words (Spark words), the longer ones still counting if found? It is what makes boards of mostly letters land | Off; `max_target_letters` is 0 |
| 9 | **Letters outside the phase on a phonics board.** The guided fill leaves no dead jewel if untaught letters may appear. Is that acceptable to the reviewer, against a quarter of the tiles being dead jewels? | Off; the filler is the phase's alphabet |
| 10 | **Performance premise.** Should the performance page and the profiling tier name the board mode each budget is for, with the standard board's dictionary load on the critical path? | Not changed |

## What was measured

All on one Windows 11 desktop, MSVC 19.51, Release. Counts and rates only; no
word list is reproduced. The seeding measurement used throw-away programs that
are not committed, on proxy indexes made from the dictionary (the words of three
or more letters spelled with S, A, T, P, I and N: 249, those not ending in S:
122, and those of three or four letters not ending in S: 68). The tables are in
[Testing](../testing.md#seeding-what-was-measured). The shake detector was
exercised on synthetic traces only; nothing was recorded from a device.
