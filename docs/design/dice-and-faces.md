# Dice and their faces

A die has six faces with six different letters; a shake mixes the dice and
leaves one face up. The board scene in the prototype and the roll in the library
both lose that. This page says what a die is in this application, where the
letter frequencies are encoded, what the scene has to be given, and what the
phonics board does about dice. Proposals unless marked **Fact**; the open
choices are put to the author at the end.

It was prompted by the author's observation, on seeing the 3D shake prototype,
that every die shows the same letter on all six sides, that in the real game each
side is a different letter, and that the frequency of letters across the dice
has to be designed (more E than Q) for the standard board and for the phonics
board.

## What the code does today

| | **Fact**, checked in the code |
| --- | --- |
| The library's dice | `DiceSet` holds dice of up to 12 faces each (`wjs/dice.hpp`). `roll(dice, seed)` puts the dice in a random order, a die to a tile, and each shows one face: a pure function of the set and the seed. The project's sets have six different letters to a die and a vowel on each |
| What `roll` returns | A `Board`. Which die is on which tile, and which of its faces is up, are not returned |
| The scene | `BoardScene.qml` takes a finished board (one string a tile). `Die.qml` is one cube that "carries its tile's face on every side", and `FaceAtlas.qml` is one cell a tile. So the six faces of every die show the same letter, and nothing in the scene knows a die has other faces |
| The frequency | `tools/data/make_dice.ps1` deals faces in proportion to a fixed Scrabble-style English tile-count table (A9 B2 C2 D4 E12 ... Z1, the 100-tile counts with blanks not counted): six different letters to a die, one to three vowels and at most one of J K Q V X Z on each, 30% to 40% vowels and at most 8% S in a set, Q as `QU`. The sets are data (`data/dice/<n>x<n>.txt`) and the `data.dice` test re-makes them and compares |

The first and the last rows are as the author expected. The two in the middle
are a gap in the design: `board.md` gave the scene "a finished board and a
seed", which is not enough to draw a die that has other faces.

## What a die is here

Same as the real game, and the application should show it:

1. **A die is six different letters.** The set is fixed: the same 16 dice are
   shaken each game, as in the box.
2. **A shake does three things**: it mixes which die is on which tile, which
   face is up on each, and how each die lies. The library decides the first two
   (the board), and the third is cosmetic.
3. **The scene shows real faces.** Each cube of the scene carries its die's six
   letters. While it tumbles the other letters are seen, and it lands with the
   rolled face up. Nothing decides anything but the library's roll: the faces
   that are not up are never read by the game.
4. **At rest every letter is upright** (as on the replica in the scanner, which
   shows the top faces only: there the real dice are the dice).

### What the scene is given, and what the library returns

```text
roll(dice, seed) -> RolledBoard
    board:      the Board, as today
    placements: for each tile (row-major): die index, face index (the one up)

scene.play(board, faces, up, seed)
    faces[tile]: the six face strings of the die on that tile ("A", "QU", ...)
    up[tile]:    which of them is up
```

`board` is still the finished board and `faces[tile][up[tile]]` equals the
tile's letters; a test holds that. The cosmetic orientation is derived from the
seed in the scene and is not part of the roll. Dead jewels (phonics board) are a
die whose faces are all the jewel. This is a **proposal for an interface
change**, not a finding about the prototype's code beyond what is in the table.

## Where the frequency is encoded

### What was in the shipped sets before 2026-10-19

**Historical, kept as the reason for the change. Fact, measured from
`data/dice/*.txt` on the branch of pull request 10, with letters counted over
the whole dictionary as the tool then counted them.** The share the dictionary's
counts would give a set is what largest-remainder dealing aimed at. The shipped
sets now come from the Scrabble-style table above.

| Set | Dice | Faces | Distinct letters | E faces (share would give) | S faces (would give) | Qu | J, X, Z |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 3x3 | 9 | 54 | 20 | 4 (6.2) | 6 (5.1) | 0 | 1, 0, 1 |
| 4x4 | 16 | 96 | 20 | 7 (11.1) | 11 (9.1) | 0 | 0, 0, 0 |
| 5x5 | 25 | 150 | 22 | 16 (17.3) | 10 (14.3) | 0 | 0, 1, 0 |
| 6x6 | 36 | 216 | 23 | 23 (24.9) | 26 (20.6) | 0 | 0, 1, 1 |

The dictionary's shares: E 11.5%, S 9.5%, I 9.0%, A 7.6%, R 7.1%, N 6.8%; Q is
0.16% and Z 0.47%.

What this shows:

- **E is more common than Q**, as the author expects: seven faces to none on the
  4x4.
- **There is no Qu face in any set.** Q's share of 96 faces is under a sixth of a
  face, so largest-remainder dealing gives it none. The real game has one, and a
  `Qu` tile is what the library and the scanner already handle.
- **E is under-dealt on the small sets and S over-dealt** (4x4: E 7 against 11,
  S 11 against 9). The rule "six different letters to a die" with "a vowel on
  each" moves counts: a letter can be on at most one face of a die, so on 11 of
  the 16 dice of the 4x4 there is an S. Plurals are easy words, and a board full
  of S may not be what is wanted.
- **20 to 23 different letters** of 26. W is missing from the three smaller sets.

