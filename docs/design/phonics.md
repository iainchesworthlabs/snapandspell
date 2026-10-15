# Phonics: the mechanism

How a phonics phase becomes a board, how a trace becomes a grapheme block, and
how a found word is "sounded out". Written from
[product requirements](../product-requirements.md) sections 7.3, 12.1 and 12.4.

**What this page is not.** The author has said phonics is not their field and
the plan assumes errors (section 12). Gate G7 in the [roadmap](../roadmap.md)
puts a literacy review before any child uses the content and before any audio
is recorded. So this page designs the **mechanism** and prints **no word
list**: a phase's graphemes, words and wording are *data*, written and reviewed
by someone qualified, and the code does not know them. Where an example word is
used it is one the brief itself gives, to show a format, and it is not a
statement that the word belongs to a phase. Programme names are not used; the
phases are Phonics Phase A and Phonics Phase B (section 12.1).

## What is data and what is code

| | Is | Where it lives | Who owns it |
| --- | --- | --- | --- |
| A phase: its graphemes, and what sound each makes | Data | `data/phonics/phase-<x>.graphemes.txt` | The reviewer |
| A phase's words, each split into graphemes | Data | `data/phonics/phase-<x>.words.txt` | The reviewer |
| The sound clips and their wording | Assets | The application's resources | The reviewer, then whoever records |
| Reading those files, checking them, finding the words on a board | Code | The library | Engineering |
| Seeding a board, binding a trace, timing the sequence | Code | The library and the application | Engineering |

Nothing in the code names a letter, a sound or a word of a phase.

## The data

One line a word, graphemes separated by spaces. A format example, with a word
from the brief:

```text
# phase: A
# reviewed-by: <name>   reviewed-on: <date>   source: <where the list came from>
pain  p ai n
```

and one line a grapheme, with its sound and its clip:

```text
ai  /eɪ/  clip:ai
```

A checker, `tools/data/check_phonics.ps1`, in the style of
`check_wordlists.ps1`, holds each file to:

- every word is in `data/dictionaries/en.txt` (the lists are subsets of what
  the solver accepts, as the lexicon is);
- the graphemes of a word, joined, spell the word;
- every grapheme of a word is in the phase's grapheme file, or in an earlier
  phase's (phases are cumulative: a word is in a phase if all its graphemes
  are);
- no duplicate words; sorted, as the other lists are;
- a file with no `reviewed-by` and `reviewed-on` is a **test file**: the
  release configurations refuse to build with one in the data. That is the
  review gate as a build rule, not a hope.

Tests use phase files of a dozen words written in the test itself, as the
repository's rule for word lists says, and they are marked as test data.

**Tier is derived, not stored.** A word's tier is its letter count, `Qu`
counting two: 3 letters Spark, 4 Power, 5 or more Mega (section 12.1 settles
RAIN and STAR as Power). So `QUIT` is four letters and a Power Word. Whether an
inflection is a new word with its own sticker is the reviewer's, and is a field
on the word line if it is wanted (`base` = the word it is an inflection of).

**Tricky words** (ones that do not decode) carry no grapheme split and are
marked `tricky`; they are spoken whole and are never "sounded out".

## Measured, from the repository's own data

**Fact, counts only.** The dictionary's words of three or more letters spelled
only with S, A, T, P, I and N number 249 (43 of three letters, 70 of four, 54
of five, 36 of six, 28 of seven, 13 of eight, 4 of nine, 1 of ten); 127 end in
S. This reproduces the brief's count. The brief says that before stripping this
set holds unsuitable entries; no list of it is reproduced here.

## Seeding

Mode B's board is **seeded**: made so that it holds three to five viable words
(section 12.3) and so is quickly cleared. "Viable" means a word of the active
phase's index that is on the board.

### The method

