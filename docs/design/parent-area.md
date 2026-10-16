# The parent area as built

What was built of [the dashboard design](dashboard.md): the store, the gate, the
dashboard's numbers, the screens and the privacy checks. This page records what
the build does, where it chose between readings of the design, what is checked
and by which test, and what is **not** established. The design pages are
proposals; this page is **Fact** about the code unless a line says otherwise.

**Scope kept to the MVP's frozen privacy rules** ([product requirements](../product-requirements.md)
12.2, 12.3, 12.5): no voice, no export, no network, no time played, no limit, no
passive tracking and no several children. None is built, not even behind a flag.

## What is where

| Piece | Where | Qt? |
| --- | --- | --- |
| Days and the week | `wjs/calendar.hpp` | No |
| Settings, and which values are allowed | `wjs/settings.hpp`, `settings.cpp` | No |
| The store: records, file format, migration, damage, atomic write | `wjs/store.hpp`, `store.cpp`, `store_format.cpp` | No |
| The gate | `wjs/parent_gate.hpp`, `parent_gate.cpp` | No |
| The copy between the game session's values and the store's | `wjs/session_store.hpp`, `session_store.cpp` | No |
| The dashboard's numbers, the focus sounds | `wjs/dashboard.hpp`, `dashboard.cpp` | No |
| Where the file goes | `wjs::platform::user_data_directory()`, one definition per platform under `src/platforms` | No |
| The controller: gate events, dashboard and settings as maps, the resets, the clock | `src/app/parent/parent_controller.*` | Yes |
| The screens | `src/app/qml/parent/*.qml`, a QML module `WordJewelsSolver.Parent` | Yes |
| Every sentence a parent reads | `src/app/qml/parent/ParentWording.qml` | Yes |
| A stub shell to try the screens in | `tests/app/parent_ui` (test support, not part of the application) | Yes |

The library stays Qt-free and OS-free. The module is linked into the application
by one line in `src/app/CMakeLists.txt`.

## Integration (not done here)

The Solo Play session owns `Main.qml` and the home screen. `ParentArea.qml` is one
screen with one signal, so wiring it in is a few lines in that shell once both
are on `develop`:

```qml
import WordJewelsSolver.Parent
// in the screen switch:  case "parent": return parentScreen
Component {
    id: parentScreen
    ParentArea { controller: parentController; onClosed: Navigation.back() }
}
```