None of this is wrong for a first set. It is what the rule produced, and the
rule was not written down anywhere the author could see. The sets were replaced
on 2026-10-19 by dice dealt from the Scrabble-style table: every shipped size
now has QU tiles, 30% to 40% of its faces are vowels, S is at most 8%, and no die
holds more than one of J K Q V X Z.

### Proposal: the target is data, and the dice are checked against it

1. **A weights file**, `data/dice/weights.txt`: for each letter its weight and
   where the weight came from. The weights are now the fixed table in the tool
   and in each file's header; there is no separate file.
2. **Overrides** beside it, each with a reason: at least one `Qu` face on the
   4x4 and larger, none on the 3x3 (the first board; see
   [Board](board.md#sizes)); a letter's most faces per set, if S is to be capped.
3. **The tool deals from the file**, and a check (extending `data.dice`) holds each
   set's face count for each letter to within one face of the weights, except
   where an override says otherwise, and reports the letters that are absent.
4. **The distribution is shown**: the check prints the table above for the
   shipped sets, so a change of a set is a visible change.

### Whose frequency

The dictionary's letter counts are **type frequency** (how many words a letter
is in). The real game's dice follow something nearer how often letters turn up
in English text. The shipped sets use the published Scrabble tile counts, a
frequency table with a stated source in `data/README.md`, and the table is in the
tool and in each file's header. Choices that were weighed for the standard board:

| Weights from | Gives | Source and terms |
| --- | --- | --- |
| The Scrabble-style tile counts (used from 2026-10-19) | Boards of letters that make English words, with few S and rare letters | Published English tile counts; in `data/README.md` |
| The dictionary's counts | Boards of letters that make many dictionary words | ENABLE, public domain as published; already in the repository |
| The child-facing lists (`data/lexicon`, 689 words) | Boards that make words a child knows | Written for the project |
| A text-frequency table | Dice nearer the real game | Not in the repository; needs a source |
| A mix: dictionary counts with a floor (a `Qu`, one of each rare letter on the larger sets) | The real game's range of letters | As the dictionary's counts |

Decided 2026-10-19: the Scrabble-style tile counts, with the quality rules in
the tool (vowels, rare letters, S) rather than a per-set letter table.

## The phonics board

The phonics board is not a roll of fixed dice: it is planted and filled so that
it holds three to five words of the phase, and the dice of the standard board
are not used (see [Phonics](phonics.md) and [MVP library](mvp-library.md)). The
question is what the scene shows for it.

| | A. Phase dice, rolled | B. A planted board, dressed with dice (recommended) |
| --- | --- | --- |
| How the board is made | Fixed dice of the phase's letters (and some jewel faces), rolled, then verified with the solver: rejected and rolled again unless it holds three to five words | As now: targets planted, the guided fill, verified. Each tile is then given a die whose up face is the tile and whose other five faces are drawn from the phase's letters in proportion to the phase's own counts |
| What the child sees | A real shake of a fixed set of dice | The same shake; the dice's other faces are decoys |
| Yield | **Measured, blind draws:** about one board in four at best (25% to 30%), so three or four rolls; each a fraction of a millisecond | **Measured by the library session:** 90% or more of seeds on a 4x4 and larger with targets of three letters |
| Focus sounds, variety | Weak: the dice decide | Strong: targets are chosen from the focus sounds and the recent boards |
| Honesty | Every face is a real face of a real die | The decoy faces are made up for the picture and decide nothing |
| Faces are the same each shake | Yes | No: drawn from the seed |
| Needs | A phase-dice tool and files | The decoy draw, from data the phase already has |

Both put only the phase's letters (and the filler letters the author allowed)
on the faces, so the tumble shows nothing the phase has not taught except what
the guided fill puts on the board anyway. B keeps what has been built and
tested and gives the seeding's control; A is closer to the real game and could
be a later option. The decoy faces are the one place the application draws
letters for show, and the design says so.

## Scanner (Mode A)

The replica shows the top faces of the real dice, upright. It does not tumble.
There are no decoy faces: the real board is on the table and the application
reads it.

## Work this page implies

| Piece | Where | Owner |
| --- | --- | --- |
| This page; the revised scene interface in `board.md` and the answers on dice | `docs/design` | Design |
| `roll` returns the placements; a test that `faces[tile][up] == board tile` | Library | The session that built the dice |
| `weights.txt`, overrides, the dealing from them, the extended check, the printed table | `tools/data`, `data/dice` | The same |
| The scene takes faces and the up index; the atlas is one cell for each face of each die | The prototype under `src/app/prototype/shake` | The prototype session |
| The decoy draw for the phonics board, if B | Library, with the seeding | The library session, after the author's answer |
| A look at the real dice's letters against the sets | The author, with the box | The author |

## Questions for the author

1. **Qu.** Is a `Qu` face wanted on the 4x4 and larger, as in the real game, and
   none on the 3x3? (Default: yes, and the 3x3 has no Q.)
2. **Whose frequency** for the standard board: the dictionary's counts with the
   overrides (default), the child-facing lists' counts, or a text-frequency table
   if a source is found?
3. **Phonics dice**: B (planted, dressed with decoy faces: default) or A (phase
   dice rolled and verified)?
4. **S.** Should a set cap how many dice carry an S (11 of 16 today on the 4x4)?
5. **The same dice every shake**, as in the box, for the standard board?
   (Default: yes; the set is the game's.)
