# Game design

How Solo Play (Mode B) and the Family Assistant and Scanner (Mode A) behave:
what the child does, what the app answers, what it reveals and what it keeps.
Written from [product requirements](../product-requirements.md) sections 3, 4,
6, 7 and 12. Every statement is a proposal unless marked **Fact**. The answers
to the numbered questions are on [Answers](answers.md); this page is the
working.

> **Decided 2026-10-16.** The first release's board is the **standard board**
> (dice) by default, and the **phonics board** described on this page and in
> [Phonics](phonics.md) is a parent's switch. Where this page says "Mode B
> boards are seeded", it means the phonics board; a standard board is a roll of
> dice and holds as many words as it holds. The two game modes (alone, with
> others) are separate from the two board modes.

## What the first release plays

Section 12 narrows the full brief. Where this page says **MVP** it means that
scope; the rest is the product that follows.

| | MVP (section 12) | After the MVP |
| --- | --- | --- |
| Words | A hand-filtered, reviewed phase index (Phase A, SATPIN) | Further phases; the open dictionary |
| Mode B boards | 3x3 and 4x4, seeded to hold three to five viable words | 5x5 and 6x6; dice |
| Mode B end | The board clears; a calm resting screen | |
| Mode A | A single capture, confirm-and-correct, the start tile highlighted | Standing camera that watches the lid come off |
| Help | One level: the start tile | A ladder of five rungs the parent sets |
| Reward | Stars by word tier | Worlds, the mascot, accessories (to confirm) |
| Parent | An on-device dashboard behind a gate | Weekly report, time played, a limit, an export |
| Left out | Voice capture, export, network, timers and limits, passive time played | Each is deferred, not removed (question 62) |

## Principles

| Principle | What it rules out |
| --- | --- |
| **Nothing can be lost or failed.** | A game over, a lives count, a buzzer, a red cross, a score that goes down (7.1) |
| **Nothing pulls.** | A timer, a limit, a streak, a daily reward, a prompt to carry on, a hint offered unasked, a high score on show to chase (12.3) |
| **A board ends.** | An endless game. Three to five words, then a resting screen that invites putting the device down |
| **Sound before text.** | A screen a child has to read to use. Letters and words are the only text a game screen needs |
| **The child does the finding.** | A hint that is the word, until the parent allows it and the child asks for it |
| **Everything stays on the device.** | A network, an account, a microphone, an export (12.2) |
| **A parent is a different person.** | A child changing the help or reading the dashboard by accident: those sit behind a deliberate action |

## Words and what each does

**Fact, measured, from this repository's data.** The dictionary is ENABLE,
172,823 words, and the library's default minimum is 3 letters. The child-facing
lists in `data/lexicon` are a core tier of 237 words and ten category lists,
689 words together, which is small beside the dictionary:

| Length | ENABLE words | In a child-facing list | Share |
| --- | --- | --- | --- |
| 2 | 96 | 1 | 1% |
| 3 | 972 | 155 | 16% |
| 4 | 3,903 | 231 | 6% |

`mat`, `is`, `at`, `in`, `on`, `up` and `go` are in none of them. In the MVP the
word set is neither of these but a third, the phase index (section 12.1),
**which does not exist yet**: a reviewed list derived from the 249 words
spelled with S, A, T, P, I and N, fewer than 249 once stripped. The five
early-reader words (NIP, PAT, SAT, SIT, TAP) are where it starts.

A traced path is one of three things:

| State | What it is | What the child gets |
| --- | --- | --- |
| **Viable** | A word of the active phase's index that is on this board | A jewel in the tray; the stars of its tier; the sound-it-out sequence; its sticker the first time. It counts toward clearing the board |
| **Real, not today's** | A dictionary word of 3 or more letters that is not viable on this board | A neutral cue: the tiles settle with a soft tick, no star, **no word shown or read**. Nothing says wrong |
| **Not a word** | Anything else | The path fades over 300 ms. No sound, no text |

Why this is the answer to questions 3, 16 and 61:

