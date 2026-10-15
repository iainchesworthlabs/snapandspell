# MVP library: what was built, and what the design left open

Written 2026-10-16, with the library groundwork for Mode B. The design set
itself (`docs/design/` on `feature/game-design`: game, phonics, board,
performance, path) is not on this branch, so it is named here, not linked. This
page is a note for the author. It records what turned out wrong or missing in
the design once it was built, and the changes it would like made to the brief
and the roadmap, **as questions**: neither `product-requirements.md` nor
`roadmap.md` was edited.

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

**4. The data format cannot hold what the design says it holds.**

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

**5. The "reviewed" gate needs PowerShell and a release configuration to be seen.**
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

**8. `validate_path` is written.** `game.md` and the roadmap say "not yet
written".

## Questions for the author

These are the changes to the brief and the roadmap that the work suggests. Each
has the default this branch uses so that nothing waits.

| # | Question | Default |
| --- | --- | --- |
| 1 | **Roadmap, M1:** may `validate_path` and the shake detection (both library parts) be marked done, leaving the touch handler and the sensor? | Left as written; not edited |
| 2 | **Roadmap, M2b:** may the phase reader, the checker, the review gate, the seeding and the binder be marked done "on test data", with the content still blocked on G7? | Left as written |
| 3 | **Brief, 12.3 and 12.6:** is a board that is mostly dead jewels (four letters on a 3x3 or 4x4) acceptable, or should the index be smaller, or should the fill be guided by the solver (not built: each filler letter chosen so that it adds no word, which might give fuller boards at the same share)? | Dead jewels are a parameter; the board is whatever the caller asks for |
| 4 | **Brief, 12.6:** does the header (`reviewed-by`, `reviewed-on` as a date, and `source`) say what the reviewer is to attest? Should `source` be required, as `data/README.md` asks of every list? | Reviewer and date are required; `source` is not |
| 5 | **Phonics data:** may the format grow split digraphs and a grapheme with more than one sound (item 4 above) before the first phase file is written? Changing it afterwards means rewriting reviewed files | Not built |
| 6 | **CI:** should one leg build a release preset, so that the review gate runs in CI and not only on a person's machine? | Not changed; workflows were not touched |

## What was measured

All on one Windows 11 desktop, MSVC 19.51, Release. Counts and rates only; no
word list is reproduced. The seeding measurement used throw-away programs that
are not committed, on proxy indexes made from the dictionary (the words of three
or more letters spelled with S, A, T, P, I and N: 249, those not ending in S:
122, and those of three or four letters not ending in S: 68). The tables are in
[Testing](../testing.md#seeding-what-was-measured). The shake detector was
exercised on synthetic traces only; nothing was recorded from a device.