1. **Choose targets.** From the phase index, pick two or three words. Prefer
   those that contain a focus sound the parent ticked (see
   [Dashboard](dashboard.md#focus-sounds)), and avoid the words of the last few
   boards (a short ring buffer of the last ten boards' targets).
2. **Plant.** Place each target as a path of adjacent cells, allowing two words
   to share a tile where the letter agrees.
3. **Fill** the other cells (options below).
4. **Verify.** Solve the board against the phase index. Accept if every target
   is found and the number of viable words is from 3 to 5. Otherwise change one
   filler cell and verify again, up to 20 times, then start again from step 1.
5. **Fall back.** After 20 starts keep the board whose count was nearest the
   range, and record that it was a fallback so a test can count them.

The roll is a pure function of the phase data, the board size, the focus
sounds, the ring buffer and a seed. The library can therefore replay and test
it, with the same seed giving the same board on every compiler (the generator
is `std::mt19937` taken modulo, as Cubeulator's tests do, because
`std::uniform_int_distribution` is not the same algorithm in each standard
library).

### Why verification is the method

**Fact, measured.** A board of random letters from the six SATPIN letters holds
on average 27.6 of the 249 words on a 3x3 and 67.5 on a 4x4, and only 2% of 3x3
boards hold three to five. Putting dead jewels in some cells brings the counts
down; the share of random boards that land in three to five, by the number of
dead cells, for three illustrative index sizes:

| Index | Size | Dead cells that give the best share | Share in 3 to 5 |
| --- | --- | --- | --- |
| All 249 | 3x3 | 3 or 4 | 25% to 27% |
| | 4x4 | 9 | 27% |
| 122 (no word ending in S) | 3x3 | 3 | 26% |
| | 4x4 | 8 | 29% |
| 68 (3 to 4 letters, no S ending) | 3x3 | 3 | 27% |
| | 4x4 | 8 | 30% |

These are blind draws, with nothing planted, from sets that are *illustrations*
of what a stripped index might be and not proposals for one. They show two
things. The range is reachable without trying hard (about one board in four,
so four or five solves a board at about 0.03 to 0.5 ms each). And it is not
reached by chance at the ends: a 4x4 with no dead cells never lands in it.
Planting should raise the share well above blind draws; that has **not**
been measured.

### What fills the other cells

| Filler | Looks like | Consequences |
| --- | --- | --- |
| **Dead jewels** (recommended for Phase A) | A jewel with no letter | The child sees only letters the phase has taught. No other real word can be spelled, so a "real word, not today's" cue arises only from words of the phase's letters that the index excludes. Differs from a real Boggle board |
| Letters not yet taught | A normal board | Realistic; untaught letters appear; real words using them (a three-letter word with a letter outside the phase) are traced often and get no star, which is the frustration question 61 asks about |
| Letters of the phase | A normal board of the phase's letters | More incidental viable words, so a count of 3 to 5 is harder to hit and boards cluster |

The first is recommended for Phase A **subject to the literacy review**; the
mechanism takes the filler as a parameter, so the answer can change without
touching the algorithm. A dead jewel is drawn as a jewel and is not a
touch target for a path.

### Steering, and what limits it

The brief's "dynamic vocabulary scaling" (7.2) steers the letters: simple
consonant-vowel-consonant shapes early, R, S, T, A, I and N clustering later.
Proposed limits:

- Steering chooses **targets and fillers**; it never changes the dictionary, so
  every word of the phase is still found and counted wherever it is.
- It never moves a child between phases. The parent picks the phase (see
  [Game design](game.md#progress-without-a-pull)); steering works inside it.
- A board is always verified, so a steered board is as fair as an unsteered
  one: it holds 3 to 5 viable words and every one can be traced.
- Variety is measured: a test reports, over 1,000 seeds, the distinct
  target sets and the words each phase never produced.
- **The scanner cannot be steered.** It reads a real board. The phase there is
  only a *filter*: which words of the board are highlighted. See
  [Game design](game.md#mode-a-the-family-assistant-and-scanner).

### Few words, or the same five

If the index has only the five early-reader words (NIP, PAT, SAT, SIT, TAP),
every board offers some of the same five, in different places. The mechanism
works; the child sees the same words. Section 12.1 treats the five as the
subset to start from, not the index.

If too few words contain a chosen focus sound, targets fall back to words of
the phase in general, and the dashboard says "few words use this sound yet", not
that nothing was played (question 45).

## Binding a trace into grapheme blocks

### In flight

As each tile joins the path, the letters traced are matched, greedily and
longest first, against the **multi-letter graphemes of the active phase**. Where
two or three consecutive tiles spell one, they are drawn as one block: a shared
bar beneath and a common edge colour (shape first). Dice carry single letters
(plus `Qu`); no die has a digraph on it, because that would make the words that
need those letters impossible.

| Case | What happens |
| --- | --- |
| R then A then I | A and I link as a block when I joins; R stays single |
| The same letters in the other order (I then A) | No grapheme, so no block; the word, if it is one, is read as separate sounds |
| A grapheme not in the active phase | Not linked; the phase decides |
| A split digraph (C, A, K, E) | Cannot be known until the E; the arc over the K is drawn **when the word is complete**, from the authored split |

### Audio while tracing

Each tile plays its letter's **sound**, not its name, as it joins (a clip of
300 ms or less, cut off by the next). A block's own sound is **not** played
while tracing: the link is visual, and the vowel team's sound is heard in the
sequence below. The alternative, delaying each tile's sound by one tile so a
block can play its own, makes every trace feel late, and is not recommended.

## "Sound it out"

When a viable word is complete, the celebration decodes it. The segmentation is
the word's authored split, which is authoritative: if it differs from the
in-flight blocks, the blocks are redrawn to match.

| Step | What the child sees and hears | Timing |
| --- | --- | --- |
| 1. Segment | Each block lights in turn; its sound plays | Each begins when its clip starts; 150 ms between |
| 2. Blend | The blocks slide together; the sounds again, quicker | 380 ms, the `turn` token |
| 3. Say it | The whole word is read | A pre-rendered clip, or the platform's own speech |
| 4. Cheer | A short phrase, naming the sound where it helps | A pre-rendered clip |

A three-block word takes about three seconds.

**How the animation and the sound are kept in step.** The highlight is driven by
the clip's *start event*, not by a clock: the block lights when its sound
begins, so there is nothing to drift. Clips are short and play one after
another; the sequence is a list in the data, not a timeline in the code.

**Where the sounds come from.** Isolated phonemes are not something the
platform's speech does well (the brief says so), so they are pre-rendered
assets, recorded by a person after the review. Whole words and sentences may
use the platform's own speech or recordings. Nothing is captured from the child
(section 12.2).

**The settings a parent has** (see [Dashboard](dashboard.md#settings)): Sound it
out *full*, *the word only* or *off*; and the volume, which is the device's.

## Phases, in the data

A phase is a set of graphemes. Phase A is the six SATPIN letters (section
12.1); Phase B is "consolidated blends". The brief's own fuller sequence has
three steps: single-letter phonemes, then double consonants and simple
digraphs, then vowel teams. Which grapheme sits in which phase, including where
AI and AY go, is the reviewer's decision (question 33) and changes only the
data files.

The sound sticker book (question 36) is **one album**. A sticker belongs to a
word. Its pages are views: *by word* (the vault) and *by sound*, where a word
appears on the page of each grapheme it contains. A word with two graphemes of
interest therefore sits on two pages without being stored twice.

## What must happen before a child uses this

| Item | Blocked until reviewed | Reviewer's decision |
| --- | --- | --- |
| The phase files | The release build (the rule above) | Which graphemes, which words, which are tricky |
| The phoneme clips | Recording | The sounds, the accent, the set |
| The sound-it-out wording and praise | Recording | Order and words |
| The block-linking rules | Merge, if the active phase has multi-letter graphemes | Whether the link is right |
| Anything shown to a parent as a finding | The dashboard | The wording |

What can be built and tested **before** the review: the checker, the data
reader, the seeding with test data, the binding, the sequence timeline with
placeholder clips, and the build gate.

## Unknowns

- The yield of planting over blind draws.
- Whether a board of dead jewels is acceptable (above).
- Whether linking blocks in flight helps a child or distracts. A test with
  children settles it, and the setting to turn it off should exist.
- The accent: the brief asks about Australian English. Phoneme labels in this
  repository's documents are the brief's own (/eɪ/).