- A real word is not called "not a word". `TAP` traced when only `SAT` and `SIT`
  count is a real word, and a child who is told it is not has been taught
  something false; this is the risk [Licensing](../licensing.md#the-word-list)
  already names. The neutral cue says *it is not one of today's*, with no
  words.
- Because the neutral cue never **shows or reads** the word, no block list is
  needed in the MVP: an unsuitable dictionary word is never spoken or printed.
  Only reviewed words are. An open-dictionary game after the MVP would need one.
- The cue for "real, not today's" should differ from "not a word" so a child
  can learn the difference by touch alone. Whether they can is a test with
  children.
- Which inflections are viable is the reviewer's: 127 of the 249 end in S.
  Recommended: an inflection of a viable word is viable, earns stars, and
  shares its base word's sticker (the `base` field in
  [Phonics](phonics.md#the-data)).

## Stars, tiers and stickers

The author's table (7.2), unchanged except that the sticker and accessory are
the post-MVP part:

| Tier | Letters | Feedback | Reward |
| --- | --- | --- | --- |
| Spark | 3 | Soft sparkle; the word is read | +1 star |
| Power | 4 | A rainbow trail on the text; a cheer | +2 stars, a word sticker |
| Mega | 5 or more | A large burst; a voiceover | +5 stars, an accessory (after the MVP) |

- A word's tier is its letter count, `Qu` counting two. So `QUIT` is a Power
  Word (question 19).
- Stars are earned on **every** find, a sticker only the **first** time a word
  is found on this device. Repeating the five words on many boards therefore
  earns stars and nothing is lost by repeating.
- A found word is drawn as a **jewel** (the original brief's "jewels collected
  in a bank"), whose size follows its tier. Jewels are the picture; stars are
  the count. The two descriptions in the brief are one thing seen two ways.
- There are no negative stars and no buzzer (7.1).
- The Mega voiceover and screen shake are the effects question 25 asks about:
  see [Application design](application.md#motion). The screen shake is replaced
  by a bounce of the jewel for everyone.

## Mode B: Solo Play

### The flow

```text
  solo.box ──Shake──▶ solo.shaking ──still──▶ solo.dropping ──▶ solo.playing
     ▲                                                              │
     │                                                  trace or tap │
     │                                      viable / real / not a word│
     │                                                              ▼
     │                                         solo.word-found ─ more words ─▶ solo.playing
     │                                                    │
     │                                                    └ last viable word
     │                                                              ▼
     └─── Again (child or parent) ◀─ solo.resting ◀─ solo.cleared
```

| id | When | Leaves to |
| --- | --- | --- |
| `solo.box` | The box with its lid on, centred. A **Shake** button, the board-size choice (3x3, 4x4), and two quiet corner buttons (history, parent) | **Shake**, a device shake or Space: `solo.shaking` |
| `solo.shaking` | The box rattles. At least 0.7 s and while the device is shaken, at most 3 s | `solo.dropping` |
| `solo.dropping` | The lid lifts, the dice fall, the board moves to the leading two-thirds and the panel slides in | `solo.playing` |
| `solo.playing` | The board and the panel. The child traces or taps | A word: `solo.word-found` (viable) or back at once (real, not today's; not a word). **Start tile**: the highlight |
| `solo.word-found` | The path's blocks light in turn and the word is sounded out; the jewel drops into the tray | `solo.playing`, or `solo.cleared` after the last viable word |
| `solo.cleared` | The last jewel lands; one quiet celebration | `solo.resting` after about 3 s |
| `solo.resting` | The calm resting screen (below) | **Again**: `solo.box`. Back: `home`, not the box, so leaving does not show the shake button |

Nothing is a countdown. `solo.cleared` and `solo.resting` follow by themselves
only because the board has no word left to find.

### The board size

Chosen on `solo.box` with two pictures (3x3, 4x4). The parent sets which is
the default in the parent area; the app never moves a child from 3x3 to 4x4 by
itself. The 3x3 is the first board of a new device.

### Shake, lid, drop

The board is **decided before the shake begins**: the library seeds it (see
[Phonics](phonics.md#seeding)) and hands the animation a finished board. The
shake is the presentation of that, so a replay is identical and a physics
engine is not needed. Timing, sound, the 3D scene and the reduced-motion
versions are in [Board design](board.md#the-shake-and-the-drop).

Ways to shake, all always available:

| Way | Where | Note |
| --- | --- | --- |
| Shake the device | Phones and tablets | Needs an accelerometer. The detector is pure logic over samples, so it is unit-tested with recorded traces |
| **Shake** button, 96 dp | Everywhere | A tap starts a shake of fixed length |
| Space, or drag the box fast | Desktop, browser | |

A device shake is never the only way (WCAG 2.5.4), and shake detection can be
turned off in the parent area.

### Tracing and tapping

Two ways to make a word, both always on, so no gesture is required:

| | How | Ends with |
| --- | --- | --- |
| **Trace** | Press a tile and slide across neighbours, diagonals included | Lifting the finger |
| **Tap in order** | Tap a tile, then each neighbour in turn | The **Done** jewel, or tapping the last tile again |

- Each tile lights as it joins and plays its letter's *sound* ("kuh", "a"), not
  its name. A sound is cut off by the next one.
- Tiles that make one sound link into a block as they join. How is on
  [Phonics](phonics.md#binding-a-trace-into-grapheme-blocks).
- The path is a glowing line with a dark core and a pale casing, so it shows on
  the dice and on the tray (see [Application design](application.md#colour-and-contrast)).
- Sliding back onto the previous tile takes the last tile off the path.
  Lifting outside the board cancels with no penalty. A tile cannot be used
  twice and a non-neighbour is ignored, not an error. The library will check a
  path with `validate_path` (written; `wjs::validate_path`).
- The hit area is a circle smaller than the tile, and the path stays on a tile
  until the finger leaves a larger circle, so a diagonal slide does not skip a
  tile. The two radii are **starting values for a small finger**, the one thing
  no unit test settles (roadmap, M1).
- Only the first touch draws a path. A second finger or a resting palm is
  ignored.

### The panel

| Part | Holds | Behaviour |
| --- | --- | --- |
| **Words I found** | One jewel and the word in lower case, in the order found, for this board | Tap a word to hear it. Nothing is ever removed. It does not show how many are left |
| **Hints** | One button, the start tile (**MVP**), and nothing else | One press highlights the start tile of one viable word not yet found. Absent if the parent has turned help off |
| **History** | Earlier boards, newest first | Inline thumbnails where there is room; otherwise a button that opens a sheet |

There is no **New board** button. A board ends by being cleared (next
section). A child who wants to stop uses the back control, which puts the lid
back on (`solo.box`); the board is saved in history either way.

### The resting screen

**MVP**, section 12.3: "a calm, non-stimulating resting screen that encourages
putting the device down".

- **What it shows.** A still, low-contrast picture with nothing to chase: for
  example the closed box and the mascot asleep. No numbers, no stars, no
  words, no best. The art is to be made; the brief names a mascot only as an
  idea.
- **What moves.** Nothing, after the screen has faded in over 600 ms. No loop,
  no sound, no vibration.
- **How to play again.** One quiet control, an outline button at the foot of
  the screen, 64 dp, available only once the celebration has finished. It
  needs a deliberate press: **hold for one second** while a ring fills. That
  is a brake, not a gate: a parent or a child can do it, and a screen-reader
  user activates it by the usual action.
- **What it never does.** Start another board by itself, remind, or ask.
- **What is kept.** The board, its words and its stars are saved to history,
  which the child can open; none of it is shown here.

### High scores, without a pull

The original brief wants a high score. Section 12.3 asks that one be worked out
"without a pull to continue". Proposed:

- The **best** is the most stars in one board, for each board size, on each
  device (one child per device, question 27).
- It is **recorded silently** and celebrated **once**, in the moment it
  happens: inside the celebration of the last word, one extra sparkle and a
  sound. It is **never shown as a number** to the child, on the resting screen
  or the box, so there is nothing to chase.
- It is shown to the **parent**, on the dashboard.
- A game that does not beat it says nothing.

That is a design for questions 4 and 63. Whether even the single sparkle is a
pull is for the reviewer.

## Help

**MVP: one level, the start tile.** It is the same in both modes (question 68).

| Rung | The child sees | Gives away | In |
| --- | --- | --- | --- |
| 1 | A number: "5 words for you" | How many are left | After the MVP |
| 2 | **One tile glows and pulses** | Where a word starts | **MVP** |
| 3 | Slots `C _ _` and, if it has one, a picture for its group | Length, first letter, group | After |
| 4 | The first two tiles lit and an arrow | Which way it goes | After |
| 5 | The whole trail lit; the word read | The word | After |

After the MVP the parent sets the highest rung in a dial of five stops (Off,
Nudge to 2, Clue to 3, Trail to 4, Answer to 5), the child climbs one rung a
press, and the app never offers a hint unasked. Questions 2 and 6 are answered
on that footing.

How a start tile is chosen: among viable words not yet found, the shortest,
then the one nearest the top left, so the same press gives the same answer; it
stays until that word is found or the child asks again. Nothing is announced,
lit or nudged by the app on its own.

## Progress without a pull

| What | Design |
| --- | --- |
| **Stars** | The count. Kept on the device. Shown to the parent; shown to the child only in the vault |
| **The vault ("My Word Vault")** | One album, a sticker for each word of the active phase's index. A found word is its picture; a word not yet found is a grey outline. **Tap to talk** reads the word and, if the reviewer has written one, a sentence. Opened by the child on purpose; never shown after a board. The pictures need art that the repository does not have (question 21) |
| **Worlds (7.2)** | A theme: the board's look and the background. The unlock is by stars. **Not in the MVP** unless the author says so (12.5). Designed as a swap of design tokens, so no world changes a rule |
| **The mascot** | After the MVP; the same character in every world (question 28) |
| **Phases** | Chosen by the **parent**, never by the app (question 29). Two axes: the phase is *what the board is made of*; tiers and stars are *what the child earns* |

The brief's thresholds (0 to 20, 21 to 50, 51 and up stars) are reached in a
sitting or two: a board holds 3 to 5 words worth 1 to 2 stars each, which is
about 3 to 10 stars, so 20 stars is two or three boards (**estimate**, from the
brief's own numbers). If worlds come in, the author should set the thresholds
from how many boards a week they expect.

## Mode A: the Family Assistant and Scanner

The family plays with a real board; the tablet looks at it and helps the
child. The app is not the game.

### The MVP: one capture

Section 12.4 and the planning assumption of question 67.

```text
 scan.ready ──Capture──▶ scan.reading ──▶ scan.check ──Yes, play──▶ assist.playing
     ▲                                        │                        │
     └──────────── Read again ◀───────────────┘         New board      │
     └────────────────────────────────────────────────────────────────┘
```

| id | When | Leaves to |
| --- | --- | --- |
| `scan.ready` | The camera is on, with a frame to put the board in. A **Capture** button, 96 dp. **Type in your board** if there is no camera or the parent prefers | **Capture**: `scan.reading`. **Type in**: `scan.check` with an empty grid |
| `scan.reading` | One frame is read on the device. A still picture says so | `scan.check` |
| `scan.check` | The replica, upright, with doubtful tiles ringed. A grown-up confirms | **Yes, play**: `assist.playing`. **Read again**: `scan.ready`. A tile: the letter picker |
| `assist.playing` | The replica in the leading two-thirds and the panel. **Show me a start** | **New board**: `scan.ready` |

The camera is **on only in `scan.ready`**: once the frame is taken nothing
needs it, and a child's room is in the picture. The brief has the camera on
while the tablet is on its stand; this keeps it on only while it has something
to do, and the author may reverse that. Frames are processed on the device and
not kept (roadmap section 8), there is no audio capture, and nothing is sent.

### The check screen

```text
  +-----------------------------------------------------------+
  |   Is this your board?                        [ turn ]     |
  |                                                           |
  |        +------+------+------+------+       4 x 4          |
  |        |  C   |  A   |  T   |  S   |                      |
  |        +------+------+------+------+                      |
  |        |  O   | (R?) |  E   |  D   |   ringed: the reader  |
  |        +------+------+------+------+   doubts this tile    |
  |        |  ...                       |                      |
  |        +------+------+------+------+                      |
  |                                                           |
  |   [ Read again ]               [      Yes, play      ]    |
  +-----------------------------------------------------------+
```

- The replica is **upright**: the real dice are turned every which way and the
  replica is not, which is itself a help to a young reader. **Turn** rotates
  it a quarter turn to match how the child sees the real board.
- A tile is one to three letters (**Fact**, the library's limit). The picker has
  A to Z, `Qu` and a multi-letter entry, because some larger sets carry other
  pairs (**unverified**, see [Board design](board.md#sizes)).
- The size is counted from the picture and shown first; nothing is solved until
  the board is confirmed (question 11).
- An OCR error would otherwise feed a wrong board to the solver (question 67),
  so **the check is never skipped in the MVP.** No tile accuracy has been
  measured; nothing is built.
- Nothing on this screen is for the child, and no message says the app could
  not read something because of what the child did.

### Playing

- The confirmed board is solved against the **phase index only** (section
  12.4). **Fact, measured:** a real-looking 4x4 board (letters drawn by ENABLE
  letter frequency, which is a test corpus) holds, for an index of the 68 to 122
  SATPIN words that is the likely size of a stripped Phase A, a mean of 3.2 to
  3.4 viable words, and **35% of boards hold none**. If the index were only the
  five early-reader words, 70% hold none (mean 0.4). The design therefore has
  to handle "nothing for you this time" as an ordinary case: a picture of the
  box and a button to read the next board, not a message and not a blame.
- **Show me a start** highlights one viable word's first tile at a time (above).
  It is the only help. The child traces from it on the replica and the same
  sound-it-out sequence plays.
- The child's words are the tray's jewels and stars, for the round. The app keeps
  **no scoreboard** against the other players (question 5); a word found on the
  real board and not traced is not counted and nothing says so.
- Hint audio is silent in this mode by default, because the others are at the
  same table; the display is a picture.
- The stars go into the same total as Solo Play's; a round does not make a
  "best", because the board was not shaken by the app.

### After the MVP

- **Stand-and-watch.** The camera runs on the stand, finds a board-shaped grid
  that has been still for about a second (to be tuned on recorded frames) and
  reads; a **Read it now** button stays. Not the lid coming off: a hand and a
  lid moving over the board look like a board moving. It reads, then asks for
  the same check.
- **A new round** is the **New board** button, never "the camera noticed a
  different board": an adult lifting the board to look at it would end the
  child's game.
- **Tailorable help** (rungs 1 to 5, above) in the parent's dial.

### No camera

With no camera, or one the person refuses, the Mode A tile says so in a
picture and `scan.ready` offers **Type in your board**: the check screen with an
empty grid, the size chosen by the adult. It is a screen Mode A needs anyway;
the current window already has a grid to type a board into. Mode B is whole
without a camera.

## What this design does not do

- **No timer and no limit.** The real game has a sand timer and a family may
  use it; the app does not add one.
- **No level that changes itself.** The app never moves a child from 3x3 to
  4x4, from one phase to another, or raises the help.
- **No streaks, daily rewards, unlockables to buy.**
- **No scoreboard across players, and no accounts.**
- **No voice capture, no export, no network, no analytics.**
- **No time played.** Nothing is stored from which a figure for time on screen
  could be made.

## Unknowns

- Whether a child of five can use tap-in-order and trace without being shown.
  There is no first-run tutorial in this design; it is a test with children.
- The phrases, the voice and the accent. Recordings follow the review.
- Whether the "real, not today's" cue is understood, and whether the single
  best-score sparkle is a pull.
- The size of the phase index, and so how often a scanned board holds nothing.
