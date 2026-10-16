# Testing

The tests are Catch2 unit tests for the library, the platform layer and (when
the Qt window is built) its model, the scanner with a synthetic corpus, a
performance tier with its own binary, and checks about the build and the data
itself. They run in CI on Windows (MSVC), Linux (GCC) and macOS (AppleClang), 366
tests each, of which the 23 of the performance tier and the five that hold the
scanner to its accuracy target skip themselves in a Debug build (see
[Performance tests](#performance-tests) and [The scanner](#the-scanner)); the Qt
model tests run only where the window is built, which is Windows so far.

## Running them

After a build (see [Building](building.md)):

```sh
ctest --preset windows-msvc-debug              # or linux-gcc-debug, macos-clang-debug
ctest --preset windows-msvc-debug -R platform  # just the two build checks
ctest --preset windows-msvc-debug -R data      # the word-list and phonics checks
```

Every Catch2 `TEST_CASE` is registered with CTest by name
(`catch_discover_tests`), so a failing case is named in the CTest output, and
`ctest --preset <preset>` shows the output of any that fail. To run the
unit-test program directly, with Catch2's own filters:

```sh
build/windows-msvc-debug/bin/wjs_unit_tests --list-tests
build/windows-msvc-debug/bin/wjs_unit_tests "[solver]"
```

The scanner's tests are their own binary, `wjs_scan_tests`, tagged `[scan]` (with
`[geometry]`, `[image]`, `[finder]`, `[font]`, `[glyph]`, `[reader]`, `[draft]`,
`[platform]`, `[corpus]` and `[smoke]`); see [The scanner](#the-scanner).

The tags are `[board]`, `[dictionary]`, `[scoring]`, `[solver]`, `[path]`, `[dice]`
(and `[data]` for the cases that read the shipped data),
`[phase]` (and `[review]` for the review header), `[seeding]`, `[binder]`,
`[shake]`, `[rules]`, `[session]`, `[deal]`, `[platform]` and `[profiling]`, and `[app]` in
`wjs_app_tests` (with `[layout]`, `[trace]`, `[fake]` and `[solo]`). The parent area's are
`[calendar]`, `[settings]`, `[gate]`, `[store]` (with `[record]`, `[file]`, `[migration]`, `[bounds]`,
`[reset]`, `[resume]` and `[privacy]`), `[dashboard]` (with `[sounds]`, `[week]`, `[vault]`, `[best]`,
`[focus]`), `[session-store]` (the copy to and from the game session) and, in `wjs_app_tests`, `[parent]`.

## What there is

| Test file | Tests | What it holds down |
| --- | --- | --- |
| `tests/unit/board_test.cpp` | `wjs::Board::parse` | One letter per tile or separated tiles (`Qu I T`), `/` and `\r\n` as row breaks, each way a board is refused (empty, ragged, bad tile, over 16 by 16), `contains` |
| `tests/unit/dictionary_test.cpp` | `wjs::Dictionary` | Case-insensitivity, prefix versus word, words refused (apostrophe, hyphen, digit, empty), duplicates counted once, stepping through the trie, loading skips blanks and non-words, a missing file is an error value, a file round trip |
| `tests/unit/scoring_test.cpp` | `wjs::score_word` | The printed Boggle scores from 2 letters to 12, and the letter-rarity scores for `E`, `Z`, `EZ` |
| `tests/unit/solver_test.cpp` | `wjs::solve` | Adjacency, the minimum length, the path reported for a word, a tile used at most once, a word reached by several paths reported once, the `Qu` tile, the scoring scheme applied, an empty dictionary; and the work counter (`wjs/solver_work.hpp`): visits, solves and the largest, reset, and that each thread has its own |
| `tests/unit/path_test.cpp` | `wjs::validate_path`, `wjs::are_neighbours` | The word a path spells (a `Qu` tile adds two letters), each fault (off the board, a tile twice, tiles that do not touch, no tiles), no wrap-around from the end of a row, and that every path the solver reports validates |
| `tests/unit/phase_data_test.cpp` | `wjs::parse_words`, `parse_graphemes`, `PhaseSet` | The two file formats, the review header (all three on one line or one to a line, placeholders, a half-given or malformed review), each line problem with its line number, the cumulative phase index, each cross-file check, `reviewed()` and the files it names, loading a directory |
| `tests/unit/seeding_test.cpp` | `wjs::seed_board`, `RecentTargets` | The same inputs give the same board (and two boards are pinned, so a change of compiler or algorithm shows), a range of three to five words unless the board says it fell back, targets that are words of the phase and are on the board, dead jewels, focus sounds, recent targets, the cap on tries, each refused option, variety over 300 seeds; the target-length limit, and the guided fill (count kept in range, dead jewels listed as they are on the board, a function of the seed, none with letters outside the phase) |
| `tests/unit/grapheme_binder_test.cpp` | `wjs::GraphemeBinder` | The brief's `R [AI] N`, order, longest first, greedy from the left, blocks that are whole tiles (`Qu`), single letters ignored, a block growing as the path does, every path of up to four tiles covered in order |
| `tests/unit/shake_test.cpp` | `wjs::detect_shakes` | Synthetic traces: a shake, one still, a gentle wave, walking, a knock, turning over, a slow tilt, two shakes, a pause, sample rates and jitter, units, a harder threshold, damaged samples |
| `tests/unit/game_rules_test.cpp` | `wjs::classify_path`, `viable_words`, `tier_for_letters`, `reward_for`, `FoundWords`, `start_tile_help`, `is_cleared` | The three states on each word set (the standard board's has no middle state; the phonics board's holds only the index, and a real word outside it is "real, not today's" with no word handed back), a word under three letters never counting, each path fault by name, a `Qu` tile adding two letters (QUIT is Power), every one of the 10,305 paths of a 3 by 3 board against the solver on both word sets; the tier table and rewards; a word found once whatever its case and kept in order; the help (shortest in letters, then nearest the top left by distance and not by reading order, then alphabetical, the same in all 120 orders of its input, unmoved while other words are found, and following it finds every word and clears the board); cleared, and the empty board |
| `tests/unit/deal_test.cpp` | `wjs::deal_playable`, `deal_standard_board`, `next_deal_seed` | With stub boards written in the test: the same inputs deal the same board; a board with a word is dealt in one roll; an empty board is made again from the next seed (the maker is asked for exactly the two seeds); the viable list is the board's own solve; `min_viable` of 2 is kept; the cap on rolls (counted by the maker) and a board that comes on the second not being dealt at a cap of one; each refused option, a missing maker and a maker that fails; only the active set's words count; `next_deal_seed` is `std::mt19937`'s first output; each error's name. With the shipped 3x3 dice and dictionary: none of 2,000 deals has an empty viable list (and some were made again), two seeds are pinned, one of them dealt in two rolls, and dice that make no board are `bad_board` |
| `tests/unit/game_session_test.cpp` | `wjs::GameSession` | Every edge of the flow and every event refused in every other state (a table of nine events by six states, a refused event changing nothing); the three kinds of trace, found again, and an ignored one; stars by tier on a board with words of three, four and five letters; clearing, the celebration, resting and again; the help (the same tile until its word is found, an assisted find, off at the parent's switch, followed to the end of the board); the deal (the same seed gives the same boards and outcomes, different seeds differ, three to five words over 600 seeds, never none, the range and the seedings the options', the focus sounds reaching the seeding, dead jewels); the record, and a snapshot restored in each state and refused in each way; and what it never says: every one of the 10,296 paths of two or more tiles on a 3 by 3 board answered as the two word lists say, and a word only in the dictionary never handed out |
| `tests/unit/profiling_test.cpp` | `wjs/profiling.hpp` | That the variant CMake chose is the one it was asked for, that every call can be made with no profiler connected, and that a zone cannot be copied or moved |
| `tests/unit/dice_test.cpp` | `wjs::DiceSet`, `wjs::roll`, `wjs::roll_with_placements` | The file format (a face of several letters, commas, comments, a byte-order mark), each way a set is refused, a roll using each die once and showing one of its faces, a function of the seed, two boards pinned so a change of compiler shows; the placements: the die and face of each tile are the tile's letters, each die used once, `roll` the same board as `roll_with_placements` for 200 seeds, the same two boards pinned through it, and the placements of two seeds pinned; every face shown over many rolls |
| `tests/unit/dice_data_test.cpp` | The shipped dice, `data/dice` | Each size has the dice for its board and six different faces to a die with a vowel among them; for each size, 300 boards whose placements name, for every tile, the face of its die that is the tile's letters, with each die used once; the board the deal keeps is the roll of the seed it reports, die for die (so a scene is given the dice of a dealt board by `roll_with_placements(dice, dealt.seed)`); 300 boards a size rolled and solved against the dictionary hold about the words a random board does; two boards pinned, through `roll` and through `roll_with_placements`. Reads the shipped files by their path in the checkout, as the performance tier does |
| `tests/unit/paths_test.cpp` | `wjs::platform::executable_path`, `user_data_directory` | That the first names the running test program: absolute, an existing file, called `wjs_unit_tests`; that the second is an absolute path and is the same each time |
| `tests/unit/calendar_test.cpp` | `wjs/calendar.hpp` | A date as a day (against dates counted by an independent program), the ISO weekday, the week from any first day of the week, over 800 days for each |
| `tests/unit/settings_test.cpp` | `wjs::Settings` | The defaults are the table's; every value a screen offers survives; any other becomes the default for it; names, sizes, text scales and focus sounds are brought within their bounds |
| `tests/unit/parent_gate_test.cpp` | `wjs::Gate` | The layout on every screen shape; two fingers held two seconds open it, anything else does not (one finger, two on one spot, a third touch, a slide out, a lift and a press again); the accessibility action; and a property test, 400 random runs of touches, against an oracle written separately |
| `tests/unit/store_test.cpp` | `wjs::Store`, `parse_store`, `serialize_store` | The format and its round trip; a file cut at every length and with every byte changed is refused; each record checked; a later version told apart; migration through a pretend older format; what a snapshot records (stars once, first-time words, bests, tallies, cleared boards, the bounds); the game in progress; each way a file on disk can be (missing, damaged, newer, older, unreadable, a refusing disk), kept whole and never overwritten; the worst case the bounds allow is under the cap; no time of day in the file |
| `tests/unit/session_store_test.cpp` | `wjs::snapshot_of`, `record_of`, `saved_session_of`, `session_snapshot_of` | The copy between the game session's values and the store's: fields, the stars from each find's reward, a stamp that is not a day refused, a board and every saved state through the store and back as it was, the words back in upper case and in order, what cannot come back refused; and a real session (one restored from a hand-written board, one the session dealt itself) taken through the store's file and brought back by `restore()` |
| `tests/unit/dashboard_test.cpp` | `wjs::build_dashboard`, `focus_sound_choices` | Each count of a sound (met, found, unassisted, often found, few words) on a made-up phase; the week on either first day of the week; the vault and the best; nothing said of a week without play |
| `tests/app/solver_model_test.cpp` | The solver's `SolverModel` | Solving once a dictionary is loaded, a dictionary that cannot be opened, a board that cannot be read. Built only with `WJS_BUILD_APP` |
| `tests/app/shake_script_test.cpp`, `shake_script_check.js` | The 3D board scene's script (`ShakeScript.js`), run in Qt's own JavaScript engine | Each of a die's six sides can be put up with its letter upright; the cube's mesh (24 vertices, 12 outward triangles, a cell to each side); for the weighted, staged and reduced-motion scripts, every die of a play lands with the face the board says is up, and a die has on average 2.5 faces or more on top in a weighted play and 1.5 in a staged one (one under reduced motion, which does not tumble); a replay is the same checksum and another seed or another face up is not; another set of faces up moves no die (positions and angles equal at six times through a play); a board that is shown has its faces up at once. Built only with `WJS_BUILD_APP` and `WJS_BUILD_SHAKE_PROTOTYPE` |
| `tests/app/board_layout_test.cpp` | The layout rule, `SoloLayout` | The nine reference windows of the board design: which side the board takes, the tile of each size 3x3 to 6x6 against the page's table, the inset and gap either side of 600 dp, the board centred, a size offered only at 48 dp, the object recomputing on a rotation (`[layout]`) |
| `tests/app/path_tracer_test.cpp` | `PathTracer` | A slide, a diagonal that skips no tile, a fast jump, the hit circle smaller than the tile, sliding back, a tile used twice, a dead jewel, lifting outside, tapping in order and the last tile again, a trace carried on from taps, a press in a gap, the keyboard's moves, a rotated window (`[trace]`) |
| `tests/app/fake_solo_game_test.cpp` | `FakeSoloGame` | The flow of the game design: the board dealt by the shake, the size, a viable word and its stars, a word found again, a real word that is not today's (no word in the answer), not a word, clearing and resting, the best, the start tile (shortest, nearest the top left, stays until found), the history, the blocks (`[fake]`) |
| `tests/app/real_solo_game_test.cpp` | `RealSoloGame` over `wjs::GameSession`, `SoloGameSetup`, `make_solo_game()` | The port's mapping as a table, run on two tiny phases written in the test: each state, the shake that deals no board, `settled` from either state it waits in, each answer to a trace (a word of today's by tier and stars, found again, a real word that is not today's with no word in the answer, not a word, a single tile), clearing and the new best, a sticker only the first time on a device, what is handed to the store after a find and when the lid goes on, the history newest first with a dead jewel blank only on a board of the phase, the start tile and the help switch, the grapheme blocks; loading a phase directory, the settings it makes a game from, a store file round trip, and each way loading fails; and the game in progress: what the store keeps after each change (the box, the shake and the drop keep none), the size chosen written to the settings and nothing else of them, a table of runs closed in each state and opened again (a board with finds and a lit tile, cleared, resting, Again, the lid on, mid-shake), a restored game going on with its board recorded once, four records that cannot come back (another phase, a changed board, a state that does not fit, a start tile lit for a word already found) starting a new game in the box and clearing the file, the help switch off at restore, and `SoloModel` opening on a restored board or the resting screen (`[real]`) |
| `tests/app/solo_model_test.cpp` | `SoloModel` over the fake | The states as the screens see them, input before the dice settle, the find shown and ended, the tick and the fade, tap and Done, a find again, the start tile, the blocks as a path grows, a dead jewel, the last word and the rest, the lid back on (`[solo]`) |
| `tests/app/check_scene_contract.cmake` | `BoardScene.qml` (2D) and the prototype's | `app.scene_contract`: both declare the whole interface of the board scene, under the same names |
| `tests/app/run_qmllint.cmake` | The application's QML | `app.qml_lint`: no warning from `qmllint`; skipped if the application has not been built |
| `tests/app/parent_controller_test.cpp` | The parent area's `ParentController` | The gate as the screens drive it, the dashboard and settings maps, every setting changed by name and every wrong value refused, the resets, what the parent is told of the store. Built only with `WJS_BUILD_APP` |
| `tests/app/parent_ui/` | The parent area's screens | `parent.screens`: see below |
| `tests/privacy/` | The source tree | `privacy.sources` and `privacy.sources.selftest`: see below |
| `tests/parent/` | `ParentWording.qml` | `parent.wording` and `parent.wording.selftest`: see below |
| `tests/perf/` | The solver, the word list, and the code that makes a board (the roll of the dice, the seeding), binds its blocks and deals and plays a session, and the scanner's work, as `wjs_perf_tests` | See below: [Performance tests](#performance-tests) |
| `tests/scan/` | The scanner, the camera and `board_reader` interfaces and the synthetic corpus, as `wjs_scan_tests` | See below: [The scanner](#the-scanner) |
| `tests/scan/check_privacy.cmake` | The scanner's sources and the camera and `board_reader` files | `scan.privacy`, `scan.privacy.catches_a_leak`: see [The scanner](#the-scanner) |
| `tests/profiling/check_presets.cmake` | `CMakePresets.json`, `vcpkg.json`, `cmake/Tracy.cmake` | See below: `profiling.shipped_presets` |
| `tests/profiling/check_zone_names.cmake` | Every `zone<"...">` under `src/` | See below: `profiling.zone_names` |
| `tests/platform/check_fragments.cmake` | Every `src/platforms/*/platform.cmake` | See below: `platform.fragments` |
| `tools/checks/check_platform_isolation.ps1` | The source tree | See below: `platform.isolation` |
| `tools/data/check_wordlists.ps1` | `data/dictionaries` and `data/lexicon` | `data.wordlists`: lowercase a-z, sorted, unique, and every lexicon word in the dictionary |
| `tools/data/make_dice.ps1 -Check` | `data/dice` | `data.dice`: each file is what the tool makes from the dictionary as it is, so the recorded source hash and the dice both match |
| `tools/data/check_phonics.ps1` | `data/phonics` | See below: `data.phonics` |
| `tools/data/test_check_phonics.ps1` | The phonics checker itself | See below: `data.phonics.checker` |

The first solver case uses the board of the
[README example](https://github.com/iainchesworthlabs/wordjewelssolver/blob/develop/README.md#a-worked-example):

```text
C A T
O R E
D G S
```

Of the dictionary `CAT CAR CARE ARE TAR TEA DOG COD ROD CATS REST AT`, exactly
`CARE ARE CAR CAT COD DOG ROD TAR TEA` are found, in that order, because `CATS`
and `REST` cannot be traced and `AT` is below the minimum length.

## Game rules: what is pinned

`game_rules_test.cpp` uses no word list that is in the repository: the standard
board's word set is thirteen words written in the test, the phonics index is
seven of them (the same seven the phase-data tests use) and every board is
written inline, 3 by 3 or smaller.

Most cases are the design's sentences. A few pin a reading the design left open;
each is one comparison in `src/lib/src/game_rules.cpp` and one case, so a
different decision is a small change:

| Reading | Pinned by |
| --- | --- |
| "Nearest the top left" is the straight-line distance from the top-left tile, compared as row squared plus column squared. Reading order (the topmost row first) is the other reading and gives a different tile, for example the first tile of a word at row 0, column 3 against one at row 2, column 0 | `help names the word whose first tile is nearest...`, "distance, not reading order" |
| "The shortest" is counted in letters, as tiers are, so QUIT is four and not three | `help counts a word's length in letters, not tiles` |
| Ties after distance go to the upper tile, then the left one, then the word that sorts first, so the order is total and no input order can change the answer | the same case, and `help gives the same answer however the words are ordered` (all 120 orderings of five words) |
| A real word that is not today's carries no word in the answer, so the application has none to show or read | `a word that is not today's is never handed back` |
| A word of fewer than three letters is `not_a_word` on both boards, even when it is in the active set | `a word of fewer than three letters is not a word, however real` |
| Only the Power tier carries a sticker, as in the author's table (the "viable" row of the design's three-state table says "its sticker the first time", which could be read as every tier) | `each tier earns its stars, and Power a sticker` |
| A board with no viable word at all is cleared | `a board with no viable word has none left to find` |

The path check is held against the solver rather than restated: for every simple
path of a 3 by 3 board, `classify_path` says `viable` exactly when the solver
finds the path's word, and on the standard board never says
`real_not_todays`.

## Game session: what is pinned

`game_session_test.cpp` uses no word list that is in the repository: the phases
are the seven and four words the seeding tests use (TEST DATA, placeholder
sounds), the dictionary is those eleven words and nine more written in the test,
and most cases play a 3 by 3 board written out in the test and brought in through
`restore`, so that its five words and their paths are known without asking the
solver. The cases about dealing use the session's own boards and hold them to the
solver. A few pin a reading the design left open or took two ways:

| Reading | Pinned by |
| --- | --- |
| A real word is never answered "not a word", of any length. The rules (`classify_path`) call a word under three letters `not_a_word` because it cannot count; the session answers `real_not_todays` for it, the soft tick, and names nothing. | `a real word of two letters is a real word, not 'not a word'`, and the 10,296-path case, which holds the answer to the two word lists for every path |
| A path of fewer than two tiles is `ignored`: a touch to hear a letter, or a trace cancelled outside the board, is not a word attempt | `a touch on one tile, or nothing, is not a word attempt` |
| The word is named only when it is one of the phase's: `real_not_todays` and `not_a_word` carry none; the record, the snapshot, the tray and the help hold none either. A stand-in for a word a child must never meet is in the dictionary and traced, and is found nowhere the session hands out | `a word that is only in the dictionary is never handed out`, and `no path of the board is answered wrongly, or names a word that is not today's` |
| The help answers with a tile, never the word; the word it is for is in the snapshot only, because the light has to come back with the board | `the help answers with a tile and never a word` |
| A refused event changes nothing: not the state, the board, the finds or the seed | the table of events by states, which compares snapshots before and after |
| `solo.word-found` is not a state: the session is already `playing` (or `cleared`), and a trace is accepted through the screens' few moments of showing a word | `a viable path is a find`, `the last viable word clears the board` |
| A snapshot saves `shaking` and `dropping` as the box and `cleared` as `resting` | `a snapshot in shaking or dropping is the box`, `a snapshot in cleared is saved as resting` |
| The recent targets are not saved: a restored session continues the same only from the box | `a snapshot brings a session back as it was, in each state` (it checks the next board from the box only, on purpose) |

**The three-to-five guarantee** is a property over 300 seeds of each size on the
test phase: every board holds a word (counted by the solver, not the session),
the report says `in_range` exactly when the count is three to five, the seedings
are at least one and at most `max_seedings`, and all 600 land. It holds for this
phase because the phase is a handful of words; for a phase of a few hundred a 3x3
does not always land (35 of 100 boards miss after five seedings on the 249-word
proxy, and the 4x4 lands every one), which is reported, not hidden, and is held
by the performance tier's budgets
([Performance design](design/performance.md#the-game-rows)). A board in play may
therefore hold fewer than three or more than five words; it never holds none.

## The build checks

**`platform.fragments`** runs `tests/platform/check_fragments.cmake`. A build
only includes the one `platform.cmake` for the platform it targets, so a typo in
another platform's fragment (a renamed file, a hook that was never defined)
would otherwise be found only by someone on that platform. The check includes
every fragment under `src/platforms/`, each in a process of its own, and holds
it to the contract in `cmake/PlatformContract.cmake`: every variable and hook
defined, `WJS_PLATFORM_DESKTOP` either `ON` or `OFF`, every file it compiles
present. It also checks that `cmake/Platform.cmake` has a branch selecting each
fragment, and that every branch has a fragment. It runs on whatever host runs
the tests, so a Linux machine checks the Windows and macOS fragments.

**`platform.isolation`** runs `tools/checks/check_platform_isolation.ps1`
against the source tree. It is registered only when `pwsh` is found. What it
checks, and why, is in [Platform isolation](platform-isolation.md#enforcement).
You can run it by hand from the repository root:

```sh
pwsh ./tools/checks/check_platform_isolation.ps1
```

**`profiling.shipped_presets`** runs `tests/profiling/check_presets.cmake`. A
profiling build has a Tracy client in it, which opens a TCP port and answers
discovery broadcasts, so nothing that ships may have it. The check resolves
`inherits` the way CMake does and fails if any configure preset other than a
`*-profiling` one turns `WJS_ENABLE_TRACY` on (a release preset, above all), if
the option's default in `cmake/Tracy.cmake` is not `OFF`, if `tracy` is a base
dependency of `vcpkg.json` or `profiling` a default feature, if the top-level
`CMakeLists.txt` requests the feature other than under that option, or if a CI
workflow turns the option on by hand. It reads only the presets' JSON, so it
runs on every host and in every CI leg. [Profiling](profiling.md) is the page
about why.

**`profiling.zone_names`** runs `tests/profiling/check_zone_names.cmake`. A
zone's source location is a function-local static of `zone<"name">()`, so one
name is one place and a name used twice would show the first place's file and
line for both. It scans `src/` and fails on a name that is used twice or is not
`<layer>.<thing>`, lower case and dotted.

## The privacy and parent-area checks

**`privacy.sources`** runs `tests/privacy/check_sources.cmake` over the first-party tree
(`src/`, `cmake/`, `tests/`, the top-level CMake files, any directory that is not
third-party or documentation) and its manifests, for the rules of the first release that
a source scan can hold ([Application design](design/application.md#privacy-built-in-and-checked)):
no audio capture, no network class or header or URL literal or network Qt module or
manifest permission, no export route (printing, sharing, a save dialog, the clipboard,
a URL handed to the system, mail, a launched program), and no file written outside
`src/lib/src/store.cpp`. A comment line is skipped; the checker, its self-test, the
3D prototype (`src/app/prototype`, a program of its own) and the scanner's own privacy
checker and its deliberate leak (`tests/scan`, which name the forbidden things as data) are
not scanned. It is a
pattern scan, not a proof, and Qt Qml links Qt Network itself, so the rule is "no
capability and no use", not "no module". **`privacy.sources.selftest`** holds each rule to
a violation it must find and to the cases it must leave alone, so a pattern that stops
matching is a failure and not a pass. Both are CMake scripts, run on every host with no
pwsh.

**`parent.wording`** holds `src/app/qml/parent/ParentWording.qml`, the one file of
sentences a parent reads, to the design (dashboard.md, "Wording"): the sentences the
design quotes are there, none of a list of words an assessment or a gap is made of
("weak", "mastered", "missed", ...) is in any parent screen, and no screen has a `qsTr`
of its own. **`parent.wording.selftest`** holds the check to cases.

**`parent.screens`** (with `WJS_BUILD_APP`) runs `wjs_parent_preview --check`: the
parent area's screens in a stub shell, drawn by the software renderer under Qt's
offscreen platform (`QT_QPA_PLATFORM=offscreen`; the platform fragment names anything
else the host needs, `WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT`). It drives the gate with
synthetic touches on a clock of its own, walks the dashboard, the settings and the about
page over made-up play, checks what is on screen (the design's sentences, the settings
the table names, nothing of the data at the gate, none of the forbidden words) and that
QML warned of nothing, and with `--out <dir>` saves a picture of each screen. The same
program without `--check` is a window to look at them in.

## Performance tests

`wjs_perf_tests` is a binary of its own, linked against `wjs::lib` and nothing
else (no Qt, no platform layer), so that it can be built and run alone and so
that nothing else's allocator churn is in the way of a stopwatch. It is
modelled on [Cubeulator's](https://github.com/iainchesworthlabs/cubeulator/blob/main/docs/testing.md#performance-tests)
and is the implementation of the tier in the [performance
design](design/performance.md). **Two halves, and the split is the design:**

| | Holds | Exact? | Runs |
| --- | --- | --- | --- |
| **Work budgets** | What the code *does*: visits, words found, the word list's words and trie nodes, verifications and solves spent seeding, comparisons made binding, and a digest of every result | Yes, on every machine and compiler | In every `ctest` of an optimised build |
| **Wall-time checks** | What it *takes*, as a multiple of a calibration workload run in the same process | No, and loose on purpose | Only when asked: `WJS_PERF_TIMING=1`, or a `*-perf` test preset |

```sh
ctest --preset windows-msvc-release -L perf        # the tier, work budgets only
ctest --preset windows-msvc-perf                   # the tier, with the wall-time checks
build/windows-msvc-release/bin/wjs_perf_tests "[.perf-report]"   # what each metric is now
```

(`linux-gcc-` and `macos-clang-` likewise.) Every case carries the `perf`
label. A **Debug or sanitizer build skips** the work budgets (reported as
skipped, not failed: the corpus is much longer there, and the counts are the
same in every build), and a coverage or Tracy build skips the wall-time checks.

### The corpus

The same boards on every machine, compiler and standard library:

- **Random boards:** 200 each of 3x3, 4x4, 5x5 and 6x6. Board `i` of size `n` is
  drawn from `std::mt19937` seeded `7919 * i + n`, one draw a tile in row-major
  order, **taken modulo** (never `std::uniform_int_distribution`, whose algorithm
  is each standard library's own). A draw picks a letter by ENABLE's letter
  counts, in integer arithmetic. It is a test corpus, not a set of dice.
- **Four boards built to hold many words** (4x4, 5x5 and two 6x6) and **two with
  none** (one letter throughout).
- **The standard board** (the default mode): the shipped dice,
  `data/dice/3x3.txt` to `6x6.txt`, rolled with the same seeds as the random
  boards (`7919 * i + n`), 200 a size, each solved against the full word list.
- **The phonics board** (a parent's switch): a test phase of the 249 words of
  three or more letters of ENABLE spelled only with S, A, T, P, I and N (the
  proxy index of [Seeding: what was measured](#seeding-what-was-measured)), built
  by walking the trie; 100 seeds of each size 3x3 to 6x6 with the options of
  `wjs::phonics_board_options`. And, for the binder, three paths of nine tiles
  against a test set of 39 graphemes (31 of more than one letter and not split).
- **The game session** (phonics board): the same test phase, 100 sessions of each
  of 3x3 and 4x4, session `i` seeded `7919 * i + n`, each dealt (`shake`) and
  then played to the resting screen: a find for every word of the board (with
  the help asked for before every other), a word traced again, a touch of one
  tile, a snapshot after the first find, `rest`, and the last snapshot; then the
  first snapshot restored.

### What is counted

| Metric | Budget |
| --- | --- |
| `dictionary.words`, `dictionary.nodes` | Exactly 172,823 and 387,889. They change with the data (`data/README.md`) |
| `solve.visits.max.<n>x<n>` over the 200 boards | A quarter above the measurement: 640, 1,940, 3,580, 6,470 |
| `solve.visits.rich.*` | A quarter above: 3,550, 6,900, 11,800, 15,500 |
| `solve.words.rich.*` | Exactly 1,126, 1,464, 1,788, 327 |
| `solve.visits.empty.*`, `solve.words.empty.*` | Exactly 100 and 256; 0 |
| `solve.digest` | Exactly. An FNV-1a digest of every word, score and path over the whole corpus, in the order `solve()` returns them |
| `roll.visits.max.<n>x<n>` over 200 rolls, **standard board** | A quarter above: 589, 2,050, 4,690, 8,080 |
| The dice digest | Exactly. The same digest over every solution of the rolled boards |
| `seed.tries.total`, `seed.solves.total`, `seed.visits.total`, `seed.visits.max`, `seed.fallbacks` by size, **phonics board** | A quarter above each (docs/design/performance.md has the figures); a 3x3's verifications are the cap of 100 seeds of 20 tries |
| `seed.tries.max.<n>x<n>`, `seed.out_of_range.<n>x<n>`, `seed.phase.words` | At most 20 (the cap), exactly 0, exactly 249 |
| The seeding digest | Exactly. An FNV-1a digest of every board seeded: its tiles, targets, dead jewels, the words found, tries and solves |
| `bind.comparisons.<path>.9`, **phonics board** | A quarter above: 349, 63, 234 for a path of no block, all blocks and a mix |
| `session.seedings`, `.tries`, `.solves`, `.visits` (total and most) and `.out_of_range` by size, **phonics board** | A quarter above each (docs/design/performance.md has the figures); a session seeds again up to its cap of 5 when a board misses three to five words, so these are the price of that guarantee |
| `session.seedings.max.<n>x<n>`, `session.empty.<n>x<n>`, `session.words.total.<n>x<n>` | At most 5 (the cap), exactly 0 (no board is dealt with no word), exactly 522 and 476 (the corpus, the phase and the seeding pinned together) |
| `session.play.solves.<n>x<n>`, `session.play.visits.<n>x<n>`, `session.restore.solves.<n>x<n>` | Exactly 0, 0 and 100: tracing, asking for the help, recording and saving make no solve, and a restore makes one |
| The session digest | Exactly. An FNV-1a digest of every deal (seedings, tries, solves, words, range), the kind of every trace, and the snapshots after the first find and at rest |
| `scan.*` | Reading four fixed synthetic frames: the reads, boards answered and the pixels of the frames and working pictures are exact (they pin the corpus); the warped pixels, blobs, die candidates, cells read, template comparisons and pixel buffers are a quarter above (`wjs/scan/work.hpp`) (reading seeds and solves nothing, so these rows carry the default, standard, board mode) |
| `time.read.4x4`, `time.read.6x6` | Opt-in, as the other wall-time rows: one read of a frame against the heavy calibration, budgets seven to eight times the measurement |

Every metric is for one **board mode**, and the report and a failure say which.
The standard board is the default mode; the phonics board is a parent's switch,
so its rows are not the standard board's work and vice versa. A **verification**
is one solve of a candidate board; a **solve** is any call to `wjs::solve`,
including the guided fill's trial of a letter on a tile; the seeding reports
both (`SeededBoard::tries`, `SeededBoard::solves`) and the performance case
holds `solves` to the solver's own tally. A **comparison** is a grapheme held
against the letters at the start of a block; the binder tallies them in
`wjs/binder_work.hpp` once per bind, and a case recounts them from the blocks
that came back.

A **visit** is a tile step that stayed inside the dictionary's trie: the
letters of the path so far, with the tile's own, still begin some word. The
solver counts them in a member of its search and adds the total to a
thread-local tally (`wjs/solver_work.hpp`) once as the solve ends, so the cost
is one add a solve and nothing per visit; nothing in the application reads it.
`wjs::solve`'s signature is as it was. A separate case holds the solver's tally
to a second walk written out in the test with nothing but the dictionary's
trie, so the budgets are held to a definition and not to an implementation.

Every threshold is in `tests/perf/budgets.hpp`, with the figure it is above
beside it. The measurements and the machine are in the
[performance design](design/performance.md#what-was-measured).

### What a failure means, and how to move a budget

A work failure names the metric, the corpus seed that gave a maximum, and both
numbers: the search did more than it did. Find out why before touching the
number. A budget moves when the work **genuinely** changed (a different move
order, a new dictionary), by a person, with the reason in the commit, and never
to turn a red run green. `solve.digest` is the same: it moves when the solver's
output was meant to change, and the failure says which number to set.

A time failure is the ratio of the task's fastest run to the calibration
workload's fastest run, alternating, so that the machine's speed and a stretch of
load cancel. The budgets are several times what the ratio measures, so they are
for the order-of-magnitude slowdown a count cannot see (an allocation or a copy
per visit). They must never fail because another process was busy; if one does,
the budget is too tight, and that is a finding about the budget. The timing
cases are `RUN_SERIAL`, so `ctest -j` runs them one at a time.

The Linux (GCC) Release build runs the work budgets on every push, in the
`Performance` leg ([CI](ci.md)); the Debug legs prove only that the tier
compiles warning-free.

### Capturing it

A short run of `wjs-cli` is over before a profiler attaches; the tier's binary
runs for seconds, which makes it a good thing to capture:
[Profiling](profiling.md#capturing-a-trace).

## The scanner

`wjs_scan_tests` is a binary of its own, linked against `wjs::scan`, `wjs::lib`,
`wjs::platform` and the synthetic corpus (`wjs_scan_synth`), so that the corpus and its
renderer stay out of `wjs_unit_tests` and the scanner can be built and run alone
([Scanner design](design/scanner.md) is the page about what it holds and why).

| Test file | What it holds down |
| --- | --- |
| `scan_geometry_test.cpp` | Hulls, the four-sided outline of a convex shape (a rounded rectangle reduces to the rectangle's corners; a triangle to nothing), which corner is first, the projective map and its inverse and the cases where none exists |
| `scan_image_test.cpp` | When a frame is valid; luma of every pixel format, in either byte order and with a stride; shrinking, sampling, blurring; the local threshold (a smooth gradient is bright nowhere inside the picture); blobs (4-connected, area, hull, a hole not counting); the buffer count |
| `scan_glyph_test.cpp` | The stroke font (26 letters and Qu, each in its box) and the glyph reader on a drawn die, no board and no perturbation: every glyph at every quarter turn and at awkward angles, the turn found, a lighter or heavier hand, no ink where there is none, a gradient of light |
| `scan_finder_test.cpp` | The board found at its true corners in a frame of each condition; a bright rectangle a die's size and a pitch from the board not taken for part of it; the cells counted from the comb for 4, 5 and 6; what the finder knows narrowing the count; the centres of the dice found from their gaps; which corner is first; nothing found in a blank frame, a gradient or a room with a sheet of paper |
| `scan_reader_test.cpp` | `read_board` end to end: the board that was drawn, the same picture as RGBA and with a padded stride, determinism, a given size, the doubt thresholds, a given glyph height, each error, a lattice of blank dice, and the counts a read keeps |
| `scan_confirmation_test.cpp` | `BoardDraft`: from a reading, the doubted and unfilled tiles in order, a correction kept upper-case, each refused letter, accepting, clearing, a quarter turn each way and four of them, a corrected size, a typed-in grid, `confirm()`; the whole flow from a reading to the words on the board; and `static_assert`s that a `BoardReading` cannot become a `Board` |
| `scan_platform_test.cpp` | The `camera` and `board_reader` interfaces held to their contract with no platform named: not capturing at start, an unlisted camera cannot be started, each error has words, and the portable reader is the library's reader unchanged |
| `scan_corpus_test.cpp` | The accuracy target (below), and a smoke subset |

```sh
ctest --preset windows-msvc-release -R scan              # the scanner's tests, the corpus included
build/windows-msvc-release/bin/wjs_scan_tests "[smoke]"  # the subset that runs in Debug too
build/windows-msvc-release/bin/wjs_scan_tests "[.scan-report]"   # what the corpus measures now
```

**The target** is five numbers a condition (the board found, correct, silent errors,
flagged, offered) over the synthetic **test** split: four conditions of 24 boards each,
and 21 frames with no board. It runs in an optimised build only (the Release `Performance`
leg of CI runs it on every push); a Debug build with the sanitizers **skips** those five
cases, as the performance tier skips, and runs a smoke subset (a board of each condition,
a frame with no board of each kind) so the pipeline runs clean under ASan. The limits are
`kTargets` in `scan_corpus_test.cpp`; they move only with a reason written beside them in
the design note, and never to turn a red run green.

The corpus is the same frames on every machine: `std::mt19937` taken modulo, a seed from
the condition, the split, the size and the index. Two splits: **tuning**, which the
reader is developed against and the doubt thresholds are chosen on, and **test**, which
is held to the target. The hidden cases are the tools (they assert nothing, and read the
tuning split unless `WJS_SCAN_SPLIT=test` is set):

| Case | Prints |
| --- | --- |
| `[.scan-report]` | The five numbers of each condition, and what drawing and reading the frames cost |
| `[.scan-tune]` | For each bound on silent errors, the least doubt that stays under it: where to choose the thresholds |
| `[.scan-confusions]` | Each wrong tile as truth -> read, how often each true letter is right, and why a frame was not read |
| `[.scan-oracle]` | The glyph step alone, given the true corners and centres: what it gets right when the geometry is perfect |
| `[.scan-negatives]` | How many frames with no board were answered with an error |
| `[.recorded]` | The same numbers over real frames kept outside the repository, from `WJS_SCAN_RECORDED_DIR` (grey PGMs with a `.txt` truth beside each). Skips when unset, and **has not been run on anything** |

`wjs_scan_render --out <directory>` writes the corpus as PNG files with the truth beside
each, and `--sheet <file.png>` a contact sheet, to look at. **Recorded camera frames are
never committed** (`.gitignore`, [Licensing](licensing.md#camera-input-and-letter-recognition)).

**`scan.privacy`** runs `tests/scan/check_privacy.cmake`: the scanner's sources and the
camera and `board_reader` files of every platform may not include a file or stream
header, use a file class or API, capture audio or use a network class (a frame is never
written, nothing is recorded, nothing is sent). `scan.privacy.catches_a_leak` runs it on a
fixture tree whose one file opens an `ofstream` and expects it to fail. Both only read
source, so they run on every host.

## Phonics data: the checker and the review gate

A phonics phase is two files in `data/phonics/`: `phase-<x>.graphemes.txt` (a
grapheme, its sound, its clip id) and `phase-<x>.words.txt` (a word, split into
graphemes, or a word marked `tricky`). **There is no phase file in the
repository.** The content has to be written and reviewed by a person qualified
to do it, and nothing here writes any; every word, grapheme and sound in the
tests is marked as test data and written inside the test. The format and the
reader are in [Architecture](architecture.md#phase-data).

```text
# phase: A
# reviewed-by: <name>   reviewed-on: <date>   source: <where it came from>
ai  /sound/  clip:ai        <- a graphemes file line
pain  p ai n                <- a words file line
cake  c a_ k _e             <- a split digraph, the grapheme a_e
snow  s n ow:2              <- the second sound of a grapheme with two
```

**`data.phonics`** runs `tools/data/check_phonics.ps1` over the tree. It holds
the files to the checks the phonics design lists, and to the same ones the
library's `PhaseSet` applies, so a file that passes loads: the file name and
the header's phase agree; the line grammar; a word's graphemes, joined, spell
it; no word, and no grapheme with the same sound, twice, in a file or across
phases; sorted (byte order); every word in `data/dictionaries/en.txt`; every
grapheme is in the word's phase or an earlier one, and one with several sounds
there is told which. With no `data/phonics` there is nothing to check and it
passes.

**`data.phonics.checker`** runs `tools/data/test_check_phonics.ps1`, which builds
throw-away data and holds the checker to a table of 55 cases (the ones in
`phase_data_test.cpp`, so the two implementations agree). A gate that passes
everything is worse than none, so the gate is itself tested; it takes half a
minute because each case starts a PowerShell.

**The review gate.** A file with no `reviewed-by`, `reviewed-on` (a date,
`YYYY-MM-DD`) and `source` is test data. Test data need not say where it came
from; a file that claims a review has to, as `data/README.md` asks of every
list. `WJS_REQUIRE_REVIEWED_PHONICS` makes the
build refuse it: with the switch on, a build step runs
`check_phonics.ps1 -RequireReviewed` before the library is built, every time, so
a file edited after configuring is still caught, and `data.phonics` passes
`-RequireReviewed` too. The switch is on by default for `Release`,
`RelWithDebInfo` and `MinSizeRel` and the release presets set it. It needs
PowerShell 7: if the switch is on and `pwsh` is not found, configuring stops and
says so, rather than building something unchecked. Turning it off is an explicit
`-DWJS_REQUIRE_REVIEWED_PHONICS=OFF`, which is on the command line of whoever
did it. A half-given review (a reviewer and no date, or a review and no source)
and a date that is not a date are problems, not passes. What a reviewer attests is theirs; the gate only
makes it impossible to ship a file that does not say so.

## Seeding: what was measured

`seed_board` is tested for what it guarantees (above). How often it lands a
board in range depends on the size of the phase's index and on the dead jewels,
and the repository has no phase, so this was measured, once, with a throw-away
program that is not committed, on proxy indexes made from the dictionary: the
words of three or more letters spelled only with S, A, T, P, I and N (249 of
them), those that do not end in S (122) and those of three or four letters that
do not end in S (68). The proxies are sizes, not a proposed index. One Windows
11 desktop, a Release build (MSVC 19.51), 1,000 seeds a row, a phase alphabet
filler and no focus sounds. "Landed" is a board of three to five words found
within 20 verifications; the rest are fallbacks.

| Index | Board | Dead jewels | Landed | Mean verifications | Mean time |
| --- | --- | --- | --- | --- | --- |
| 249 | 3x3 | 0 / 3 / 4 / **5** | 0.4% / 29% / 78% / **100%** | 20.0 / 17.1 / 10.5 / 2.4 | 0.9 / 2.0 / 1.3 / 0.4 ms |
| 249 | 4x4 | 0 / 8 / 9 / 10 / 11 / **12** | 0% / 8% / 27% / 57% / 91% / **100%** | 20.0 / 19.2 / 17.3 / 14.0 / 7.9 / 2.2 | 2.5 / 2.2 / 3.3 / 6.6 / 3.7 / 0.8 ms |
| 122 | 3x3 | 0 / 3 / 4 / **5** | 4% / 61% / 97% / **100%** | 19.6 / 13.1 / 5.9 / 2.0 | 0.7 / 1.2 / 0.7 / 0.2 ms |
| 122 | 4x4 | 0 / 8 / 9 / 10 / 11 / **12** | 0.1% / 34% / 65% / 89% / 99.5% / **100%** | 20.0 / 16.4 / 12.8 / 8.4 / 4.2 / 1.9 | 2.1 / 1.3 / 1.7 / 3.1 / 1.7 / 0.5 ms |
| 68 | 3x3 | 0 / 3 / 4 / **5** | 6% / 65% / 98% / **100%** | 19.4 / 12.7 / 5.7 / 2.0 | 0.5 / 0.2 / 0.1 / 0.1 ms |
| 68 | 4x4 | 0 / 8 / 9 / 10 / 11 / **12** | 0% / 53% / 85% / 93% / 99.7% / **100%** | 20.0 / 14.3 / 9.7 / 7.4 / 4.2 / 1.9 | 0.9 / 0.3 / 0.2 / 0.2 / 0.1 / 0.1 ms |

What it says, and what it does not:

- **The number of dead jewels is the lever, and the best share leaves four
  letters.** A 3x3 with 5 dead jewels and a 4x4 with 12 land every time, but they
  are boards of four letters in a field of dead jewels. With no dead jewel a
  board lands 6% of the time at best: a full board holds 10 to 19 words of these
  indexes on a 3x3 and 18 to 42 on a 4x4, on average. Whether a board that is
  mostly dead jewels is acceptable is the literacy reviewer's question (the
  design lists it), and the index size decides how many are needed. With a
  smaller index (the 7 and 11 words of the tests) nearly every board lands at
  most settings, and a 3x3 with 6 or more dead jewels never does, because the
  three tiles left hold one word; that is a fallback every time, and it is
  reported as one.
- **Planting helps where it was expected to and not everywhere.** The design's
  blind draws, with nothing planted, landed 25% to 30% of boards at their best
  dead count (3 on a 3x3, 8 to 9 on a 4x4). A planted board at those counts lands
  2% to 9% of verifications, depending on the index: it always holds its targets
  and the letters round them add words. Where the dead jewels leave about four
  letters it lands 40% to 50% of verifications. The planting was "not measured" in the
  design; this is that measurement.
- **Changing one filler tile at a time did not help.** With 0, 1 and 4 changes
  before planting again the share moved a few points either way and was best, or
  equal, with none. The default is none: each try plants again.
- **Cost.** A verification, with the planting, is 0.02 to 0.5 ms on this
  machine (the design estimated 0.5). A seed takes a mean of 0.06 to 6.6 ms in
  the rows above, and the worst single seed took 25 ms (249 words, 4x4, 10 dead
  jewels, 20 verifications). The performance design's budget is 10 ms typical
  and 50 ms worst on the lowest device, with a tablet at three to five times a
  desktop core (its estimate): the worst case would be over it there.
- **Not measured**: seeding with a real index (none exists), with focus sounds on
  a real phase, with recent targets over a session, on a tablet, or in Debug.

### Fuller boards: a guided fill and short targets

The result above leaves a board with four letters in a field of dead jewels,
which is not a board anyone would play. Two options exist to do better, and
both were measured on the same proxies (400 seeds a row, Release, no dead
jewels asked for, 200 seeds for 5x5 and 6x6):

- `guided_fill` gives each tile left after planting a letter chosen at random
  from those that keep the board's count under a ceiling (drawn between the
  words already planted and the most wanted), tried tile by tile with the
  solver; a tile no letter fits is made a dead jewel. The planted words are
  also kept apart where the board has room.
- `max_target_letters` limits the targets to words of that many letters or
  fewer. Longer words are still words of the phase and still count if found.

**The fill alone did nothing.** It cannot take a word away, and the words
planted already hold too many: a planted word of four or more letters brings its
shorter words and its inflections with it, so three targets of an index of
hundreds hold more than five words before any filler is placed. What moved the
result was the targets, three letters at most, and keeping them apart.

| Index | Board | Landed, targets of 3 letters | Dead jewels (of the tiles) | Mean time |
| --- | --- | --- | --- | --- |
| 249 | 3x3 | 23% | 2.6 of 9 | 0.6 ms |
| 249 | 4x4 | 92% | 4.4 of 16 | 0.6 ms |
| 249 | 5x5 | 100% | 7.0 of 25 | 1.0 ms |
| 249 | 6x6 | 100% | 9.6 of 36 | 2.1 ms |
| 122 | 3x3 / 4x4 / 5x5 / 6x6 | 75% / 90% / 100% / 91% | 1.7 / 3.4 / 5.7 / 9.0 | 0.3 / 0.6 / 1.2 / 2.4 ms |
| 68 | 3x3 / 4x4 / 5x5 / 6x6 | 56% / 90% / 100% / 91% | 1.9 / 3.3 / 5.5 / 8.7 | 0.4 / 0.5 / 0.9 / 1.8 ms |

With targets of four letters the 4x4 lands 19% to 50% of the time, and with no
limit 2% to 48%; the random fill, with any limit, landed 0% on a 4x4 or larger.
So: **boards of about three quarters letters at 4x4 and larger, a few dead
jewels, 90% or more of seeds in range**, at the price of planting only 3-letter
words (what the design calls Spark words). The 3x3 stays crowded (23% to 75%).
A seed that does not land is reported as a fallback, as before.

Put letters outside the phase in the filler (the design's "letters not yet
taught") and a tile always has a letter that forms no word of the phase, so
nothing is made dead at all; the board then holds untaught letters. The fill
takes a letter of the phase when one fits and an outside letter only when none
does, so the phase's letters stay the most of the board.

**The phonics board as decided** (the author, 2026-10-16: targets of three
letters, and untaught letters acceptable) is `phonics_board_options(index,
size)`: those two options with the whole alphabet as filler, less the first
letter the phase does not use, which is the dead letter. Measured on the same
proxies, 400 seeds (200 for 5x5 and 6x6):

| Index | 3x3 | 4x4 | 5x5 | 6x6 |
| --- | --- | --- | --- | --- |
| Landed, 249 | 23% | 92% | 100% | 100% |
| Landed, 122 | 75% | 90% | 100% | 91% |
| Landed, 68 | 56% | 90% | 100% | 91% |
| Tiles that are letters of the phase | 72% to 82% | 72% to 79% | 72% to 78% | 74% to 76% |
| Dead jewels on a board that landed | 0 | 0 | 0 | 0 |
| Mean time, 249 / 122 / 68 | 0.7 / 0.5 / 0.4 ms | 1.4 / 1.3 / 1.2 ms | 3.0 / 3.3 / 3.0 ms | 6.7 / 6.9 / 5.9 ms |

The time is higher than the table above because there are 25 letters to try at
each tile, not six. A seed that does not land is a fallback and its tiles are
dead jewels, as before.

What it does not settle: the proxies are sizes, not a proposed index; and a
tablet, where the 5x5 and 6x6 times are three to five times these.

## The dice: what was measured

The project's dice (`data/dice`) were checked by rolling each size 1,000 times
(seeds 1 to 1,000) and solving every board against the dictionary, once, with a
throw-away program; `dice_data_test.cpp` holds the same to loose bounds on 300
boards a size. One Windows 11 desktop, Release.

| Board | Words a board holds, mean | 10th / 50th / 90th percentile | Boards with no word | Vowels (A E I O U) among tiles |
| --- | --- | --- | --- | --- |
| 3x3 | 30 | 7 / 26 / 58 | 3.3% | 30% |
| 4x4 | 105 | 41 / 98 / 182 | 0.3% | 29% |
| 5x5 | 254 | 137 / 243 / 389 | 0% | 40% |
| 6x6 | 471 | 291 / 458 / 669 | 0% | 38% |

These are about what the design measured for boards of letters drawn by the
dictionary's letter counts (31, 120, 254 and 435), which the dice are made from.
A `QU` tile did not appear in 1,000 boards of any size: with six different
letters dealt by count, Q is dealt about once in 1,000 faces, so no set of the
four has one. The model takes a `QU` face and the tool shows a Q as one; whether
a set should be made to carry one is a rule the tool does not yet have.

## Sanitizers, warnings and Debug

Debug builds run the tests under AddressSanitizer (and UBSan on GCC and Clang),
and every first-party target is built with warnings as errors. A sanitizer
finding aborts the test. Details, and how to turn them off, are in
[Building](building.md#sanitizers-and-warnings).

## Coverage

The `linux-gcc-coverage` preset builds the library, the platform layer and the
tests with gcov instrumentation and the sanitizers off. Run `ctest` against it
and the `.gcda` files hold what was executed. The report tool and the threshold
(if CI enforces one) are described in [CI](ci.md); this repository does not
fix a number here.

## Adding a test

- Put the file in `tests/unit/` (library and platform interface), `tests/scan/`
  (the scanner and its interfaces), `tests/app/`
  (the window's model) or `tests/platform/` (a case only one platform can run),
  and add it to the executable's source list in `tests/CMakeLists.txt`. The
  lists are explicit, not globbed.
- Name each `TEST_CASE` as a sentence that says what is true, and give it a
  tag. The name is what CTest shows.
- A test that needs a file writes it to the system's temporary directory and
  removes it, as `dictionary_test.cpp` does. No test reads a real word list
  but the ones about the shipped data themselves (`[data]`, and the performance
  tier): the rest build their dictionaries from a few words in the test itself.
- A test must not need a camera, a network or a word list that is not in the
  tree.
- A change in behaviour comes with the test that shows it; a bug fix with the
  test that failed.

## What is not tested

- **`wjs-cli` itself.** Nothing runs the program: option parsing, the exit
  codes and the output format are checked by hand. The library behind it is
  tested, and the README example is the solver case above.
- **The screens.** The models, the layout rule, the touch logic and the fake are
  tested, and the QML is linted with no warning allowed. No test loads the QML or
  sends it input: the screens were run on a Windows desktop and driven with the
  mouse and the keyboard by hand, at four window shapes, and that is all. Nothing
  was run on a touch screen, a tablet or a phone, or on Linux or macOS.
- **The scanner on anything real.** No frame from a camera, no real dice, no iPad,
  no light that was not drawn: every number is on synthetic frames
  ([Scanner design](design/scanner.md#what-this-does-not-prove)). The iOS camera and
  reader do not exist.
- **The console code on Linux and macOS.** It does nothing there (a POSIX
  terminal already carries UTF-8), so there is nothing to observe. The path code
  is covered by `paths_test` in CI on both.
- **A UTF-8 console on Windows**, and files opened by paths longer than
  `MAX_PATH` (a [known problem](building.md#platform-notes)).
- **Performance of the application.** The tier holds the solver and the word
  list. It is not the frame time of the window, a launch, or anything on a
  device, and nothing has been measured on a tablet or a phone
  ([performance design](design/performance.md)). Boards larger than 6x6 are not
  in the corpus. Seeding is held by the unit tests, and its work (verifications,
  solves, visits, fallbacks) by the performance tier on a proxy phase of 249
  words; its milliseconds were measured by a program that is not committed.
- **Real word lists.** The tests use made-up lists of a dozen words. The
  behaviour on a list of a few hundred thousand words has not been checked by
  a test.
- **Real phonics data.** There is none. The reader, the checker and the seeding
  are tested on a dozen words of test data written in the tests, with
  placeholder sounds. That the data is right, that its segmentation is right and
  that its words suit a child are for a literacy reviewer, not a test.
- **The shake on a device.** The detector is tested on synthetic traces: nothing
  here is a recording from a device, and its thresholds are starting values that
  have to be tuned on one.
- **Apple clang's boards.** `seeding_test` pins two boards, which is how a
  difference between standard libraries would show. They were made on MSVC and
  come out the same on GCC 15.2 with libstdc++ (run in a container, in Debug and
  Release); Apple clang with libc++ has not been run.
- **Threads.** The library keeps no shared state between calls, but nothing
  tests solving from several threads at once.
