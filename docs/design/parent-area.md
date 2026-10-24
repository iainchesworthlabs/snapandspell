# The parent area as built

What was built of [the dashboard design](dashboard.md): the store, the gate, the
dashboard's numbers, the screens and the privacy checks. This page records what
the build does, where it chose between readings of the design, what is checked
and by which test, and what is **not** established. The design pages are
proposals; this page is **Fact** about the code unless a line says otherwise.

**Scope kept to the MVP's frozen privacy rules** ([product requirements](../product-requirements.md)
12.2, 12.3, 12.5): no voice, no export, no network, no time played, no limit on
play, no passive tracking and no several children. None is built, not even behind
a flag. One thing the frozen text ruled out **is** built, by the author's decision
of 2026-10-21 ([D8](../roadmap.md#4-decisions-that-gate-the-plan)): a **board
clock** the parent can set (off, count up, or a count-down of 1 to 30 minutes that
ends the board). It is a setting behind the gate; the store keeps no duration.

## What is where

| Piece | Where | Qt? |
| --- | --- | --- |
| Days and the week | `wjs/calendar.hpp` | No |
| Settings, and which values are allowed | `wjs/settings.hpp`, `settings.cpp` | No |
| The store: records, file format, migration, damage, atomic write | `wjs/store.hpp`, `store.cpp`, `store_format.cpp` | No |
| The gate | `wjs/parent_gate.hpp`, `parent_gate.cpp` | No |
| The copy between the game session's values and the store's | `wjs/session_store.hpp`, `session_store.cpp` | No |
| The dashboard's numbers, the focus sounds | `wjs/dashboard.hpp`, `dashboard.cpp` | No |
| Where the file goes | `wjs::platform::user_data_directory()`, one definition per platform under `libs/platforms` | No |
| The controller: gate events, dashboard and settings as maps, the resets, the clock | `apps/app/src/parent/parent_controller.*` | Yes |
| The screens | `apps/app/qml/parent/*.qml`, a QML module `WordJewelsSolver.Parent` | Yes |
| Every sentence a parent reads | `apps/app/qml/parent/ParentWording.qml` | Yes |
| A stub shell to try the screens in | `tests/app/parent_ui` (test support, not part of the application) | Yes |

The library stays Qt-free and OS-free. The module is linked into the application
by one line in `apps/app/CMakeLists.txt`.

## Integration

The parent area is routed from the Solo Play window, on one store.

- **The screen.** `Main.qml` has a `parent` case that shows
  `ParentArea { controller: window.parentController; onClosed: Navigation.back() }`.
  Home's parent button, the family card ("Play with a real board") and the box's
  bar all emit `parentRequested()`, which goes to `Navigation.go("parent")`;
  `Navigation.parentAvailable` is true. Closing goes
  back to where the person came from (home, or the box), by Back, Esc or the system
  back, and shuts the gate again. The gate is the parent area's own and nothing in
  the shell opens it: `ParentArea` shows the gate until the controller says it is
  unlocked.
- **The store.** `main.cpp` opens **one** `wjs::Store` for the program before
  anything else: `--store <file>`, otherwise the per-user file, otherwise (no place
  to keep one) a store that is never written. `SoloGameSetup` holds it by reference
  and `ParentController::useStore()` is given it with the status it was opened with
  (the note the parent sees if the file was damaged or newer), so the two never
  write the file separately. The stand-in game, which is what runs when no phase is
  given, is made from the same store, so the parent's settings reach it as they
  reach the real one. The controller is made in `main.cpp` and handed to the window
  as the initial property `parentController`; it has the program's calendar
  (`useCalendar`) and the phases the game plays (`usePhases`; none with the
  stand-in, so the dashboard has no sounds and the Phonics board cannot be chosen).
- **The next game.** A setting the parent changes, or a reset, is told by
  `ParentController::storeEdited()`; the window passes it to
  `SoloModel::storeEdited()`, which takes a new game from the program's maker, made
  from the store as it is now (the help switch, the size to offer first and the
  sounds in focus: `SoloGameSetup::make()`). It does so at once when the box has its
  lid on, which is where the parent area is reached from, and otherwise when the box
  next has: a game in play is not taken from a child (that includes one brought back
  from the run before and not yet played out: a reset made while it waits clears the
  store's copy, but the game on the screen goes only with its lid, and its next change
  saves it again). Opening the dashboard is a read and not an edit. The phase is read once, at the start of the run: a change of
  the Phase setting is seen by the next run.
- **The tokens.** `Main.qml` binds `Theme.reducedMotion` to the Motion setting,
  `Theme.textScale` to the Text size (`controller.textScale`) and `Theme.colourScheme`
  to the Colour scheme setting, so every screen that draws from `Theme` follows them
  without being touched. The parent screens read the same `Theme` (the separate
  `ParentTheme` they once had is gone, with the parent area's own buttons and
  choices), so the parent area is in the same light or dark chrome as the rest and
  its pairs are held by the same contrast test
  ([Application](application.md#the-jewel-box-design-system)).
- **The scanner.** The dashboard's entry for a real board opens Mode A. The
  parent area keeps the gate open while the scanner is shown (`stayUnlockedOnClose`),
  so Back from the scanner returns to the dashboard and not to the gate; leaving the
  parent area from there shuts it. No test drives the route through `Main.qml`.
- **Colour scheme: System, Light or Dark.** System (the default) follows the
  operating system's light or dark setting through `SystemColourScheme` (over
  `QStyleHints::colorScheme`, with a `dark` property that notifies); Light and Dark
  are the parent's choice for every screen's chrome. `Theme.dark` is
  `colourScheme === "dark" || (colourScheme !== "light" && SystemColourScheme.dark)`,
  and the board itself is the same in every scheme (application.md, "Colour and
  contrast"). A change reaches the screens as the setting changes.
- **Motion: System follows the system.** Reduced forces reduced motion on
  (`controller.reducedMotion`: the parent chose it). System is not a choice for or
  against, so it comes to whatever the operating system asks for: `SystemMotion`
  (`apps/app/src/system_motion.*`, beside `SystemColourScheme`) is a QML singleton over
  `QAccessibilityHints::motionPreference`, which Qt 6.12 has, with a `reduced`
  property that notifies (QML does not get the change notification of
  `Qt.styleHints`, which is why it is a wrapper). `Main.qml` binds
  `Theme.reducedMotion` to `controller.reducedMotion || (controller.motion ===
  "system" && SystemMotion.reduced)`. The OR is in the window because only the window
  has both: the controller is the settings' and `SystemMotion` is the system's, and
  neither should know of the other. If the platform cannot say, Qt reports no preference and
  System is full motion. A change of the system's setting while the program runs
  reaches the screens when Qt reports it (`motionPreferenceChanged`). The design's
  rule that System is the default is [application.md](application.md#motion)'s.
- **Text size: the system's is not read.** The design multiplies the system's text
  size by the app's (1, 1.25 or 1.5) and clamps to 0.85 to 2.0. Qt 6.12 does not
  say what the system's text size is: `QStyleHints` has the colour scheme and
  `QAccessibilityHints` the contrast and motion preferences, and nothing in either, or
  in the platform theme's hints, is a text scale. The application font Qt reads from
  the platform theme is a base size that differs by platform, which is no scale. `Theme.textScale` is therefore the app's
  own size alone, and `Theme.qml` says so. Reading it would be a platform service
  (the Windows text scale factor, Android's font scale, iOS's preferred content size
  category, GNOME's text scaling factor are the ones we know of; none has been
  tried here) behind an interface under `libs/platforms`, as the user data directory
  is; that is its own piece of work.

## The store

One small text file, `store.wjs`, in Qt's per-user application data directory
(`QStandardPaths::AppLocalDataLocation`, which follows the organisation and
application names: under `%LOCALAPPDATA%` on Windows, `$XDG_DATA_HOME` or
`~/.local/share` on Linux, `~/Library/Application Support` on macOS; the last two
are written and not run), or, if Qt cannot name one, the platform's
`user_data_directory()`; `--store <file>` overrides both. The per-user,
per-device directory, never a roaming one: nothing moves between devices (question
50).

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
  nothing"). The design had the best shown to the parent only; **as built** the
  child's My jewels screen shows the best board for each size, the points and the
  week with no gate ([D10](../roadmap.md#4-decisions-that-gate-the-plan)), computed
  from the boards the store still holds. Whether that is a pull is the reviewer's.
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
the lines before it). The version is **2**. Version 2 lets the saved game in
progress be a standard board: its saved board carries the count of its words and
no list of them. A version 1 file could not hold one, so the step from version 1
rewrites nothing, and the file as it was is still kept as `store.wjs.v1` beside
the new one. A test holds the mechanism to a pretend older format
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
case in alphabetical order**, or `restore()` answers `board_changed`. A phonics board the
store kept no list of (more words than it lists) cannot come back; a standard board's
saved game carries no list, only how many words it held, which `restore()` checks the
board against. A size of other than 3 to 6, and a state the session does not save
(shaking, dropping, cleared; it saves those as the box and resting) are refused. A board
with no phase is a standard board, the session's default. The test `[session-store]`
takes a real session of each board mode through the store's file and back, including one
the session dealt itself, and `restore()` accepts it.

## The gate

[dashboard.md](dashboard.md#the-gate): the design is two fingers on two marked
circles for two seconds. The rules are the library's and have no screen and no clock
of their own (`Gate` is told the time with each touch, so a stopped clock cannot
open it and a late one only opens it late).

**As built (decided 2026-10-21, D9): one press-and-hold.** `GateScreen.qml` is one
large "hold to enter" button with a progress bar. Pressing it puts two virtual
touches down, one in each of the library's two hidden spots (ids 9001 and 9002),
and letting go, or the screen losing focus, lifts them, so the library's
rules below are unchanged and a child needs **one finger**, not two. Space or Enter
holds it by keyboard. This is weaker than the design, on purpose and knowingly: the
gate is a brake against accidents, not security, and guards the settings, the
dashboard and the scanner only. The text below describes the library's rules, which
are as they were; where it says "two touches", read it as what the button sends.

- The circles are a diagonal apart (a quarter in from each side, half the width
  between them), at least 48 dp in radius, so that one finger cannot be on both and
  a flat palm falls between them. A layout whose circles are closer than four radii
  is refused as unusable.
- Exactly two touches, one in each circle, held without a break. A finger that
  slides out, lifts, or a **third touch** starts the hold over.
- **Accessibility.** The first circle carries an accessible press action
  that opens the gate with no hold; a tap is not that action.
- **Desktop.** The button is held with a mouse, or with Space or Enter. (The
  earlier arrangement, a mouse on the left circle and Space for the right, is gone
  with the circles.)
- Leaving shuts it again (`lock()`), so the next visit meets the gate.

A property test drives 400 random runs of touches (fingers in and out of the circles,
short and long) against an oracle written separately, and the gate must be open at
the end exactly when two circles were held for two seconds.

**Not established:** which age cannot pass it (the gate is to be tried with children;
the library's number is two seconds because the design says so), and what Apple's and
Google's children's-app policies accept as a gate (roadmap G6). The MVP has no links
out and no purchases, so the gate guards settings and data only.

**What the tests hold:** the library rules and 400 random runs (`parent_gate_test`);
the controller's events (`parent_controller_test`); and a QML case that a single
press opens it (`tst_parent`). The QML case is still named for two spots, and no QML
test sends a key to the gate or cancels a hold by moving off the button.

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
controller refuses any other (a test tries a long list). Two rows differ from the
design and are decisions: **Help** has three levels (off, a start tile, two letters;
D12, to confirm) and **Time on the board** (D8) is off, count up, or a count-down of
1 to 30 minutes. Some rows are conditional:

- **Board: Phonics** is disabled and the **Phase** row is hidden until phase data is
  loaded.
- **Focus sounds** is the phase's graphemes; a sound outside the phase **cannot be
  ticked** and is dropped when the phase changes.
- **School font** is not offered: no font whose licence permits bundling exists
  (dashboard.md, roadmap). The setting is stored and validated; the screen's row shows
  only when `ParentController.schoolFonts` has a font, and that list is an
  always-empty stub.
- **Vibration** is stored and applied to nothing on the desktops (no haptic
  backend); **Sound it out** changes only whether the word is said, because no
  phoneme clip exists; **Phase** is read once at the start of the run.
- **Language** is one of the eight codes or System (the device's own when it is one
  of the eight, English otherwise). It is applied at once by `LanguageManager`
  (`apps/app/src/language_manager.*`), which installs the catalogue, calls
  `QQmlEngine::retranslate()` and turns the layout around for Hebrew and Yiddish;
  the board and its letters stay as they are (application.md, "Languages,
  right-to-left and fonts").

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
| Zero export | The same scan for printing, sharing, saving through a dialog, the clipboard, a URL handed to the system, mail, a launched program, and any file written outside `libs/core/src/store.cpp` | `privacy.sources` |
| The checks themselves | Cases on throw-away trees: each rule finds its violation and leaves alone a comment, the store's own source, the prototype, a name inside a longer word | `privacy.sources.selftest` |
| Local store | Round trip, migration, damage, newer version, nothing left in the directory | the `[store]` cases of `wjs_unit_tests` |
| Store size cap | The worst case the bounds allow is under the cap and reads back; a file over it is refused; a busy few months is under 32 KiB | the `[bounds]` cases |
| Nothing but what the design says | The file has only the lines the design lists, and no time of day (no colon, no ten-digit number) | the `[privacy]` cases |
| What a parent reads | The wording, source and as drawn | `parent.wording`, `parent.wording.selftest`, `parent.screens` |

**Not checked yet**, each waiting for its target: a built Android package's permissions,
an iOS property list, a web page's content-security policy (no such target exists). **Not
built:** a QML network manager that refuses remote URLs (this page and the application
page once listed it as checked; it is neither built nor checked). It would need `Qt6::Network` linked
directly (its base class is declared there), which is exactly what `network.modules`
forbids, so it is a decision for the author and the UI session (Qt Qml links Qt Network
itself, so "no capability and no use" is the rule, not "no module"). The scan does not
cover `apps/app/src/prototype/` (a program of its own for trying the 3D board, which writes
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
press action are set and not heard); a child using the gate; the stores' rules on a
gate; the wording in front of a reviewer. (The dark theme's pairs are measured by
`quick.contrast`; no test renders a parent screen in dark.)
