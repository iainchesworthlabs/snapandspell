# From the MVP to the full product

The brief describes a product much larger than its first release
([section 12](../product-requirements.md)). This page is the path between the
two: a sequence of releases, each of which is whole on its own, what each adds,
what each depends on, and what the MVP must **leave open** so that none of the
later ones needs a rewrite. It may take several releases; the order is a
proposal, and each release can be reordered by the author.

It does not change the [roadmap](../roadmap.md), whose milestones are the full
product's order and which says section 12 wins where they disagree. The
roadmap changes this page suggests are put as questions at the end.

> **Where 0.1 stands against this path (2026-10-22).** The release that is being
> prepared is neither R1 as drawn below nor a later one. It takes from R3 the dice
> for 3x3 to 6x6 and the open dictionary (the standard board, with no block list),
> and a parent-set help; from R4 the desktops; and from R6 a parent-set board clock
> (decision D8). It is not on a tablet: 0.1 is the three desktops plus an ad-hoc
> iOS build that has never run on a device (D11), and Android is in progress. It
> leaves out R1's reviewed Phase A index (there is no reviewer, so the phonics board
> has no data), R1's stickers by tier on screen, and every R2 item. The diagram and
> the sections below are the plan as written, not a record; the record is
> [Requirements and traceability](../requirements.md) and the
> [roadmap](../roadmap.md#4-decisions-that-gate-the-plan).

## The shape of it

```text
 R1 MVP        R2 More to       R3 Bigger boards   R4 More          R5 Out of the     R6+ Deferred
 iPad, private find and keep    and more help      devices          house             features
 ───────────── ──────────────── ────────────────── ──────────────── ───────────────── ───────────────
 Phase A index Phase B, C       5x5, 6x6, dice     Android tablet   Store submission  Voice "read
 3x3, 4x4      Vault with art   Open dictionary    Phone layouts    Licence decision  aloud"
 Seeding       Worlds, mascot   Help ladder 1-5    Desktop          Privacy review    Printable
 Start tile    Focus sounds     Stand-and-watch    Web              Accessibility     summary
 Sound it out  School fonts     scanner            (WebAssembly)    audit             Time played
 Single capture Weekly report                                                         A limit
 On-device
 dashboard
```

Each release keeps every rule of section 12.2 and 12.3 **unless a release's own
review lifts one**. Only R6 does, one feature at a time.

## The releases

> **Decided 2026-10-16.** R1's board is the **standard dice board** (3x3 to 6x6,
> the project's own dice, the full dictionary), and the phonics board is a
> parent's switch. R3's "dice" and "open dictionary" items below are therefore in
> R1; what stays in R3 is a block list and a child-appropriate cut of the
> dictionary, which the open dictionary needed.

### R1: the MVP

Section 12, as designed in [Game](game.md), [Phonics](phonics.md) and
[Application](application.md): the iPad, a private ad-hoc build; Mode B at 3x3
and 4x4, seeded to hold three to five words of the Phase A index; one help
level, the start tile; the sound-it-out sequence; stars by tier; a calm resting
screen; Mode A as a single capture with confirm-and-correct; an on-device
dashboard behind a gate. Not in it: voice, export, network, timers and limits,
passive time played, 5x5 and 6x6, tailorable help, worlds and the mascot
(unless the author says so), school fonts.

**Exit.** It installs on the author's iPad; a child can shake, trace, hear a
word sounded out and reach the resting screen; a parent can capture a board,
correct it and see a start tile; the privacy checks pass.

**Gates.** The literacy review of Phase A (G7) before any child uses it and
before any audio is recorded; the phase index written and reviewed.

### R2: more to find, and more to keep

| Adds | Depends on |
| --- | --- |
| Phase B, then the vowel teams (the reviewer says where each sound sits) | The review of each phase; more recordings |
| The vault with real pictures and sentences | Art and text, with a licence the repository can state (question 21) |
| Worlds and the mascot, as token swaps | Art; the author's thresholds (question 23) |
| Focus sounds that steer seeding | Nothing new: seeding takes them from R1 |
| The weekly report, computed on the device | Definitions reviewed (see [Dashboard](dashboard.md#definitions-questions-42-to-48)) |
| School fonts, for the fonts whose licence permits bundling | Written permission or a clear licence per font (question 38) |
| Inflections as viable words, with `base` stickers | The reviewer |

**Why second.** Everything in it is content or a theme over mechanisms R1
already has. It is the lowest-risk way to make the first release last longer.

### R3: bigger boards and more help

| Adds | Depends on |
| --- | --- |
| 5x5 and 6x6 in Mode B, where a tile fits | Dice from the project's own tool ([Board](board.md#dice-the-standard-board-in-the-mvp-decided-2026-10-16)) |
| The open dictionary as a mode ("play with any word") | A reviewed block list: the neutral cue shows no word, but an open mode would name and read words |
| The help ladder, rungs 1 to 5, set by the parent | Nothing new: the start tile is rung 2 |
| Stand-and-watch scanning (a board still for about a second reads itself) | Recorded frames outside the repository, and a tile-accuracy target (roadmap M4) |
| 5x5 and 6x6 in the scanner, and multi-letter faces such as `Th` | The same corpus |

**Why third.** The scanner's auto-read and the larger boards are where the risk
and the cost are (OCR on glossy, angled dice), and the MVP's single capture
with a check is what makes it safe to defer them.

### R4: more devices

| Adds | Depends on |
| --- | --- |
| Android tablet and phone | An Android platform fragment; Qt's Android kit is in the shared tree; the extra Qt modules for it |
| Phone layouts, portrait and landscape | The layout rule already covers them ([Board](board.md#layouts-and-sizes)) |
| Desktop: macOS, Linux, Windows | The fragments exist; packaging is built and checked on Windows, and the Qt window in CI and the packaged window on macOS and Linux remain (roadmap M6) |
| Web (WebAssembly) | An Emscripten fragment; threads need cross-origin isolation headers; no shake sensor assumed; speech unverified |

**Why fourth.** Each is a platform fragment under `libs/platforms` and a CI
leg, not a design change: the layout, the interfaces (`haptics`, `board_reader`,
`camera`, `sensors`, `speech`) and the privacy checks are written to cover all of
them.

### R5: out of the house

| Adds | Depends on |
| --- | --- |
| A store submission (TestFlight, App Store, Google Play) | Roadmap D2 (the licence) and G6 (privacy and child-safety review) |
| (Settled: the scene is 2D, decided by the author on 2026-10-24, so the store build has no licence cost from it) | [Board](board.md#drawn-in-2d) |
| An accessibility audit with real people | A screen reader on each target; children |
| A parental gate checked against each store's rules | The stores' current policies, read |

**Why here.** Nothing before it needs a store. What a store needs is decided at this
point, with a working application and its seams, not guessed now.

### R6 and after: the deferred, privacy-sensitive features

Each is **its own release or its own decision**, started by a privacy review and
a design, none designed now (question 62). They return only if the author wants
them and the review allows.

| Feature | First step |
| --- | --- |
| The "read aloud" voice recording | The review: the permission, retention, deletion and the stores' children's-app policies (question 41) |
| The printable summary, and e-mail | The review, then a parent's action behind the gate, handed to the system's share and print facilities with no stored address (questions 55 to 57) |
| Time played, and a parent-set limit | Definitions, and how it sits beside the system's screen-time controls and against "nothing pushes the child to continue" (questions 52 to 54) |
| Several children on one device | Picture avatars and separate stores ([Dashboard](dashboard.md#several-children-a-new-device)) |

## What the MVP must leave open

These are the constraints on R1's engineering that make the later releases
additions, not rewrites. They cost little now.

| In R1 | So that |
| --- | --- |
| **The board scene is one component** with the interface "given a finished board and a seed, play the shake and drop; say when settled" | R5 can swap 3D for 2D, or the reverse, in one place |
| **Board size is a parameter** from 3 to 6; layout, seeding and the scene take `n` | R3 adds sizes with data, not code |
| **Phases are data files**, cumulative, with a `reviewed` header the release build checks | R2 adds a phase by adding two files and recordings |
| **Help is a rung number** with a ceiling the parent sets; R1's ceiling is 2 | R3 adds rungs 1, 3, 4 and 5 without a new concept |
| **The word set is a dictionary object**; R1 passes the phase index, the open mode passes ENABLE | R3's open dictionary is a different argument |
| **Themes are token sets**; no colour or size is written into a screen | R2's worlds, R2's school fonts and dark mode are token swaps |
| **The store is versioned** and migrated; finds record the phase and whether the start tile was used | R2's report and R3's rungs read old data |
| **Platform needs are interfaces** under `libs/platforms/interfaces` with a no-op where a platform has none | R4 adds a fragment per platform; nothing else changes |
| **Frame and work budgets are tests from the start** ([Performance](performance.md)) | Each release inherits the gate rather than adding it |
| **The privacy checks are in CI from R1** ([Application](application.md#privacy-built-in-and-checked)) | Every later release is held to the frozen rules without a new effort |
| **No text is drawn into pictures and no phrase is typed into code** | Languages and re-recordings are a data change |

## Traceability to the roadmap

| Release | Roadmap milestones it draws on |
| --- | --- |
| R1 | M1 (Solo Play on the iPad), the single-capture slice of M4, the first slice of M2b |
| R2 | The rest of M2b; M2's lexicon and scoring |
| R3 | M4's stand-and-watch; the larger boards and dice of the brief's section 5 |
| R4 | M6 |
| R5 | M5, G5 and G6 |
| R6+ | Nothing: the roadmap defers them |

## Questions for the author and the roadmap

1. **Is the order right?** Worlds and the mascot are the brief's answer to
   "long-term novelty" (7.2) and are in R2 here because they are theme and art
   over R1's mechanisms. If they matter more than Phase B, they move to the
   front of R2 or into R1.
2. **Should the roadmap say which release a milestone belongs to?** The roadmap
   keeps the full product's order and puts a note that section 12 wins. A
   table like the one above in the roadmap would stop the two disagreeing. That
   file is on another branch; this page proposes it and does not edit it.
3. **Is R4 (more devices) before R3 (bigger boards)?** The brief names Android
   tablet and phone as platforms; the author's own devices decide.
4. **Should D2 in the roadmap record the decision?** The scene is 2D since 2026-10-24, so the
   binary is GPL-3.0-or-later and D2 has no Qt Quick 3D cost left to weigh. A line in D2 saying so
   keeps the roadmap and these pages in agreement.
5. **Is the web a release or a by-product?** The browser build is cheap once
   the fragment exists, and it is the one target with no accelerometer, no
   known speech and a memory limit.
