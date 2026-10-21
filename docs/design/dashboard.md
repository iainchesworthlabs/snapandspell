# The parent area: dashboard and settings

What a parent sees and sets, how it is guarded, what it measures and how it
must be worded. Written from [product requirements](../product-requirements.md)
sections 7.4, 10 and 12. Proposals unless marked **Fact**.

**As built:** [The parent area as built](parent-area.md) records the store, the
gate, the numbers and the screens, where the build chose between readings of this
page, and what is for the reviewer.

**Scope.** The MVP's dashboard is **on the device and nowhere else**
(section 12.2): no export, no printable summary, no e-mail, no voice clip, no
time played and no limit on play. Those stay in the brief and are deferred, not
removed (question 62). (A board clock the parent can set is built and is not
the limit this sentence defers: [D8](../roadmap.md#4-decisions-that-gate-the-plan).) This page designs the part that is in the MVP and records the
constraints the deferred parts have to meet when they return.

Everything shown is a statement about a child's learning. It needs the literacy
review (gate G7) before a parent sees it: the definitions and the wording below
are proposals for the reviewer, not content.

## The gate

Question 49. The gate keeps a child out of the settings and the data by
accident; it is not security.

| Option | A five-year-old can pass it? | Difficulty for a parent | Notes |
| --- | --- | --- | --- |
| A sum | Some can, and it asks a parent to read and calculate | Low, but excludes some parents | Common in children's apps; the brief names it |
| A long press | Easily: children hold buttons all the time | None | Too weak alone |
| **Two fingers on two marked spots for two seconds** | Rarely | Low; needs both hands | Recommended |
| Both | | | More friction than the data needs |

**Recommendation:** the two-spot hold. A screen-reader user is given the same
screen through an accessible action, since a hold cannot be done blind.

> **As built, decided 2026-10-21 (D9): one press-and-hold.** The gate screen is a
> single large button; holding it for two seconds (a finger, a mouse, or Space or
> Enter) presses both of the library's hidden spots for the person, so the
> library's rules are unchanged (the hold, the release that starts it over, the
> accessible action) and only the way in changed. This is the "long press" the
> table above calls *too weak alone*: a determined child passes it with one
> finger. The author kept it knowing that. It guards the settings, the
> dashboard and the scanner, and the MVP has no purchase, link or network for it
> to protect, so it is a brake against accidents and not security. Reconsider it
> before a store listing (roadmap G6).

The
**age assumed unable to pass it** is not established by anything in this
repository; the gate is to be tried with children. **Unverified:** what Apple's
and Google's children's-app policies accept as a gate. The roadmap already
asks that they be read before a store (G6). The MVP has no links out and no
purchases, so the gate guards settings and data only.

## What the dashboard shows (MVP)

| Part | Shows | Source |
| --- | --- | --- |
| **Sounds** | For each grapheme of the active phase: how often a word with it has been on a board, how often one was found, and how often unassisted | The store, below |
| **This week** | Boards cleared, stars, words met for the first time, the longest word | The store |
| **The vault** | The stickers, as the child sees them | The store |
| **Best** | The best board for each size | The store. **Also shown to the child, ungated, on My jewels** ([D10](../roadmap.md#4-decisions-that-gate-the-plan)); the design had this the only place |

It does not show: time on screen, a streak, a comparison with other children,
a score out of anything, a grade.

### The store

Per find: the word, the board size, the phase, the day, whether the start tile
was used for that word. Per board: its letters, its size, how many viable words
it held. Plus: the stars, the best, the stickers, the last twenty boards, the
settings. **Nothing else**: no duration, no identity, no image, no sound.

### Definitions (questions 42 to 48)

| Term | Proposed definition | Why |
| --- | --- | --- |
| **Unassisted** | The start tile was not shown for that word on that board | The only help in the MVP, so it is known exactly |
| **Found** (for a grapheme) | At least one word containing it, found | |
| **Often found** | Three or more *distinct* words containing it, found unassisted, on any boards | The brief's "3 or more words" |
| **Met** | A word containing it was on a board | A sound that never came up is not a blind spot |
| **Available, not found** | Words containing it that were on boards and were not found, as a count over boards: "on 6 boards; found on 1" | The brief's "completely missed over multiple boards", as a number not a label |
| **A week** | From the device's calendar locale's first day, through the seventh | Question 47. The report is **computed when the dashboard is opened**: no notification (which would need a permission), no schedule |

**Dropped from the MVP.**

- **"Emerging"** (the child started a word and abandoned it, question 43). A
  touch plays the letter's sound, so a child exploring tiles to hear them
  produces many short paths that are play, not difficulty. A trace of one tile
  cannot be told from a stray touch at all. Showing the inference to a parent as
  a finding is not reliable enough, so it is not recorded. If it returns it
  should be validated against recordings of real play first.
- **"Family play insights"** (question 48). In Mode A the start tile is
  pressed by whoever holds the tablet; the app cannot tell who. The count is
  "start tiles shown", never "the child asked".

### Wording (question 44)

Observations, not assessments, and never about a gap:

- "Words with *ai* have been on 6 boards. 4 were found." Not "weak on ai".
- "Found without a start tile: 3." Not "mastered".
- "This week: 3 boards cleared, 14 stars, 2 new words." Not a grade.
- A sound that has not been met says "not met yet", never "missed".
- No finding is shown about a child who has not played: **a gap in play is not
  a deficit** (the brief's own caution).
- The advice the brief gives ("point out a 'ch' word") is phrased as a
  suggestion to look for together, and each line is reviewed.

## Focus sounds

The parent ticks the sounds the school is on this week (7.4). They steer
seeding (see [Phonics](phonics.md#seeding)): targets prefer words with a ticked
grapheme. The relationship to phases (question 45): **a phase is a set of
sounds; the focus selector picks individual ones within the phase.** A sound
outside the active phase cannot be ticked. If too few words use a ticked sound,
the seeding falls back to the phase generally and the dashboard says "few words
use this sound yet".

## Settings

| Setting | Values | Default |
| --- | --- | --- |
| Help | **As built:** Off, a start tile (the first press lights it, the next hides it), or two letters (the start tile, then the word's first two tiles); default two letters ([D12](../roadmap.md#4-decisions-that-gate-the-plan), to confirm). The design: On or Off, and after the MVP a dial of five stops | Two letters |
| Time on the board | Off, count up, or count down for 1, 2, 3, 5, 10, 15, 20 or 30 minutes (default 5 when counting down); a count-down ends the board and keeps what was found. Not in the original design ([D8](../roadmap.md#4-decisions-that-gate-the-plan)) | Off |
| Board | Standard (dice) or Phonics (a phase's words, grapheme blocks, sounding out) | Standard |
| Phase | A or B (generic names; contents are the reviewer's). Used by the Phonics board | A |
| Focus sounds | A subset of the phase's graphemes | None |
| Board size | 3x3 to 6x6 on the standard board, 3x3 or 4x4 on the phonics board | 3x3, then the last used |
| Sound it out | Full, the word only, off | Full |
| Celebration sounds | Full, quiet, off | Full |
| Sound, Vibration | On, off | On |
| Shake the device | On, off | On |
| Motion | System, reduced | System |
| Colour scheme | System, light, dark | System |
| Language | English, French, Spanish, German, Yiddish, Hebrew, Chinese or Japanese; System is the device's own when it is one of the eight, English otherwise | System |
| Text size | 1, 1.25, 1.5 | 1 |
| School font | Off, or a font whose licence permits bundling (to confirm) | Off |
| Reset | Clears boards and finds; stars and stickers only with a second step | |
| About, licences, privacy | | |

## Several children, a new device

- **One profile per device** in the MVP (questions 27 and 51). Separate
  heatmaps are a later feature; they would need picture avatars with no names
  and a "who is playing" step on `home`, and each profile's store is separate.
- **Reset** is in the parent area behind the gate and says what it clears.
- **A new device starts fresh.** Nothing moves between devices (question 50).
- **Backups.** An iOS app's data is included in a device backup by default
  (**general knowledge, not checked**). That is the user's own backup, not the
  app sending anything, but "air-gapped" and "zero export" read as wanting it
  excluded; the cost is that a lost iPad loses the progress. The author decides.

## Deferred, and what each must meet when it returns

| From the brief | Question | When it returns it must |
| --- | --- | --- |
| "Read aloud" voice recording | 41 | Have its privacy review first; the microphone permission, its retention and deletion, and each store's children's-app policy |
| Time played | 52 | Define "played" as active play (a touch within the last 30 s, say) and a day's end; not stored in the MVP, so nothing past can be reconstructed |
| A parent-set limit on play (a daily or weekly allowance, a locked session) | 53, 54 | Sit beside the system's own screen-time controls (iPadOS has them; a browser has none); never end play abruptly; and square with "nothing pushes the child to continue". The board clock above is not this: it ends one board, on the parent's setting, and nothing is stored |
| The printable summary and its e-mail | 55 to 57 | Be a parent's action with a preview, behind the gate, handed to the system's share and print facilities with no stored address; the first route by which a child's data leaves |

The weekly report's **read-aloud shared moment** and **time played** are
therefore not in the report in the MVP.