and `Home`'s `parentRequested()` goes to the gate (`Navigation.go("parent")`,
`parentAvailable: true`). `ParentController` is shared with the game: the program
makes **one** `wjs::Store` and hands it to both (`useStore`), so the two never write
the file separately. The shell also applies two settings the controller exposes:
`reducedMotion` (the parent's Motion override; the system's own setting is the
shell's to read) and `textScale` (1, 1.25, 1.5). These screens apply the text
scale to themselves already.

## The store

One small text file, `store.wjs`, in `<user data directory>/WordJewelsSolver/`:
`%LOCALAPPDATA%` on Windows, `$XDG_DATA_HOME` or `~/.local/share` on Linux,
`~/Library/Application Support` on macOS (the last two are written and not run).
The per-user, per-device directory, never a roaming one: nothing moves between
devices (question 50).

What it holds, and what is **not** in it:

| Holds | Does not hold |
| --- | --- |
| For each find: the word, the board's size, the phase (empty on the standard board), the **day**, whether the start tile was shown for that word, which board | A duration, a time of day, an identity, a location, an image, a sound |
| For each of the last twenty boards: its tiles and size, how many words it held, and on a phonics board which, whether it was cleared, its stars | Anything about who played or how long |
| The stars, the best cleared board of each size, the words found at least once (the vault), a tally for each of the last 35 days | |
| The settings | |
| The game in progress (the session's state, size, the seed of the next deal, the board and the lit word), so that a restart is not a lost board | |

Choices where the design reads two ways, **to confirm with the author**:

- **A day, not a time of day.** `application.md` says "the time of day it was
  found"; `dashboard.md` says "the day". The store keeps the day only, because
  [game.md](game.md#what-this-design-does-not-do) says nothing is stored from
  which a figure for time on screen could be made, and a time of day on each find
  would be that. The week report needs only the day.
- **A phonics board lists the words it held** (at most 24). "On 6 boards" needs to
  know which words were on a board; a count is not enough. A standard board
  holds hundreds and lists none. Restoring a game needs the whole list.
- **The day's tallies.** "This week" must not depend on how many boards or finds
  the store still holds individually, so a tally of boards cleared and stars is
  kept for each of the last 35 days.
- **A best is a cleared board's.** The most stars on one cleared board of each size.
  `new_best` is told once, when the board is cleared with more than a best that
  exists; the first board has nothing to beat, a tie says nothing, and an
  abandoned board sets no best (game.md: "a game that does not beat it says
  nothing"). Whether even the one sparkle is a pull is the reviewer's.
- **Stars are stored per find**, as the game gave them, and not worked out again
  from the word: a later change to the reward table must not rewrite history.

### Bounds

Every list has a bound, so the file cannot grow without limit. At every bound the
file is under 128 KiB (`max_store_bytes`), and a file past that is refused before it
is read. A store of a busy few months (100 phonics boards) is under 32 KiB (a test).

| List | Bound | Past it |
| --- | --- | --- |
| Boards | 20 | The oldest goes |
| Finds | 300 | The oldest goes |
| The vault | 1,000 words | The word met longest ago goes, never the one just added |
| Day tallies | 35 | The oldest goes |
| Words a phonics board lists | 24 | None are listed, and that board cannot be resumed |
| A word | 3 to 20 letters | The snapshot is refused whole |

### Versioned, migrated, safe against damage

The first line is `wjs-store <version>`; the last is `end <checksum>` (FNV-1a over
the lines before it). The version is **1**; there is no earlier one, so the
migration list is empty and a test holds the mechanism to a pretend older format
(`wjs::MigrationStep`: a step rewrites a version `n` file as `n + 1`, on the lines
as words, so the code that read the old layout is not kept).

| What is found | What happens |
| --- | --- |
| No file | A new store; nothing written until the first change |
| A file of this version | Read |
| A file of an older version | Migrated; the file as it was is **kept** as `store.wjs.v<n>` before the next save replaces it. If it cannot be kept, nothing replaces it |
| A file that is damaged (cut, changed, not a store, too large, a record the bounds do not allow, a migration that refuses it) | **Moved aside, whole**, as `store.wjs.unreadable` (then `.1`, `.2`, never over an earlier one) and the program goes on with a new store. The parent is told |
| A file of a **newer** version | **Left exactly as it is**; the program goes on in memory and saves nothing. The parent is told |
| A file that cannot be opened | Left alone; nothing is saved. The parent is told |
| A disk that refuses a write | Noted; the game goes on |

A save is written whole to `store.wjs.tmp` and renamed over `store.wjs`, so a program
stopped half way leaves the old file or the new one. Nothing else is ever left in
the directory (a test lists it). The rename over an existing file is checked on
Windows; it is **not checked** on Linux or macOS here.

### What the game session hands over

`Store::record(BoardSnapshot, today)` after each find and when a board ends, and
`Store::save_session(SavedSession)` for the game in progress. Both are plain values
of strings, numbers and flags with the field names of the game session's own
`BoardRecord` and `SessionSnapshot`, so the copy between them is mechanical; the store
does not include the game's headers, and the copy is its own file, `wjs/session_store.hpp`
(`snapshot_of`, `record_of`, `saved_session_of`, `session_snapshot_of`), which is the one
place that knows both sets of types. A find's moment is a `Day`, the unit the game gives
as its `Stamp`. The same board's snapshot may be sent again and again: only what is new
is recorded.

**What the copy does** (the game session's `restore()` compares its record with its own
re-solve exactly): going to the store it takes each find's stars from its `reward()` (the
session never stores them) and its stamp as a day (a stamp outside 32 bits cannot be
stored, so the conversion answers nothing); the store brings words and tiles to its own
form (lower case, `S,A,T/P,I,N/T,A,P`). Coming back it turns the tiles into the session's
own text (`S A T/P I N/T A P`, a space between tiles) and the viable words into **upper
case in alphabetical order**, or `restore()` answers `board_changed`. A board the store
kept no list of (more words than it lists), a size of other than 3 or 4, and a state the
session does not save (shaking, dropping, cleared; it saves those as the box and resting)
are refused. A board with no phase is a standard board; the session deals the phonics
board only so far, and when it gains a board mode this is where it maps to the phase
(an empty one for standard). The test `[session-store]` takes a real session through the
store's file and back, including one the session dealt itself, and `restore()` accepts it.

## The gate

[dashboard.md](dashboard.md#the-gate): two fingers on two marked circles for two
seconds. The rules are the library's and have no screen and no clock of their own
(`Gate` is told the time with each touch, so a stopped clock cannot open it and a
late one only opens it late).

- The circles are a diagonal apart (a quarter in from each side, half the width
  between them), at least 48 dp in radius, so that one finger cannot be on both and
  a flat palm falls between them. A layout whose circles are closer than four radii
  is refused as unusable.
- Exactly two touches, one in each circle, held without a break. A finger that
  slides out, lifts, or a **third touch** starts the hold over.
- **Accessibility.** The first circle carries an accessible press action
  that opens the gate with no hold; a tap is not that action.
- **Desktop.** A mouse makes one touch, so the second circle is held with the
  **Space** key (a mouse on the left circle, Space for the right). Not in the design;
  without it the gate could not be passed on a machine with no touch screen.
- Leaving shuts it again (`lock()`), so the next visit meets the gate.

A property test drives 400 random runs of touches (fingers in and out of the circles,
short and long) against an oracle written separately, and the gate must be open at
the end exactly when two circles were held for two seconds.

**Not established:** which age cannot pass it (the gate is to be tried with children;
the library's number is two seconds because the design says so), and what Apple's and
Google's children's-app policies accept as a gate (roadmap G6). The MVP has no links
out and no purchases, so the gate guards settings and data only.

## The dashboard's numbers

Computed when the dashboard is opened (`ParentController::refresh()`), from the store
alone, with no notification and no schedule. The week is the device calendar's: from
the locale's first day through the seventh (`QLocale::firstDayOfWeek`).

The definitions as computed, **all proposals for the literacy review** (G7):

| Term | As computed |
| --- | --- |
| **Met** (a sound) | The number of boards, of the last twenty the store holds, on which a word with the sound was on the board. A standard board holds no sounds |
| **Found** (a sound) | Of those boards, the number on which a word with the sound was found. Never more than *met*, so "on 6 boards; found on 1" always reads |
| **Unassisted** | The start tile was not shown for that word on that board. Exact: it is the only help |
| **Often found** | Three or more *distinct* words with the sound, each found unassisted at least once, over every board the vault remembers (`often_found_words`, 3) |
| **Words found / unassisted** (a sound) | Distinct words with it found at least once; of them, found at least once without the start tile |
| **Few words** | A *ticked* sound that fewer than 3 words of the phase use (`few_words_threshold`: the number of targets a board takes). **To confirm with the author** |
| **This week** | Boards cleared and stars from the day tallies; new words (met for the first time this week) from the vault; the longest word found this week |

A sound is a grapheme of the active phase, once for a grapheme that makes several
sounds. With **no phase data** (none exists in the repository until the reviewer
writes it) the dashboard has no sounds and the Phonics board cannot be chosen.

Wording (`ParentWording.qml`, one file): the sentences the design quotes are there as it
quotes them, with the singular and the plural of each. Where the design is silent the
build chose, **each to confirm**:

- **Nothing is said of a week without play** (a gap in play is not a deficit), and a
  part of the week sentence that is nothing is left out ("This week: 14 stars, 2 new
  words." when no board was cleared).
- "The longest word: paint." and "Nothing to show yet." (a store with nothing in it).
- The notes about the saved progress, for the parent only: "The saved progress could
  not be read, so a new one was started. The old file was kept on this device."; "...
  made by a newer version of this app. It has been left as it is, and nothing will be
  saved this time."; "Progress could not be saved on this device."
- The about, licence and privacy paragraphs.
- The focus-sound picker shows, for each ticked sound, "on 6 boards; found on 1" or "not
  met yet", and "few words use this sound yet".

**Held by tests** (`parent.wording`, `parent.screens`): the design's sentences are in
the file; none of these words is in any parent screen, in source or as drawn: *weak,
mastered, mastery, missed, blind spot, emerging, grade, score, streak, behind, fail,
deficit, wrong, poor, struggl, below, time played, minutes, hours, daily*; no `qsTr`
outside the wording file.

## Settings

All of dashboard.md's table is stored and edited, with the values it names, and the
controller refuses any other (a test tries a long list). Two rows are conditional:

- **Board: Phonics** is disabled and the **Phase** row is hidden until phase data is
  loaded.
- **Focus sounds** is the phase's graphemes; a sound outside the phase **cannot be
  ticked** and is dropped when the phase changes.
- **School font** is not offered: no font whose licence permits bundling exists
  (dashboard.md, roadmap). The setting is stored and validated and the screen has the
  row for when one does.

**Reset** is two separate confirmed steps, each saying what it clears: the boards and
finds (with the day tallies and any game in progress), and the stars, the best and the
stickers. Neither touches the settings.

## Privacy checks (CI)

[application.md](application.md#privacy-built-in-and-checked) lists the rules; what is
checked now:

| Rule | Check | Test |
| --- | --- | --- |
| No audio recording | A scan of first-party source and manifests for the capture classes, calls and permission strings | `privacy.sources` |
| No network capability | The same scan for the network classes and headers, a URL literal in code, the Qt modules whose job is the network, the manifest permissions | `privacy.sources` |
| Zero export | The same scan for printing, sharing, saving through a dialog, the clipboard, a URL handed to the system, mail, a launched program, and any file written outside `src/lib/src/store.cpp` | `privacy.sources` |
| The checks themselves | 50 cases on throw-away trees: each rule finds its violation and leaves alone a comment, the store's own source, the prototype, a name inside a longer word | `privacy.sources.selftest` |
| Local store | Round trip, migration, damage, newer version, nothing left in the directory | the `[store]` cases of `wjs_unit_tests` |
| Store size cap | The worst case the bounds allow is under the cap and reads back; a file over it is refused; a busy few months is under 32 KiB | the `[bounds]` cases |
| Nothing but what the design says | The file has only the lines the design lists, and no time of day (no colon, no ten-digit number) | the `[privacy]` cases |
| What a parent reads | The wording, source and as drawn | `parent.wording`, `parent.wording.selftest`, `parent.screens` |

**Not checked yet**, each waiting for its target: a built Android package's permissions,
an iOS property list, a web page's content-security policy (no such target exists); a
QML network manager that refuses remote URLs. That one needs `Qt6::Network` linked
directly (its base class is declared there), which is exactly what `network.modules`
forbids, so it is a decision for the author and the UI session (Qt Qml links Qt Network
itself, so "no capability and no use" is the rule, not "no module"). The scan does not
cover `src/app/prototype/` (a program of its own for trying the 3D board, which writes
its frames to disk) nor `tools/` and `docs/`.

## For the literacy reviewer

Everything shown to a parent is a statement about a child's learning and needs the
review (G7) before a parent sees it. To confirm or change: the **definitions** in the
table above (especially *met* and *found* counted over boards, *often found*, *few
words*); **every sentence** in `ParentWording.qml`; and the **order** the sounds are
listed (the graphemes file's). Nothing here depends on any phase file: the tests carry
their own made-up phase (`TEST DATA`), and no word list or recording is in the
repository.

## Verified, and not

**Ran on Windows 11** (MSVC 19.51, Debug with AddressSanitizer, Qt 6.12.0): the whole
`ctest`; the library, gate, store and dashboard tests; the controller tests; the
screens, driven in the stub shell with the software renderer under the offscreen
platform, with pictures of the gate, the dashboard (wide, narrow, empty), the settings
and the about page looked at.

**Not run:** Linux and macOS (the platform paths are written, compiled by no one here;
CI runs them); a device, an iPad, a touch screen (the gate was driven by synthetic
touches and the check's clock); VoiceOver or TalkBack (the accessible names and the
press action are set and not heard); a child using the gate; the dark theme (its
values are not computed); the stores' rules on a gate; the wording in front of a
reviewer.
