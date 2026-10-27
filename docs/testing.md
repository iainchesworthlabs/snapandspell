# Testing

The tests are four tiers - unit, integration, system and UI - with a quality
check beside them ([The tiers](#the-tiers)): Catch2 unit tests for the library,
the platform layer and (when the Qt window is built) its model, the scanner with
a synthetic corpus, the programs themselves in the system tier, a performance
tier with its own binary, and checks about the build, the data, the notices and
the documents themselves (one of them holds [the requirements
matrix](#the-requirements-matrix) to the tests that exist). When the window is
built there is also a Qt Quick Test tier that drives the kit and the screens
themselves, headless ([The interactive
QML](#the-interactive-qml-qt-quick-test)). The library, the scanner, the platform
layer and the command line are gated per component on the unit, integration and
system tiers together; the window is not ([Coverage](#coverage)). They run in CI on Windows (MSVC), Linux
(GCC) and macOS (AppleClang). The exact number differs by leg: the Qt model,
screen, language and Quick Test suites run only where the window is built
(Windows so far), and the 32 performance work cases and the five scanner
accuracy cases skip themselves in a Debug build, which is every required leg
(see [Performance tests](#performance-tests) and [The scanner](#the-scanner)).
What the tests do not show, requirement by requirement, is in
[Requirements and traceability](requirements.md) and, in short, in [What is not
tested](#what-is-not-tested).

## Running them

After a build (see [Building](building.md)):

```sh
ctest --preset windows-msvc-debug              # or linux-gcc-debug, macos-clang-debug
ctest --preset windows-msvc-debug -R "^platform[.]"      # the two build checks
ctest --preset windows-msvc-debug -R "^data[.]"          # the word-list, dice, sound and phonics checks
ctest --preset windows-msvc-debug -R "^requirements[.]"  # the requirements matrix
ctest --preset windows-msvc-debug -R "^quick[.]"         # the interactive QML (Qt Quick Test) and its lint
ctest --preset windows-msvc-debug -L camera              # Mode A's camera, without a camera
```

Every Catch2 `TEST_CASE` is registered with CTest by name
(`catch_discover_tests`), so a failing case is named in the CTest output, and
`ctest --preset <preset>` shows the output of any that fail. To run the
unit-test program directly, with Catch2's own filters:

```sh
build/windows-msvc-debug/bin/sns_unit_tests --list-tests
build/windows-msvc-debug/bin/sns_unit_tests "[solver]"
```

The scanner's tests are their own binary, `sns_scan_tests`, tagged `[scan]` (with
`[geometry]`, `[image]`, `[finder]`, `[font]`, `[glyph]`, `[reader]`, `[draft]`,
`[platform]` (with `[contract]`), `[corpus]` and `[smoke]`); see [The scanner](#the-scanner).

The tags are `[board]`, `[dictionary]`, `[scoring]`, `[solver]`, `[path]`, `[dice]`
(and `[data]` for the cases that read the shipped data),
`[phase]` (and `[review]` for the review header), `[seeding]`, `[binder]`,
`[shake]`, `[rules]`, `[session]` (with `[standard]` for the cases that play the standard board), `[deal]`, `[dice_sets]`, `[platform]`, `[feedback]`, `[motion]` (with `[processor]` for the pure processing) and `[haptics]`, and `[profiling]`, and `[app]` in
`sns_app_tests` (with `[layout]`, `[trace]`, `[fake]`, `[solo]`, `[feedback]`, `[motion]` and `[i18n]`,
the last with `[catalogue]`, `[language]` and `[glyph]`). The camera's are `[camera]` (with
`[scan]`, and `[app]` in `sns_app_tests`), and `camera.screens` is the QML check that
drives the live view. The parent area's are
`[calendar]`, `[settings]`, `[gate]`, `[store]` (with `[record]`, `[file]`, `[migration]`, `[bounds]`,
`[reset]`, `[resume]` and `[privacy]`), `[dashboard]` (with `[sounds]`, `[week]`, `[vault]`, `[best]`,
`[focus]`), `[session-store]` (the copy to and from the game session) and, in `sns_app_tests`, `[parent]`
(and `[system]` with `[motion]` for the singleton over the system's motion preference, which makes a
GUI application on Qt's offscreen platform and tells its accessibility hints of a change).
The rest: `[sound_it_out]` and `[version]` in `sns_unit_tests`; `[golden]` in the store's cases (a
file of known data, byte for byte); `[assist]`, `[startup]`, `[text]`, `[sequence]` (with `[feedback]`),
`[clock]` (with `[solo]`, the board clock), `[edited]`, `[real]`, `[totals]` and `[backend]` in
`sns_app_tests`; `[scene]` (with `[profiler]` and `[perf]`) in `sns_scene_script_tests`;
and `[frame]` in `sns_frame_profiler_tests` and `sns_frame_timing_tests`.

## The tiers

The suite is four tiers and a quality check, each a CTest selection. The tiers
are what the project means by a "unit", "integration", "system" or "UI" test;
they are not separate builds, and a full `ctest` runs all of them.

| Tier | Selected by | What it is | Where |
| --- | --- | --- | --- |
| **Unit** | `ctest -L unit` | One class or one function, with no file, no Qt and no process around it: the library's cases and the platform layer's. The only tier with a coverage floor (90% lines / 80% branches, [Coverage](#coverage)). | `libs/core/tests/unit/` |
| **Integration** | `ctest -R "^integration\\."` | Several first-party pieces together: the window's model over the real library and store, the scanner's pipeline over its synthetic corpus, and the board scene's script in Qt's engine. No program is started. | `apps/app/tests/unit/`, `libs/scan/tests/` |
| **System** | `ctest -L system` | The programs as programs: `sns-cli`'s options, exit codes and output; the application's start-up contract and headless smoke run; the install rules run into a throw-away prefix and the installed `sns-cli` then executed; the two programs run with the Tracy capture attached (`profiling.trace_cli`, `profiling.trace_app`, [Performance tests](#the-trace-tests)); the window's frame budget (`system.frame.*`, opt-in); and the two stub shells walking every screen. | `tests/system/`, `tests/checks/`, `apps/app/tests/unit/*_ui/`, `apps/app/tests/unit/frame_timing_test.cpp` |
| **UI** | `ctest -L ui` (or `-R ^quick`) | The application's own screens and kit driven at run time by Qt Quick Test, and the two stub shells' screen walks. `coverage-ui` shows which C++ the QML reaches ([Coverage](#coverage)). | `apps/app/ui-tests/`, `apps/app/tests/unit/parent_ui/`, `apps/app/tests/unit/scan_ui/` |

The performance tier (`ctest -L perf`, [Performance tests](#performance-tests)) is
selected by its label and sits beside these four: its cases are named `perf.*`.

`ctest -L quality` is the tree's own checks rather than a tier of behaviour: the
word lists, dice and sounds, the notices, the package and install rules, the
platform fragments and isolation, the privacy rules, the presets, the toolchain
pins and the version, the interface catalogues, the QML lint and the requirements
matrix; every one is listed in [The build checks](#the-build-checks). They read
files and need no display, so most run on every host; the ones that need
PowerShell, the Linguist tools, a Tracy build or the window report themselves
skipped where those are missing.

Test presets make the tiers one word, e.g.:

```sh
ctest --preset windows-msvc-debug-unit
ctest --preset windows-msvc-debug-integration
ctest --preset windows-msvc-debug-system
ctest --preset windows-msvc-debug-ui
```

(`linux-gcc-debug-` and `macos-clang-debug-` likewise; `ctest -L <tier>` works
without a preset and is what the coverage targets use.) The integration tier is
selected by its test names, not a label, because Catch2's tags are already the
app and scanner suites' CTest labels (`camera`, `app`, `solo`, ...).

## What there is

| Test file | Tests | What it holds down |
| --- | --- | --- |
| `libs/core/tests/unit/board_test.cpp` | `sns::Board::parse` | One letter per tile or separated tiles (`Qu I T`), `/` and `\r\n` as row breaks, each way a board is refused (empty, ragged, bad tile, over 16 by 16), `contains` |
| `libs/core/tests/unit/dictionary_test.cpp` | `sns::Dictionary` | Case-insensitivity, prefix versus word, words refused (apostrophe, hyphen, digit, empty), duplicates counted once, stepping through the trie, loading skips blanks and non-words, a missing file is an error value, a file round trip |
| `libs/core/tests/unit/scoring_test.cpp` | `sns::score_word`, `sns::word_points` | The printed Boggle scores from 2 letters to 12, and the letter-rarity scores for `E`, `Z`, `EZ`; the points a child sees (`word_points`): a base for the length (none under three letters), each letter at its English Scrabble value in either case, a longer word always worth more than a shorter one of the same letters (lengths 3 to 16), a non-letter counting in the length and adding no value; standard Boggle scores in either case |
| `libs/core/tests/unit/solver_test.cpp` | `sns::solve` | Adjacency, the minimum length, the path reported for a word, a tile used at most once, a word reached by several paths reported once, the `Qu` tile, the scoring scheme applied, an empty dictionary; the order of the answer (highest score, then longer, then alphabetical, in both schemes); a tile of three letters spending all three on one visit; a word using every tile of a 4x4 board; and the work counter (`sns/solver_work.hpp`): visits, solves and the largest, reset, and that each thread has its own |
| `libs/core/tests/unit/path_test.cpp` | `sns::validate_path`, `sns::are_neighbours` | The word a path spells (a `Qu` tile adds two letters), each fault (off the board, a tile twice, tiles that do not touch, no tiles), no wrap-around from the end of a row, and that every path the solver reports validates |
| `libs/core/tests/unit/phase_data_test.cpp` | `sns::parse_words`, `parse_graphemes`, `PhaseSet` | The two file formats, the review header (all three on one line or one to a line, placeholders, a half-given or malformed review), each line problem with its line number, the cumulative phase index, each cross-file check, `reviewed()` and the files it names, loading a directory |
| `libs/core/tests/unit/seeding_test.cpp` | `sns::seed_board`, `RecentTargets` | The same inputs give the same board (and two boards are pinned, so a change of compiler or algorithm shows), a range of three to five words unless the board says it fell back, targets that are words of the phase and are on the board, dead jewels, recent targets, the cap on tries, **focus sounds steering the targets** (targets prefer words that have a ticked sound; when too few do, the rest come from the phase and the board says `focus_unmet`; a sound is ticked by its letters in any case and not by its slashed sound; every sound the dashboard offers is one seeding honours), each refused option, variety over 300 seeds; the target-length limit, and the guided fill (count kept in range, dead jewels listed as they are on the board, a function of the seed, none with letters outside the phase) |
| `libs/core/tests/unit/grapheme_binder_test.cpp` | `sns::GraphemeBinder` | The brief's `R [AI] N`, order, longest first, greedy from the left, blocks that are whole tiles (`Qu`), single letters ignored, a block growing as the path does, every path of up to four tiles covered in order |
| `libs/core/tests/unit/shake_test.cpp` | `sns::detect_shakes` | Synthetic traces: a shake, one still, a gentle wave, walking, a knock, turning over, a slow tilt, two shakes, a pause, sample rates and jitter, units, a harder threshold, damaged samples |
| `libs/core/tests/unit/motion_test.cpp` | `sns::MotionProcessor` | Samples in, the scene's numbers out: metres per second squared become g, the gyroscope passes through unchanged, and a non-finite sample or one whose time goes backwards is dropped rather than reaching the dice. The same synthetic traces as the detector's: a device lying still is not shaken; a vigorous shake is reported **once**, on the sample that completes it; two shakes are two events; `reset()` forgets the first. No sensor and no hardware |
| `libs/core/tests/unit/feedback_test.cpp` | `sns::plan_cue` | The cue policy of the design's feedback table: every cue names its asset, the master Sound switch silences assets and speech, Celebration sounds off stops the cheer/chime/cleared notes but not the word, quiet lowers them, Sound it out full/word/off decides whether the whole word is said (and full is the seam for the sequence), a real word that is not today's is a soft tick and nothing else, Vibration is its own switch, and every haptic is dropped when it is off. No speaker, no sensor, no Qt |
| `libs/core/tests/unit/haptics_test.cpp` | `sns::platform::vibrate` | Every pattern is a call the platform answers, twice, and the three desktops answer false through the shared no-op (`[platform][haptics]`) |
| `libs/core/tests/unit/sound_it_out_test.cpp` | `sns::sound_it_out_sequence` | The sequence of steps for a found word, as data and with no clip and no speaker (`[sound_it_out]`): one sound step for each grapheme in the word's order, 150 ms apart with the first at once, then a blend of 380 ms that plays every sound again, then the word said whole, then the cheer (which names the sound of the first grapheme of more than one letter, and none for a word of single letters); a split digraph (`cake`) one block at the place of its first letter; a letter with two sounds (`o:2`) sounded as the word uses it; a tricky word said whole and never sounded out; `word_only` is the word and nothing else; `off` is no steps, for a tricky word too; a word the phase does not hold is told and not sounded; the same word and setting the same sequence; the time the sequence fixes is its gaps and its blend; every step kind has a name |
| `libs/core/tests/unit/version_test.cpp` | `sns/version.hpp` | The version block names the version and the build's provenance (`[version]`) |
| `libs/core/tests/unit/game_rules_test.cpp` | `sns::classify_path`, `viable_words`, `tier_for_letters`, `reward_for`, `FoundWords`, `start_tile_help`, `is_cleared` | The three states on each word set (the standard board's has no middle state; the phonics board's holds only the index, and a real word outside it is "real, not today's" with no word handed back), a word under three letters never counting, each path fault by name, a `Qu` tile adding two letters (QUIT is Power), every one of the 10,305 paths of a 3 by 3 board against the solver on both word sets; the tier table and rewards; a word found once whatever its case and kept in order; the help (shortest in letters, then nearest the top left by distance and not by reading order, then alphabetical, the same in all 120 orders of its input, unmoved while other words are found, and following it finds every word and clears the board); cleared, and the empty board. **The accepted risk, pinned on purpose** (`[rules][data]`): `a word a child must never meet is counted by the standard board and not by the phonics`. Eight words stand for it, kept out of the repository as FNV-1a hashes and found in the shipped ENABLE file by hash; each is laid on a 3x3 board and is a find on the standard board (counted, named) and a real word that is not today's, with no word handed back, on the phonics board. The standard board has no block list ([D7](roadmap.md#4-decisions-that-gate-the-plan)), so lifting the risk is a deliberate edit of this case, not a side effect of a refactor |
| `libs/core/tests/unit/deal_test.cpp` | `sns::deal_playable`, `deal_standard_board`, `next_deal_seed` | With stub boards written in the test: the same inputs deal the same board; a board with a word is dealt in one roll; an empty board is made again from the next seed (the maker is asked for exactly the two seeds); the viable list is the board's own solve; `min_viable` of 2 is kept; the cap on rolls (counted by the maker) and a board that comes on the second not being dealt at a cap of one; each refused option, a missing maker and a maker that fails; only the active set's words count; `next_deal_seed` is `std::mt19937`'s first output; each error's name. With the shipped 3x3 dice and dictionary: none of 2,000 deals has an empty viable list (and some were made again), two seeds are pinned, one of them dealt in two rolls, and dice that make no board are `bad_board` |
| `libs/core/tests/unit/game_session_test.cpp` | `sns::GameSession` | Every edge of the flow and every event refused in every other state (a table of nine events by six states, a refused event changing nothing); the three kinds of trace, found again, and an ignored one; stars by tier on a board with words of three, four and five letters; clearing, the celebration, resting and again (these are the session's states; the window shows no resting screen, a cleared board waits on a summary sheet, [D12](roadmap.md#4-decisions-that-gate-the-plan)); the help (the same tile until its word is found, an assisted find, off at the parent's switch, followed to the end of the board); the deal (the same seed gives the same boards and outcomes, different seeds differ, three to five words over 600 seeds, never none, the range and the seedings the options', the focus sounds reaching the seeding, dead jewels); the record, and a snapshot restored in each state and refused in each way; and what it never says: every one of the 10,296 paths of two or more tiles on a 3 by 3 board answered as the two word lists say, and a word only in the dictionary never handed out. **The standard board** (`[standard]`, the session's default mode): the mode and the data each needs (the dice and no phase, the phase and no dice, an app that holds both); each mode refusing the options it cannot play; the sizes 3x3 to 6x6 chosen in the box and the phonics board refusing 5x5 and 6x6; a board dealt at every size with no dead jewel, one solve a roll, the report's counts held to the solver, and the library's own deal seed for seed; a roll that holds no word rolled again and never dealt, and no board when no roll holds a word; the same seed the same boards and one seed a run of them; every dictionary word of three letters or more a find (the same trace a tick on the phonics board and a find on the standard), the tiers and stars, a two-letter real word a tick, the board cleared by every word and the help following all twelve of them to the end; the record (the mode, no phase, the count of the words and none listed) and a snapshot restored in each state and refused in each way (the other mode, a record and a snapshot of two modes, a list or no count of the words, a count that is not the board's, a find that is not a word that counts); a tile of two letters; every path of the 3 by 3 board answered as the dictionary says; and, from the shipped dice and the full dictionary (`[data]`), a board that holds a word at every size, the deals pinned to the library's for two seeds, and a whole board played to its clearing |
| `libs/core/tests/unit/profiling_test.cpp` | `sns/profiling.hpp` | That the variant CMake chose is the one it was asked for, that every call can be made with no profiler connected, and that a zone cannot be copied or moved |
| `libs/core/tests/unit/dice_test.cpp` | `sns::DiceSet`, `sns::roll`, `sns::roll_with_placements` | The file format (a face of several letters, commas, comments, a byte-order mark), each way a set is refused, a roll using each die once and showing one of its faces, a function of the seed, two boards pinned so a change of compiler shows; the placements: the die and face of each tile are the tile's letters, each die used once, `roll` the same board as `roll_with_placements` for 200 seeds, the same two boards pinned through it, and the placements of two seeds pinned; every face shown over many rolls |
| `libs/core/tests/unit/dice_sets_test.cpp` | `sns::DiceSets` | A set for each side from 3 to 6 in any order, `find` for each and null for any other side; a side missing, twice or out of range refused as `wrong_count` and a set that is not a square as `not_square`; the project's `data/dice` directory read as the four sets, and a directory without them unreadable |
| `libs/core/tests/unit/dice_data_test.cpp` | The shipped dice, `data/dice` | Each size has the dice for its board and six different faces to a die with a vowel among them; for each size, 300 boards whose placements name, for every tile, the face of its die that is the tile's letters, with each die used once; the board the deal keeps is the roll of the seed it reports, die for die (so a scene is given the dice of a dealt board by `roll_with_placements(dice, dealt.seed)`); 300 boards a size rolled and solved against the dictionary hold about the words a random board does; two boards pinned, through `roll` and through `roll_with_placements`. Reads the shipped files by their path in the checkout, as the performance tier does |
| `libs/core/tests/unit/paths_test.cpp` | `sns::platform::executable_path`, `user_data_directory` | That the first names the running test program: absolute, an existing file, called `sns_unit_tests`; that the second is an absolute path and is the same each time |
| `libs/core/tests/unit/calendar_test.cpp` | `sns/calendar.hpp` | A date as a day (against dates counted by an independent program), the ISO weekday, the week from any first day of the week, over 800 days for each |
| `libs/core/tests/unit/settings_test.cpp` | `sns::Settings` | The defaults are the table's (the board clock is off and the help is the last of its three steps); every value a screen offers survives; any other becomes the default for it; names, sizes, text scales and focus sounds are brought within their bounds; the board clock is off, up or down and its minutes one of the choices; the help is off, the start tile, or the start tile and two letters |
| `libs/core/tests/unit/parent_gate_test.cpp` | `sns::Gate` | The gate's logic, which is still **two touches on two spots** held for two seconds: the layout on every screen shape; two touches, one on each spot, held two seconds open it, anything else does not (one touch, two on one spot, a third touch, a slide out, a lift and a press again); the accessibility action; and a property test, 400 random runs of touches, against an oracle written separately. What the child or parent meets is **one press-and-hold** ([D9](roadmap.md#4-decisions-that-gate-the-plan)): the screen's single target puts both touches down from one press, and only `tst_parent.qml` presses it (see [the Qt Quick Test suites](#the-interactive-qml-qt-quick-test)); `parent_controller_test.cpp` drives the controller's two-touch interface directly |
| `libs/core/tests/unit/store_test.cpp` | `sns::Store`, `parse_store`, `serialize_store` | The format and its round trip; a golden file (format version 2 of known data, byte for byte, both written and read: `[golden]`); a file cut at every length and with every byte changed is refused; each record checked; a later version told apart; migration through a pretend older format; what a snapshot records (stars once, first-time words, bests, tallies, cleared boards, the bounds); the game in progress; each way a file on disk can be (missing, damaged, newer, older, unreadable, a refusing disk), kept whole and never overwritten; the worst case the bounds allow is under the cap; no time of day in the file |
| `libs/core/tests/unit/session_store_test.cpp` | `sns::snapshot_of`, `record_of`, `saved_session_of`, `session_snapshot_of` | The copy between the game session's values and the store's: fields, the stars from each find's reward, a stamp that is not a day refused, a board and every saved state through the store and back as it was, the words back in upper case and in order, what cannot come back refused; and a real session (one restored from a hand-written board, one the session dealt itself) taken through the store's file and brought back by `restore()`. **The standard board** (`[standard]`): a record with no phase and the count of its words, a record whose mode and phase disagree refused, the record and each saved state back as they were, a game in the box with no board to say its mode, a standard game brought back by the session and refused under the other switch, and the history keeping the board by its count; the store keeping a standard game in progress by the count of its words, and the session bringing it back (and a standard board the session dealt itself coming back from its saved session); a standard board recorded once however often it is sent, and clearing it a best |
| `libs/core/tests/unit/dashboard_test.cpp` | `sns::build_dashboard`, `focus_sound_choices` | Each count of a sound (met, found, unassisted, often found, few words) on a made-up phase; the week on either first day of the week; the vault and the best; nothing said of a week without play |
| `apps/app/tests/unit/assist_game_test.cpp`, `assist_model_test.cpp` | Mode A's `AssistGame` and `AssistModel` (the typed or scanned board played as a game) | A typed board played for the words that count with one of today's not named; the standard board counting every real word with no middle state; `begin` refusing a grid that is not a board (a tile holding a space or a slash cannot change the grid's shape) and nothing starting; no maker, no round; a board that cannot be made leaving the running round as it was; a round recorded once when replaced or left, and a round not left recorded by the destructor; a tile out of range and every touch with no round doing nothing; each tile sounding its letters and a word cueing the word; a real word that is not today's ticking without a word and a string fading silently; a path traced and removed tile by tile; the start tile offered only while the parent allows it; a board the store cannot keep told once as a save failure (`[app][scan][assist]`) |
| `apps/app/tests/unit/scan_model_test.cpp` | `ScanModel` (the check screen's draft and the camera's state machine) | The typed board an empty 4x4; a full grid confirms and an incomplete one does not; the reader's doubts as the tiles to ring with its other thoughts; a reading that is not a grid refused; with no camera the typed board is the way on and nothing is faked; Try camera again is a new attempt and a camera that appears is found; a frame with no board, or one the scanner cannot take, returns the screen to ready; the next capture clears the last problem; leaving while a frame is read drops the read; an index off the grid or a size that is not a board refused; the camera opened and closed only when it should be; replacing a source that is capturing stops the old one (`[app][scan]`, `[camera]`) |
| `apps/app/tests/unit/startup_failure_test.cpp` | `apps/app/src/startup_failure.*` | Each reason the program can leave for has an exit code of its own; a dialog is wanted unless a test or a headless run is in charge; a failure is told through the sink once and gives its code; the message keeps its non-ASCII text as UTF-8; the platform writes a fatal error to the error stream without a dialog (`[app][startup]`) |
| `apps/app/tests/unit/system_colour_scheme_test.cpp`, `system_motion_test.cpp` | `SystemColourScheme`, `SystemMotion` | The singleton starts as the system's setting is, reports a change once, and is read by name and notifies as QML needs; each makes a GUI application on Qt's offscreen platform. Built only with `SNS_BUILD_APP` |
| `apps/app/tests/unit/qt_text_test.cpp` | `apps/app/src/qt_text.hpp` | UTF-8 library text and a `QString` are the same text, a NUL inside keeps its length, a path from Qt text keeps letters the system's code page lacks (`[app][text]`) |
| `apps/app/tests/unit/sound_sequence_player_test.cpp` | The feedback module's `SoundSequencePlayer` | Plays the sequence of `sound_it_out_test.cpp` with no speaker: with no clips recorded the word is said at once and the steps are told in order; clips play one after another with the gap between and the word last; a clip that is not there takes no time and no gap; the whole-word-only sequence is the word alone; a new sequence stops the one under way and the old clip says nothing more; a sequence cancelled in a gap does not go on; the volume reaches the clips and the word (`[app][feedback][sequence]`). **No clip exists**: the cases use test clips, a recorded one is gated on the literacy reviewer, and the player's `stepStarted` signal is connected to nothing in the screens, so no block lights while a word is sounded out |
| `apps/app/tests/unit/frame_profiler_test.cpp` | `apps/app/src/frame_profiler.cpp`, as `sns_frame_profiler_tests` | The frame clock on a window that is never shown, its signals emitted by the test (the render thread's from a thread of its own): each signal adds to its own part of the report; the render and GUI threads' times leave out the warm-up; the GUI thread's frame is the time it was busy; the target is reached once and told on the GUI thread; the profiler can go while the render thread is mid-frame (`[app][frame]`) |
| `apps/app/tests/unit/profiler_test.cpp` | The profiler seam: the QML singleton that opens and closes a Tracy zone, in `sns_scene_script_tests` | The seam's `begin()` and `end()` always pair, named or not, and nesting past the seam's depth is balanced too (`[scene]`) |
| `apps/app/tests/unit/scene_script_test.cpp` | The board scene's script (`apps/app/qml/solo/SceneScript.js`), run in Qt's own JavaScript engine, as `sns_scene_script_tests` | The stages and their lengths (shake 600 ms, lid 250 ms, drop 380 ms a die, settle 150 ms; a 6x6 play within 2.2 s); the box's hops are whole steps of 6 px and never still; the lid lifts in quarters; a die falls through six frames in whole pixels and is not seen before its turn; the same seed is the same play and another seed another; with reduced motion nothing hops and the dice fade in together (`[scene]`, `[perf]`) |
| `apps/app/tests/unit/board_layout_test.cpp` | The layout rule, `SoloLayout` | The nine reference windows of the board design: which side the board takes, the tile of each size 3x3 to 6x6 against the page's table, the inset and gap either side of 600 dp, the board centred, a size offered only at 48 dp, the object recomputing on a rotation (`[layout]`) |
| `apps/app/tests/unit/path_tracer_test.cpp` | `PathTracer` | A slide, a diagonal that skips no tile, a fast jump, the hit circle smaller than the tile, sliding back, a tile used twice, a dead jewel, lifting outside, tapping in order and the last tile again, a trace carried on from taps, a press in a gap, the keyboard's moves, a rotated window (`[trace]`) |
| `apps/app/tests/unit/fake_solo_game_test.cpp` | `FakeSoloGame` | The flow of the game design: the board dealt by the shake, the size, a viable word and its stars, a word found again, a real word that is not today's (no word in the answer), not a word, clearing and then the C++ `resting` state, the best, the start tile (shortest, nearest the top left, stays until found), the history, the blocks (`[fake]`) |
| `apps/app/tests/unit/real_solo_game_test.cpp` | `RealSoloGame` over `sns::GameSession`, `SoloGameSetup`, `make_solo_game()` | The port's mapping as a table, run on two tiny phases written in the test: each state, the shake that deals no board, `settled` from either state it waits in, each answer to a trace (a word of today's by tier and stars, found again, a real word that is not today's with no word in the answer, not a word, a single tile), clearing and the new best, a sticker only the first time on a device, what is handed to the store after a find and when the lid goes on, the history newest first with a dead jewel blank only on a board of the phase, the start tile and the help switch, the grapheme blocks; loading a phase directory, the settings it makes a game from, a store file round trip, and each way loading fails; and the game in progress: what the store keeps after each change (the box, the shake and the drop keep none), the size chosen written to the settings and nothing else of them, a table of runs closed in each state and opened again (a board with finds and a lit tile, cleared, resting, Again, the lid on, mid-shake), a restored game going on with its board recorded once, four records that cannot come back (another phase, a changed board, a state that does not fit, a start tile lit for a word already found) starting a new game in the box and clearing the file, the help switch off at restore, and `SoloModel` opening on a restored board or on a cleared one put away (the C++ `resting` state, which the window shows as the summary sheet, not a screen of its own: [D12](roadmap.md#4-decisions-that-gate-the-plan)) (`[real]`); and the **standard board** over the shipped dice and dictionary (`[standard]`): a game with the four sizes, a roll with no dead jewel, the scene's board the dice of its seed (and a phonics board, which is seeded and not rolled, has none), the size chosen 3 to 6, a setup without a phase directory making the standard game, a standard run beginning where the last one ended, and a saved game of the other mode not played by this one; and the parent area on the game's store: the dashboard sees a find the game recorded, a setting the controller changes is what the setup makes the next game from, and the controller's edit reaches the model's next game (`[parent]`) |
| `apps/app/tests/unit/solo_model_test.cpp` | `SoloModel` over the fake | The states as the screens see them, input before the dice settle, the find shown and ended, the tick and the fade, tap and Done, a find again, the start tile and the parent's three-step help (the default is the two rungs, the start tile and then the tiles of its first two letters; the start-tile level stops at the first rung and the next press puts it away; off takes the help away, rings and all; a level the table lacks changes nothing; a find from the lit start answers the help and says what is left), the blocks as a path grows, a dead jewel, the last word and the rest, the lid back on, the model built once and read many times per trace (`[solo]`); **the board clock** (`[clock]`): off until the parent sets one and ticking only while a board is active, a count-down starting with its minutes and ending the round at zero, each new board starting it again, the clock's own timer counting the time that really went by and a stalled interface not losing it, a change mid-round keeping the time already played; an edit by the parent gives the next game at once in the box, after the lid or Again otherwise, and never with no edit (`[edited]`) |
| `apps/app/tests/unit/feedback_engine_test.cpp` | `FeedbackEngine` and the parent's settings map | The cue wiring without a speaker: master Sound off silences assets and speech and leaves the vibration request, Celebration sounds off and Sound it out off each stop their own half, a cue kind the design does not have is ignored, and the engine starts from the library's defaults; a found word at full volume asks for the cheer and the voice once each and a quiet celebration is half as loud; the rattle plays and buzzes and says nothing; requests are reported once per cue (`[app][feedback]`). **Sound it out** (`[sequence]`): a found word of the phonics board is sounded out and then said, `word` says only the word, `off` says and sounds nothing out, the standard board or a word the phase does not hold says the word whole, and with clips the word waits for the sounds and the next word stops them. The cue-to-asset mapping itself is the pure policy's case |
| `apps/app/tests/unit/motion_source_test.cpp` | `MotionSource` | On a desktop there is no accelerometer: unavailable, no sample arrives, starting and stopping is safe, and the drag stands in. Built only with `SNS_BUILD_APP` (`[app][motion]`) |
| `apps/app/tests/unit/check_scene_contract.cmake` | `apps/app/qml/solo/BoardScene.qml` and `apps/app/ui-tests/StubBoardScene.qml` (the scene the Quick Test suites of the Solo screen use) | `app.scene_contract`: both declare the whole interface of the board scene, under the same names |
| `apps/app/tests/unit/run_qmllint.cmake` | The application's QML | `app.qml_lint`: no warning from `qmllint` over the application's QML, the board scene's module and Mode A's module (not the parent area's module); skipped, exit 4, if the application has not been built |
| `apps/app/tests/unit/parent_controller_test.cpp` | The parent area's `ParentController` | The gate as the screens drive it, the dashboard and settings maps, every setting changed by name and every wrong value refused, the resets, what the parent is told of the store (a store in memory, in a file, unreadable or from a newer version; a save that fails, and one that is not repeated when a setting is already what it is), the totals and the kept boards the My jewels screen reads (`[totals]`; the screen itself has no test, [below](#the-interactive-qml-qt-quick-test)), and what the game is told of an edit (`[edited]`: a change, a reset, not a read or the same value). The gate cases drive the controller's **two-touch** interface; the one-press button is the screen's (`tst_parent.qml`). Built only with `SNS_BUILD_APP` |
| `apps/app/tests/unit/language_test.cpp` | `LanguageManager` (`apps/app/src/language_manager.*`), the catalogues in `apps/app/translations/`, the app's font chain | The eight catalogues are well formed, none of their messages marked `unfinished`, `vanished` or `obsolete`, no form empty, and the same messages as the English source. **That is not the same as translated**: a message left in English is a non-empty translation, and nothing counts those ([The interface's languages](#the-interfaces-languages)); every language's code, direction and fallback (Hebrew and Yiddish RTL, the rest LTR, the device's locale or English); applying a language installs its catalogue and turns the application's layout; and the eight sample sentences' glyphs are in the app's font chain (a Windows check today, skipped where the platform has no such fonts). Built only with `SNS_BUILD_APP` |
| `apps/app/tests/unit/parent_ui/` | The parent area's screens | `parent.screens`: see below; `parent.languages`: the gate, dashboard, settings and about in all eight languages, the catalogue really applied, the bar mirrored in Hebrew and Yiddish, no QML warning |
| `apps/app/tests/unit/scan_ui/` | The Family Assistant's screens | `scan.screens`: see below; `scan.languages`: the scan screen, the check screen and the assistant's panel in all eight languages, the split mirrored for RTL while the replica stays as confirmed, no QML warning |
| `tests/system/check_cli.cmake` | `sns-cli` as a program | `system.cli.*`: help and version, and `output-every-time` (`--version` and `--help` run 60 times each, because a flush lost at exit in a Debug build once left one run in ten with nothing printed); the README board solved from a twelve-word list (`9 words, 9 points`); `--min-length`, `--scoring rarity`, `--repeat`; a `Qu` tile; and each failure - an unknown option, a missing value, a bad length/count/scheme, no board, a ragged board (exit 3), a missing dictionary (exit 2) |
| `tests/system/check_app.cmake` | `SnapAndSpell` as a program | `system.app.*`: a missing word list (exit 2, the file named), a damaged store (set aside and said), `--phase` without `--phase-directory` (warned and ignored), and a headless smoke run that either reports frames (`smoke: frames=`, `ui.frame_ms`) and exits 0 or says no frame can be presented and exits 3 |
| `tests/system/check_install.cmake` | The install rules, run | `system.install`: `cmake --install` into a throw-away prefix; the two programs, the dictionary, `LICENSE.txt` and `NOTICES.txt`; then the installed `sns-cli` solves the README board from the installed dictionary |
| `apps/app/ui-tests/` | The kit and the screens, driven at run time | `quick.<suite>`: Qt Quick Test suites (`box`, `home`, `kit`, `panel`, `parent`, `rtl`, `scan`, `scene`, `theme`, `contrast`, `sizematrix` with `_audit` and `_solo`), and `quick.lint` with `quick.lint.selftest`; see [The interactive QML](#the-interactive-qml-qt-quick-test) |
| `tests/i18n/` | `apps/app/translations/*.ts` | `i18n.catalogues`: lupdate regenerates the catalogues and the checked-in ones must be identical; skipped (exit 4) when Qt's Linguist tools are absent |
| `tests/notices/check_notices.cmake` | The generated `NOTICES.txt` | `notices.sections`: the project's licence, the GPL-3.0-or-later application (and nothing that says GPL-3.0 only), Qt's LGPL/GPL options and the feedback modules Multimedia/Sensors/TextToSpeech (in a build with the application), ENABLE, the translations, the bundled fonts and feedback sounds and Catch2 are all stated, no placeholder is left, and a command-line-only build carries no Qt section |
| `tests/package/check_package.cmake` | `CPackConfig.cmake` and the generated install scripts | `package.config`: CPack names the project, its version, its licence and a ZIP, and the install rules carry both programs, the dictionary, the dice, the notices and the licence text, with Qt's install-time deploy script where the platform uses it |
| `tools/package/check_package.ps1` | A built tree and the package it makes | The archive's required files, `SHA256SUMS`, the packaged `sns-cli` solving a board from its installed dictionary and the packaged window's offscreen smoke. Run by hand and by the Windows CI leg, not by ctest |
| `tests/package/run_release_tools_test.ps1`, `test_release_tools.sh` | `tools/release` | `release.tooling`: the tag checks, signing and verification with a throw-away key, the SBOM (validated against the CycloneDX 1.6 schema by `validate_sbom.py`), and that `tools/package/check_package.ps1` refuses a Release package carrying Tracy, an ASan runtime or a pdb (`test_check_package_forbid.ps1`); needs bash, gpg, jq, git and pwsh and reports itself skipped (exit 77) where one is missing. `release-tooling.yml` runs it on a runner that has them ([CI](ci.md)) |
| `tests/toolchain/` | `.github/toolchain-versions.json`, the workflows, `project(VERSION)`, `vcpkg.json`, `CHANGELOG.md` | `toolchain.versions`: the workflows' CMake, Ninja, Qt and aqtinstall pins are the file's, and the vcpkg baseline is the pinned commit; `toolchain.version` and `toolchain.version.selftest`: the version is one number in the three places and the check is held to a table of cases on throw-away trees |
| `tests/profiling/check_tracy_port.cmake` | `cmake/vcpkg-ports/tracy`, `vcpkg-configuration.json`, `vcpkg.json` | `profiling.tracy_portable`: the Tracy overlay port is configured, passes `NO_ISA_EXTENSIONS` and is the version the manifest pins, so a capture tool built on one hosted runner and restored on another does not die with an illegal instruction |
| `tests/profiling/check_trace.cmake`, `check_perf_ran.cmake` | A trace of the two programs; the performance tier's cases | `profiling.trace_cli`, `profiling.trace_app`, `profiling.perf_budgets_ran`: see [The trace tests](#the-trace-tests) |
| `tools/data/check_phonics_review.cmake`, `test_check_phonics_review.cmake` | The review headers of `data/phonics` | `data.phonics.review_gate.selftest` (always) and `data.phonics.review_gate` (when `SNS_REQUIRE_REVIEWED_PHONICS` is on): see [the review gate](#phonics-data-the-checker-and-the-review-gate) |
| `tests/requirements/` | `docs/requirements.md` and `docs/requirements/*.md` | `requirements.matrix` and `requirements.matrix.selftest`: see [The requirements matrix](#the-requirements-matrix) |
| `tests/privacy/` | The source tree | `privacy.sources` and `privacy.sources.selftest`: see below |
| `tests/parent/` | `ParentWording.qml` | `parent.wording` and `parent.wording.selftest`: see below |
| `libs/core/bench/` | The solver, the word list, and the code that makes a board (the roll of the dice, the seeding), binds its blocks and deals and plays a session, what the screens wait on besides the solver (the store a play leaves, the dashboard, the phase directory, a path submitted), and the scanner's work, as `sns_perf_tests` | See below: [Performance tests](#performance-tests) |
| `libs/scan/tests/` | The scanner, the camera and `board_reader` interfaces and the synthetic corpus, as `sns_scan_tests` | See below: [The scanner](#the-scanner) |
| `libs/scan/tests/check_privacy.cmake` | The scanner's sources and the camera and `board_reader` files | `scan.privacy`, `scan.privacy.catches_a_leak`: see [The scanner](#the-scanner) |
| `tests/design/check_tokens.cmake` | `docs/design/toybox/tokens.json`, `sns-tokens.json`, `Theme.qml`, `assets/fonts`, the application's and the tests' font lists | `design.tokens`: every colour of the shared palette, the dimmer surface, the Night shadow, the three faces, the smallest text size and the touch targets of the design are what `Theme` says, and the font files, the resource list and both loaders are one list |
| `tests/analysis/check_analysis.cmake` | `CMakePresets.json`, `.github/workflows/analysis.yml`, `codeql.yml` | `analysis.config`: the analysis presets exist with their settings, `analysis.yml` is nightly and not on a push, pull request or `merge_group`, and `codeql.yml` is nightly too |
| `tests/profiling/check_presets.cmake` | `CMakePresets.json`, `vcpkg.json`, `cmake/Tracy.cmake` | See below: `profiling.shipped_presets` |
| `tests/profiling/check_zone_names.cmake` | Every `zone<"...">` under `libs/` and `apps/` | See below: `profiling.zone_names` |
| `tests/platform/check_fragments.cmake` | Every `libs/platforms/*/platform.cmake` | See below: `platform.fragments` |
| `tools/checks/check_platform_isolation.ps1` | The source tree | See below: `platform.isolation` |
| `tools/data/check_wordlists.ps1` | `data/dictionaries` and `data/lexicon` | `data.wordlists`: lowercase a-z, sorted, unique, and every lexicon word in the dictionary |
| `tools/data/make_dice.ps1 -Check` | `data/dice` | `data.dice`: each file is what the tool makes from the fixed Scrabble-style table, so a changed generator or a hand edit fails |
| `tools/sounds/make_sounds.ps1 -Check` | `assets/sounds` | `data.sounds`: each placeholder WAV is exactly what the tool writes, byte for byte |
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

`game_rules_test.cpp` uses no word list that is in the repository, except the
one case tagged `[data]` (the accepted risk, below): the standard board's word
set is thirteen words written in the test, the phonics index is seven of them
(the same seven the phase-data tests use) and every board is written inline, 3
by 3 or smaller.

Most cases are the design's sentences. A few pin a reading the design left open;
each is one comparison in `libs/core/src/game_rules.cpp` and one case, so a
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
| **The standard board has no block list** (an accepted risk, [D7](roadmap.md#4-decisions-that-gate-the-plan)): a word unsuitable for a child is counted, named and read like any other on it, and only the phonics board keeps it out | `a word a child must never meet is counted by the standard board and not by the phonics` (reads the shipped ENABLE file; the eight words are hashes) |

The path check is held against the solver rather than restated: for every simple
path of a 3 by 3 board, `classify_path` says `viable` exactly when the solver
finds the path's word, and on the standard board never says
`real_not_todays`.

## Game session: what is pinned

`game_session_test.cpp` uses no word list that is in the repository, except in
the cases tagged `[data]`: the phases are the seven and four words the seeding
tests use (TEST DATA, placeholder sounds), the dictionary is those eleven words
and nine more written in the test, the dice (for the standard board) are one die
of nine faces, six of them letters of those words and three of them not, and most
cases play a 3 by 3 board written out in the test and brought in through
`restore`, so that its five words (phonics) or twelve (standard) and their paths
are known without asking the solver. The cases about dealing use the session's own
boards and hold them to the solver. The `[data]` cases read the shipped dice and
the full dictionary by their path in the checkout. A few pin a reading the design
left open or took two ways:

| Reading | Pinned by |
| --- | --- |
| A real word is never answered "not a word", of any length. The rules (`classify_path`) call a word under three letters `not_a_word` because it cannot count; the session answers `real_not_todays` for it, the soft tick, and names nothing. | `a real word of two letters is a real word, not 'not a word'`, and the 10,296-path case, which holds the answer to the two word lists for every path |
| A path of fewer than two tiles is `ignored`: a touch to hear a letter, or a trace cancelled outside the board, is not a word attempt | `a touch on one tile, or nothing, is not a word attempt` |
| The word is named only when it is one of the phase's: `real_not_todays` and `not_a_word` carry none; the record, the snapshot, the tray and the help hold none either. A stand-in for a word a child must never meet is in the dictionary and traced, and is found nowhere the session hands out | `a word that is only in the dictionary is never handed out`, and `no path of the board is answered wrongly, or names a word that is not today's` |
| The help answers with a tile, never the word; the word it is for is in the snapshot only, because the light has to come back with the board | `the help answers with a tile and never a word` |
| A refused event changes nothing: not the state, the board, the finds or the seed | the table of events by states, which compares snapshots before and after |
| `solo.word-found` is not a state: the session is already `playing` (or `cleared`), and a trace is accepted through the screens' few moments of showing a word | `a viable path is a find`, `the last viable word clears the board` |
| A snapshot saves `shaking` and `dropping` as the box and `cleared` as `resting` (the state the store keeps for a cleared board put away; the window shows it under the summary sheet, not as a screen of its own) | `a snapshot in shaking or dropping is the box`, `a snapshot in cleared is saved as resting` |
| The recent targets are not saved: a restored session continues the same only from the box | `a snapshot brings a session back as it was, in each state` (it checks the next board from the box only, on purpose) |
| **Standard board.** Every word of the dictionary of three letters or more counts and is named, whatever it is: the accepted risk of the full dictionary with no block list (docs/design/mvp-library.md). A real word of two letters is still a tick | `every word of the dictionary of three letters or more is a find on a standard board`, `the same trace is a tick on the phonics board and a find on the standard`, `on the standard board only a real word of fewer than three letters is a tick`, `every path of the standard hand board is answered as its spelling says` |
| **Standard board.** The deal is the library's `deal_standard_board` over the dice of the chosen size, so a board that holds no word is never dealt; the report counts rolls and one solve a roll and no seedings; the next board is rolled from the seed after the roll that made this one | `a roll that holds no word is rolled again, and never dealt`, `the deal is the library's deal of the standard board, seed for seed`, `the deal is one solve a roll, and the board holds the words the solver finds`, `the session deals the standard boards the library's deal does, pinned for two seeds` |
| **Standard board.** A board is cleared by every word of the dictionary on it, which is about 33 on a 3x3 and about 314 on a 6x6 (the means over the performance tier's 100 boards a size, [below](#the-corpus)), so a standard board is ended by the child far more often than by clearing | `the standard board is cleared by every word of the dictionary on it`, `a standard board of the project's dice is cleared by every word the dictionary has on it` |
| **Standard board.** The record lists no words and counts them (`viable_total`); `restore` checks the count, and refuses a snapshot of the other mode (`wrong_mode`), a record and a snapshot of two modes, a list of words, no count, or a count that is not the board's (`board_changed`) | `a standard board's record says its mode, has no phase and counts its words`, `a standard snapshot that no session makes is refused`, `a snapshot of the other board mode is refused` |
| A game in the box has no board to say which mode it was in, so restore plays the mode it is given; a size the phonics board is not played at cannot be restored under it | `a game in the box comes back whichever mode it is played in` |

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

Every test registered in `tests/checks/CMakeLists.txt` carries the `quality`
label (and so do `app.scene_contract`, `app.qml_lint`, `quick.lint`,
`quick.lint.selftest`, `scan.privacy` and `scan.privacy.catches_a_leak`, which
are registered beside the code whose build they read). The ones the sections
below describe at length are marked "below":

| Test | What it holds | Needs |
| --- | --- | --- |
| `notices.sections`, `package.config` | The generated notices and the CPack configuration (rows of [What there is](#what-there-is)) | Nothing |
| `platform.fragments`, `profiling.shipped_presets`, `profiling.zone_names` | Below | Nothing |
| `profiling.tracy_portable` | The Tracy overlay port is configured, passes `NO_ISA_EXTENSIONS` and is the version the manifest pins | Nothing |
| `profiling.perf_budgets_ran` | A leg that claims the performance work budgets ran them ([The trace tests](#the-trace-tests)) | An optimised build, or `SNS_REQUIRE_PERF_BUDGETS=1`; otherwise it reports itself skipped (exit 4) |
| `toolchain.versions`, `toolchain.version`, `toolchain.version.selftest` | The workflows' toolchain pins are `.github/toolchain-versions.json`'s; the version is one number in `project(VERSION)`, `vcpkg.json` and the newest `CHANGELOG.md` release (and the release job runs the same script with the tag) | Nothing |
| `analysis.config` | The nightly analysis presets and workflows (row of [What there is](#what-there-is)) | Nothing |
| `design.tokens` | The design's tokens against `Theme.qml`, and the font lists (row of [What there is](#what-there-is)) | Nothing |
| `privacy.sources`, `privacy.sources.selftest`, `parent.wording`, `parent.wording.selftest` | [The privacy and parent-area checks](#the-privacy-and-parent-area-checks) | Nothing |
| `requirements.matrix`, `requirements.matrix.selftest` | [The requirements matrix](#the-requirements-matrix) | Nothing |
| `data.phonics.review_gate.selftest`, `data.phonics.review_gate` | The release build's review gate, held to cases (always) and to the data there is (only when `SNS_REQUIRE_REVIEWED_PHONICS` is on): [Phonics data](#phonics-data-the-checker-and-the-review-gate) | Nothing: a CMake script |
| `i18n.catalogues` | [The interface's languages](#the-interfaces-languages) | `SNS_BUILD_APP`, and Qt's Linguist tools (skipped, exit 4, without) |
| `data.dice`, `data.wordlists`, `data.sounds`, `data.phonics`, `data.phonics.checker`, `platform.isolation`, `release.tooling` | Rows of [What there is](#what-there-is), and below | PowerShell 7 (`release.tooling` also bash, gpg, jq and git, and is skipped, exit 77, where one is missing) |

**`platform.fragments`** runs `tests/platform/check_fragments.cmake`. A build
only includes the one `platform.cmake` for the platform it targets, so a typo in
another platform's fragment (a renamed file, a hook that was never defined)
would otherwise be found only by someone on that platform. The check includes
every fragment under `libs/platforms/`, each in a process of its own, and holds
it to the contract in `cmake/PlatformContract.cmake`: every variable and hook
defined, `SNS_PLATFORM_DESKTOP` either `ON` or `OFF`, `SNS_PLATFORM_EXECUTABLE_FORMAT` one of `elf`, `pe` and `mach-o`, every file it compiles
present, and every interface header under
`libs/platforms/interfaces/sns/platform/` included by one of the fragment's own
sources (so a new interface, haptics included, cannot link by accident). It
also checks that `cmake/Platform.cmake` has a branch selecting each
fragment, and that every branch has a fragment. It runs on whatever host runs
the tests, so a Linux machine checks the Windows and macOS fragments.

**`data.sounds`** runs `tools/sounds/make_sounds.ps1 -Check`, which regenerates
the eight placeholder feedback WAVs in memory and compares them with the
checked-in `assets/sounds/*.wav`: a hand-edited file, a missing cue or a script
change that was not committed is a failed test. It is registered only when
`pwsh` is found, like the data checks.

**`platform.isolation`** runs `tools/checks/check_platform_isolation.ps1`
against the source tree. It is registered only when `pwsh` is found. What it
checks, and why, is in [Platform isolation](platform-isolation.md#enforcement).
You can run it by hand from the repository root:

```sh
pwsh ./tools/checks/check_platform_isolation.ps1
```

**`profiling.shipped_presets`** runs `tests/profiling/check_presets.cmake`. A
profiling build has a Tracy client in it, which opens a TCP port and answers
discovery broadcasts, so nothing that ships may have it — while every Debug
build is instrumented, because it is a development build by definition. The
check resolves `inherits` the way CMake does and fails if a configure preset
other than a `*-debug` or `*-profiling` one turns `SNS_ENABLE_TRACY` on (a
release or coverage preset, above all), if a debug preset does not turn it on,
if the option's default in the top-level `CMakeLists.txt` is not `ON` for a
Debug build type (and `OFF` otherwise, including multi-config generators) or
`cmake/Tracy.cmake` declares a second default, if `tracy` is a base dependency
of `vcpkg.json` or `profiling` a default feature, if the top-level
`CMakeLists.txt` requests the feature other than under that option, or if a CI
workflow sets the option by hand. It reads only the presets' JSON, so it runs
on every host and in every CI leg. [Profiling](profiling.md) is the page about
why.

**`profiling.zone_names`** runs `tests/profiling/check_zone_names.cmake`. A
zone's source location is a function-local static of `zone<"name">()`, so one
name is one place and a name used twice would show the first place's file and
line for both. It scans `libs/` and `apps/` (and the `.qml` and `.js` sources, whose
`Profiler.begin("name")` calls share the C++ namespace) and fails on a name that
is used twice, is not `<layer>.<thing>` lower case and dotted, has a layer that
is not in the check's registry, or is missing from the required set a trace of
the program is made of, so removed instrumentation is a failed test. It reads
text, so it cannot tell a zone in code that never runs from one that does; the
trace tests ([below](#the-trace-tests)) do.

## The privacy and parent-area checks

**`privacy.sources`** runs `tests/privacy/check_sources.cmake` over the first-party tree
(`libs/`, `apps/`, `cmake/`, `tests/`, the top-level CMake files, any directory that is not
third-party or documentation) and its manifests, for the rules of the first release that
a source scan can hold ([Application design](design/application.md#privacy-built-in-and-checked)):
no audio capture, no network class or header or URL literal or network Qt module or
manifest permission, no export route (printing, sharing, a save dialog, the clipboard,
a URL handed to the system, mail, a launched program), and no file written outside
`libs/core/src/store.cpp`. A comment line is skipped; the checker, its self-test, the
scanner's own privacy
checker and its deliberate leak (`libs/scan/tests`, which name the forbidden things as data) are
not scanned. It is a
pattern scan, not a proof, and Qt Qml links Qt Network itself, so the rule is "no
capability and no use", not "no module". **`privacy.sources.selftest`** holds each rule to
a violation it must find and to the cases it must leave alone, so a pattern that stops
matching is a failure and not a pass. Both are CMake scripts, run on every host with no
pwsh.

**`parent.wording`** holds `apps/app/qml/parent/ParentWording.qml`, the one file of
sentences a parent reads, to the design (dashboard.md, "Wording"): the sentences the
design quotes are there, none of a list of words an assessment or a gap is made of
("weak", "mastered", "missed", ...) is in any parent screen, and no screen has a `qsTr`
of its own. **`parent.wording.selftest`** holds the check to cases.

**`parent.screens`** (with `SNS_BUILD_APP`) runs `sns_parent_preview --check`: the
parent area's screens in a stub shell, drawn by the software renderer under Qt's
offscreen platform (`QT_QPA_PLATFORM=offscreen`; the platform fragment names anything
else the host needs, `SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT`). It drives the gate with
synthetic touches (the controller's two-touch interface, not the one button a
person presses) on a clock of its own, walks the dashboard, the settings and the about
page over made-up play, checks what is on screen (the design's sentences, the settings
the table names, nothing of the data at the gate, none of the forbidden words) and that
QML warned of nothing, and with `--out <dir>` saves a picture of each screen. The same
program without `--check` is a window to look at them in.

## The interface's languages

The interface is offered in eight languages, right to left and left to right
([Application design](design/application.md#languages-right-to-left-and-fonts)), and the
checks below hold it. They all run in every ctest of a
build with the window (`SNS_BUILD_APP=ON`, the Windows CI leg).

- **`i18n.catalogues`** regenerates `apps/app/translations/*.ts` from the QML sources with the kit's
  `lupdate` into a temporary directory and fails when the checked-in catalogue
  differs: a `qsTr()` added, removed or moved without running the update target is
  caught. It reports itself **skipped** (exit 4) when Qt's Linguist tools are not
  installed, so the Linux and macOS legs, which do not build the window, and a
  developer without `qttools`, can still run the rest.
- **The catalogue case** (`apps/app/tests/unit/language_test.cpp`, in `sns_app_tests`) reads
  each `.ts` with `QXmlStreamReader`: well formed; no message marked
  `type="unfinished"`, `vanished` or `obsolete`; no empty translation (plural
  forms are checked where a catalogue has them); each language present, with the
  same number of messages and the same `(context, source)` set as the English
  source catalogue. **It does not check that a message is translated**, only that
  it is not marked unfinished and is not empty (next).
- **The direction case** holds the eight codes: Hebrew and Yiddish right to left,
  the other six left to right; an empty setting is the device's own language, a
  locale naming one of the eight resolves to it, and an unknown code is English.
  Applying each language sets `QGuiApplication::layoutDirection`, and the
  translator really is installed (a parent sentence comes back translated, and not
  for English).
- **`parent.languages`** and **`scan.languages`** load the parent area and the
  Family Assistant's screens once per language, before the engine loads, and walk
  them. Each language's title or tray sentence must be the catalogue's, not the
  English; Hebrew and Yiddish must mirror the bar and the split (the way back moves
  to the right, the panel to the left) while the six left-to-right languages keep
  them as before, and QML must warn of nothing. `--out <dir>` saves a picture of
  each language for the reviewer.
- **The glyph case** lays out one sample sentence per language with the app's font
  chain (`Andika`, which is in `assets/fonts`, but the case does not register the
  bundled fonts: it asks the platform's own) and checks that the platform's
  fallback carries every glyph. It **skips** itself with a message on a platform whose fonts lack
  the script. It is a Windows check today: the Windows leg gives Qt's offscreen
  plugin `QT_QPA_FONTDIR` (the system font directory), so the case runs there,
  while a Linux runner without those fonts would report it skipped. The iPad's
  fonts are looked at on a device. The fonts themselves are recorded in
  [Application design](design/application.md#languages-right-to-left-and-fonts)
  and [Licensing](licensing.md#fonts-and-the-interfaces-languages).

**What these checks do not show: that the interface is translated.** Each of the
eight catalogues has 279 messages, and none is `unfinished`, so the catalogue
case is green. But in each non-English catalogue **90 to 96 messages have a
translation identical to the English source** (90 in Chinese, 91 in Spanish,
Hebrew and Japanese, 93 in Yiddish, 96 in German and French), and **no test
counts them**. A few are the same in the language (the product's name), but many are English
left in place ("My jewels", "Back", "1 point"): in the Hebrew catalogue they are chiefly the
My jewels screen (`JewelsWording`, 24 messages), the parent area's wording (24)
and Solo Play's (23). The translations are in any case mechanical and not
reviewed by a speaker of each language ([roadmap](roadmap.md#2-where-it-stands)).
The test that would close the gap, a failure when a translation equals its source
outside an allow-list, is not written (R-I18-03 in the [application
requirements](requirements/application.md)).

## The interactive QML (Qt Quick Test)

The screens and the kit are exercised at run time by twelve Qt Quick Test suites,
headless: `sns_quick_tests` (`apps/app/ui-tests/`) starts each one under Qt's offscreen
platform with the software renderer, so no leg needs a display or a GPU. It runs
in every ctest of a build with the window (`SNS_BUILD_APP=ON`, the Windows CI leg
today). `apps/app/ui-tests/CMakeLists.txt` registers one ctest entry for each
`tst_<suite>.qml` file it finds (`quick.box`, `quick.contrast`, `quick.home`,
`quick.kit`, `quick.panel`, `quick.parent`, `quick.rtl`, `quick.scan`, `quick.scene`,
`quick.sizematrix`, `quick.sizematrix_audit`, `quick.sizematrix_solo` and
`quick.theme`, labelled `system;ui;gui`), so a new suite file is a new test with
no edit to the CMake file, and two more that read sources only: `quick.lint` and
`quick.lint.selftest` (labelled `quality;ui`, below).

```sh
ctest --preset windows-msvc-debug -R "^quick[.]"     # the tier
ctest --preset windows-msvc-debug -R "^quick[.]kit"  # one suite
build/windows-msvc-debug/bin/sns_quick_tests -input apps/app/ui-tests/tst_kit.qml -platform offscreen
```

The QML under test is the application's own, not a copy: the kit, Home (with its
wording) and Solo Play's screens are aliased (`QT_RESOURCE_ALIAS`) into the test's own QML module,
exactly as the two stub shells (`apps/app/tests/unit/parent_ui`, `apps/app/tests/unit/scan_ui`) load
theirs, and the parent area's and the Family Assistant's static modules are
linked as they are there. The application's own module cannot be linked - it is
compiled into its executable - and `Main.qml` is not loaded: it imports all three
screen modules and makes the window, and the suites test the screens it would
show.

The board scene is the one piece of test support: the Solo screen takes its scene as a
`sceneComponent`, and the suites give it `apps/app/ui-tests/StubBoardScene.qml`, a `BoardScene` of the
same interface (`prepare`, `show`, `play(board, seed)`, `pushMotion`, `settled`, `busy`,
`boardRect`, `tileSize`, `tileGap`) that draws nothing and records what it is
given, with a `Profiler` stub beside it. `app.scene_contract` already holds the two real `BoardScene`s to that
interface; no suite needs the 3D one.

The feedback seam's engine (`kit/Feedback.qml`) is left null in the fixtures:
the suites record the `cue` signal and nothing plays, speaks or vibrates, which
is exactly the quiet engine a stub shell has. `Main.qml` is the only place that
installs the real one, and the app tests hold the engine and the motion source
without a device (see `[app][feedback]` and `[app][motion]`). The box's motion
suite passes a QML object with `MotionSource`'s shape (`available`, `active`,
`shakeEnabled`, `sample`, `shakeDetected`) to `SoloScreen`, so the samples' way
to the scene's `pushMotion` and a detected shake's way to the box are driven
without a sensor.

The models are the application's classes, made by `apps/app/ui-tests/quick_fixture.*` and
registered as context properties: `SoloModel` over `FakeSoloGame`,
`ParentController` over the demo data's in-memory store and phases with a clock
the test advances (`fixture.advance`), `ScanModel`, `AssistModel` over a test
dictionary and store, and `LanguageManager`. The suites that use them call
`fixture.reset()` from `init()`, so one test function's play is not the next
one's. A suite makes the test window visible - Quick Test's own root is
invisible - and, because a positioner lays its children out in a frame rather
than in the call that changed them, lets the window draw (and, where the layout
matters, waits for it) before a point is turned into a click.

A Qt warning fails the suite: each suite calls `failOnWarning(/.?/)` for a
warning inside a test function, and the harness's own message handler fails the
process for one emitted outside one (loading a suite, building a screen at
teardown). That is the rule the two stub shells' checks hold too, and the reason
a warning is not merely printed.

| Suite | What it holds down |
| --- | --- |
| `tst_kit.qml` | AppButton's 48/64/96 targets; a release inside it is a click and a slide off it is not; its keyboard Space; its press cue; a disabled button doing nothing; a chosen button shown by fill, edge and check and not by gold; a label wrapping and the button never falling below 48; focus shown for the keyboard and not after a press. HoldButton's ring filling, hold-to-act, letting go early, and the hold as a switch action. TileFace at rest and lit (the edge thickens and the tile grows, not colour alone) and the Qu face. Jewel's tier sizes. Icon's pictures and the one that turns round in RTL. The Feedback singleton's cues. Label: every role sets its face, size and line height, follows the text size, learning text sets Andika, tracking is in em and not applied to a right-to-left language, an eyebrow is capitals by font, headings are headings to assistive technology. Card (title, empty state), SegmentedChoice (a group of radio buttons), ActionBar (both buttons in a narrow panel, stacking when they do not fit, signals), Sheet (closes on Escape, has a title, traps Tab and gives focus back, a scrim click closes it and a panel click does not), AppBar (tabs announce the one chosen; a crowded bar drops its trailing group to a second row) and the tablet picture with its stand |
| `tst_theme.qml` | The light and the dark token values; the board the same in both; the colour-scheme token flipping `dark`, with System following `SystemColourScheme`; the text scale multiplying the type sizes; every type size a step of the scale and none under twelve; the line heights; the tray fitting inside the board area's inset; the state tokens in both schemes; reduced motion taking the animation durations away (0, or the shorter cross-fade); the target sizes and the jewel sizes |
| `tst_contrast.qml` | The design's contrast gates computed from the real Theme tokens in both schemes (WCAG 2.x relative luminance): text 4.5:1, non-text 3:1 (a control's edge, the focus ring, the selected edge, a track, the gold of the path and the hint against the die), the separations that tell a card from the paper and the tray from its stage; and a disabled button using tokens and not opacity |
| `tst_home.qml` | One child placard, the family card and the grown-ups' entry (the placard at least the primary target, the entry the minimum); the placard asking for Solo Play, the family card and the grown-ups' entry asking for the parent area; the boards entry asking for the history; the placard having the keyboard and Space opening it; Back on home changing nothing; the cards swapping sides in RTL. There is no longer a scanner card of its own: Mode A is reached from the parent area |
| `tst_box.qml` | The sizes offered are the ones whose tile fits at 48 dp; the box picture following the size; a size chosen by pressing its picture and the four sizes one row of 64 dp targets; the lid never covering the compartments; Shake dealing a board and the scene stub being given `play(board, seed)`; Space shaking too; a shake in play doing nothing; device motion driving the scene and rolling the box, and the Shake setting stopping both; the way back from the box asking for home; **Back from a board in play leaving the round in progress, with the same board and the state still `playing`** (not the lid back on); the Finish summary offering Play again first, keeping the focus inside it and walking Tab round; the summary being the shared Sheet over the whole window, closed by Escape; **a cleared board showing the summary and staying on it**, with no timeout and no resting screen ([D12](roadmap.md#4-decisions-that-gate-the-plan)), Play again going from a cleared board to the box in one step, and a cleared board put away (the C++ `resting` state, as a game brought back from the store can open on) shown under the same summary; the boards sheet having a title and a worded empty state and saying each board's size and words; the flat board's line following the path and the shown word |
| `tst_panel.qml` | The tray's one row a word, with the model's tier and stars and a jewel whose size says the tier; a word found again adding nothing; the hint button asking the model for a start and resolving when its word is found, a tapped path leaving the hint and the panel alone; Finish asking for the summary; **the board clock showing what the model counts**; the tray header showing points; the words card never collapsing in a tall window and keeping its room in the smallest wide one; no action clipped in a narrow panel; the word in hand never landing on the empty state; a word row a keyboard stop of the right size; the list showing whole rows and scrolling to the newest |
| `tst_rtl.qml` | Hebrew and Yiddish are right to left and French and English are not; the bar's way back moving to the leading edge on Solo Play's, the scan's and the parent area's bars; the panel's hint and history swapping sides; the split's areas swapping while the board's square, tile and gap stay the same; the flat board's tiles and the check screen's replica keeping their reading order in a mirrored hierarchy |
| `tst_scan.qml` | `scan.ready` saying there is no camera and offering no Capture (and no live view); a running camera source's preview shown and Capture offered; the camera codes being the design's sentences; the typed board opening the check screen, and a fresh typed board starting over; a correction through the letter picker; the reader's doubtful tiles ringed and their hint; the stepper showing Place, Check and Play; the check screen's actions aligned on the trailing side, in RTL too; Yes, play refusing an incomplete board and handing a confirmed one to the assistant; Scan again and Back returning to the frame; the way back leaving Mode A from the ready screen. `AssistScreen` and `AssistBoard` have **no** QML test |
| `tst_parent.qml` | The gate: `test_theGateAsksForTwoSpotsHeld` (the name is stale) presses the **one** target, `gateHoldTarget`, which puts both of the gate's virtual touches down: the controller holds, a second's hold is about half the bar, letting go starts over, two seconds opens it; `test_twoSecondsOpensTheGateAndShowsTheDashboard` drives the controller's two touches; the dashboard's week, sounds, vault and best from the store, and an empty store saying nothing was played; the settings rows changing the controller (help, board size, motion, colour scheme, text size) and a focus sound ticked and unticked; the reset asking twice and clearing the boards but not the stickers; the way back shutting the gate. **Not tested here**: a key (Space, Return) sent to the gate, a hold cancelled by moving off the button, the gate shutting after the scanner route, a dark parent screen |
| `tst_sizematrix.qml`, `tst_sizematrix_solo.qml` | The size matrix (`SizeMatrixCase.qml`, `SizeMatrix.js`): a subject is made at the desktop sizes (900x620, 900x1200, 1024x768, 2400x1200), an iPad both ways (820x1180, 1180x820), the phones upright (390x844, 375x667, 360x640) and on their side (844x390, 667x375) and, at 150% text, 900x620, 1024x768, 390x844 and 844x390, and every item that takes Tab focus is audited: at least 48 dp each way, wholly inside the window and its clipping parents (or what scrolls), no truncated label, no two controls on top of each other; and the tree is audited too, for a page that scrolls sideways and for a text past the edge of its card or the window. `tst_sizematrix` holds the kit's gallery and a Sheet over a screen; `tst_sizematrix_screens` Home, My jewels and the parent area's screens; `tst_sizematrix_solo` the box, a board in play with a path in hand, the summary sheet and the boards sheet; `tst_sizematrix_scan` the scanner and the assistant. `tst_sizematrix_scan` runs the desktop sizes only (`SizeMatrix.desktopSizes`) until the scanner is laid out for the phones and the tablets. A `knownIssues` list is a ratchet: an entry that matches nothing fails |
| `tst_sizematrix_audit.qml` | The audit held to cases: a fine screen has no findings; a control under 48 dp, one clipped by its parent or off the window, a truncated label and two controls on top of each other are each found; the same for a page that scrolls sideways and a text outside its card or the window, and the matrix is the design's sizes and holds the phones and the tablets |
| `quick.lint` | `apps/app/ui-tests/check_qml_literals.cmake`, a source scan with no display: a screen says what a thing is (a Label role, a Theme token) and never how it looks. It counts per file a pixel or point size, a letter spacing, a `#rrggbb` colour outside `Theme.qml`, a bare `Text` outside `Label.qml`, an opacity tied to `enabled`, gold on a checked or selected line, an icon-only button that is not Back or Close, and the deleted names (`ParentTheme`, `PButton`, `PChoice`), and holds each file to the ceiling in `qml_lint_allowlist.cmake`. The ceiling only goes down: a count above it, or a file with no entry, fails. The 3D scene is exempt |
| `quick.lint.selftest` | The lint held to cases on throw-away trees: each rule catches its own offence, the exempt places stay exempt, the allow-list is a ceiling and not a licence, and the real tree passes |

The suite is also the project's evidence that the UI exercises the code:
`coverage-ui` runs the `ui` label under the coverage instrumentation and
reports `apps/app/src`, the C++ the screens bind to. The Qt Quick Test suites are
what make most of that report real (a screen press reaches `SoloModel`,
`ParentController`, `ScanModel` and the layout objects through the bindings),
and the stub shells add their own flows; [Coverage](#coverage) gives the
command and the report's location. There is no floor on this report.

Not here, and where they are instead: the board scene (`app.scene_contract`,
`apps/app/tests/unit/scene_script_test.cpp` and `quick.scene`); every screen
in all eight languages (`parent.languages` and `scan.languages`); the window's
frame budget (`apps/app/tests/unit/frame_timing_test.cpp`, a desktop check).

Not tested anywhere, so that no row above is read as covering it:

- **`Main.qml`**, the shell: its Escape and Back dispatch, the reduced-motion OR
  of the parent's setting and the system's, and the colour, text-size and
  language bindings. The suites drive the screens it would show over a stub
  scene and a fake game, and the stub shells re-implement the wiring, so the
  route into Mode A and out of it is not exercised as built either.
- **The My jewels screen** (`JewelsScreen.qml`, reached from home, ungated by
  [D10](roadmap.md#4-decisions-that-gate-the-plan)) has no test of any kind. It
  is not in the test module, nor in the size matrix; only the data it reads is
  held, by `parent_controller_test.cpp` (`[totals]`).
- **`AssistScreen` and `AssistBoard`** (Mode A's play), keyboard play on the
  board, reduced motion at screen level, and a screen reader (only the kit's
  components are asserted to carry names and roles) have no QML test.

### Screenshots

`cmake --build --preset <preset> --target sns_screenshots` takes a picture of every screen and dialog of
the application from the offscreen platform and writes 31 PNG files into `<build>/screenshots` (or the
directory named by `SNS_SHOTS_DIR`). `apps/app/ui-tests/capture_screens.qml` drives the same screens the
suites test, through the same fixture, in this order: Home, Solo Play (the box, playing, a word in progress,
the summary), My jewels, the parent gate and the four tabs, the real-board screens (no camera, a live camera,
the check screen, the assistant), the kit gallery and a sheet, then Home in the dark scheme, at 150 % text
and in Hebrew, Chinese and Japanese, and five frames of the board scene. CI takes them on the
Windows leg (`Screenshots (Windows)`, which never fails the leg) and keeps them 14 days as the
`screenshots` artifact of the run. It is not a test (the file is not called `tst_*.qml`, so nothing registers it) and
it is not part of `all`. It is how a change to the look is compared with the one before it, and the
Toybox restyle ([design](design/toybox/DESIGN.md)) is checked against its mockups with it.

The pictures use the application's own board scene (the pixel scene draws without a GPU), and the
last five are frames of its play: the closed box, a shake, the lid lifting, the drop under way and the
settled board.

### The house fonts in the QML tests

The Quick Test harness and the two stub shells (`sns_scan_preview`, `sns_parent_preview`) register Pixelify Sans,
Silkscreen and Andika from `assets/fonts` before a screen is loaded (`sns_test_fonts`,
`apps/app/tests/fonts`), as the application does from its resource. Without them a screen is laid out in
whatever the machine has: every label has another width, so a layout assertion (the action zone's sizes,
the frame's room) measures the fallback and fails on a machine whose fallback differs from the
author's, and on macOS Qt warns that it is replacing a missing family, which a run that treats a
warning as a failure cannot pass. Andika, the reading face, is also made the application's default font: a
Text that names no family takes the platform theme's, the offscreen platform's is the generic "Sans
Serif", macOS has no such family and Qt warns again ('missing font family "Sans Serif"'); the Cocoa
platform of the real application supplies a system font, so only the tests need it. The glyph check of
`language_test.cpp` does not load them: it asks what the platform's own fonts cover, and skips on a machine
with none. `design.tokens` holds the files in `assets/fonts`, the application's resource list and both
loaders to one list.

### Camera tests without a media backend

The live view is a `VideoOutput`, and a `VideoOutput` needs a Qt Multimedia backend. Where the
backend plugin cannot load (a system library it links is missing) Qt prints "No QtMultimedia backends
found" and creates no sink, so every test that shows the live view would fail for a reason that has
nothing to do with the screens. `sns_media_probe` (`apps/app/tests/media`) asks
`QMediaFormat` whether the backend can decode anything, once per process:

- `camera.screens` returns 77 and CTest reports it as skipped;
- `quick.scan` skips `test_aRunningCameraShowsThePreviewAndOffersCapture` and `quick.sizematrix_scan`
  skips the rows of subject 1, through `fixture.mediaBackendAvailable`; the other cases run.

A kit built without Qt Multimedia answers "no backend" and does not register `camera.screens`. The
Linux application leg of `release.yml` installs the libraries the FFmpeg backend looks for
(`libpipewire-0.3-0t64`, `libva2`, `libva-x11-2`, `libva-drm2`), so there the cases run: Qt warns once per
library it cannot load, and a suite that fails on a warning fails on that. The probe prints those
messages itself, once, and does not count them against whichever case happened to ask first.

## Performance tests

`sns_perf_tests` is a binary of its own, linked against `sns::lib` and nothing
else (no Qt, no platform layer), so that it can be built and run alone and so
that nothing else's allocator churn is in the way of a stopwatch. It is
modelled on [Cubeulator's](https://github.com/iainchesworthlabs/cubeulator/blob/main/docs/testing.md#performance-tests)
and is the implementation of the tier in the [performance
design](design/performance.md). **Two halves, and the split is the design:**

| | Holds | Exact? | Runs |
| --- | --- | --- | --- |
| **Work budgets** | What the code *does*: visits, words found, the word list's words and trie nodes, verifications and solves spent seeding, comparisons made binding, the bytes of the store a play leaves and the rows of the dashboard built from it, the scanner's reads, and a digest of every result | Yes, on every machine and compiler | In every `ctest` of an optimised build |
| **Wall-time checks** | What it *takes*, as a multiple of a calibration workload run in the same process | No, and loose on purpose | Only when asked: `SNS_PERF_TIMING=1`, or a `*-perf` test preset |

The sources: `work_test.cpp` (the word list and the solver), `game_work_test.cpp`
(making a board, seeding, binding, and the two sessions), `data_work_test.cpp` (the
store, the dashboard, the phase directory, a path submitted), `timing_test.cpp` (the
wall-time cases), `report_test.cpp` (the hidden `[.perf-report]` tools), over
`corpus.cpp`, `metrics.cpp`, `data_metrics.cpp` and `measure.cpp`, with every
threshold in `budgets.hpp`.

```sh
ctest --preset windows-msvc-release -L perf        # the tier, work budgets only
ctest --preset windows-msvc-perf                   # the tier, with the wall-time checks
build/windows-msvc-release/bin/sns_perf_tests "[.perf-report]"   # what each metric is now
```

(`linux-gcc-` and `macos-clang-` likewise.) Every case carries the `perf`
label. There are 32 work cases and six wall-time cases in `sns_perf_tests` (the
hidden `[.perf-report]` cases are tools, not tests). A **Debug or sanitizer build
skips** the work budgets (reported as skipped, not failed: the corpus is much
longer there, and the counts are the same in every build), and a coverage or
Tracy build skips the wall-time checks. **Skipped is not failed, so a leg of
Debug builds passes with the whole tier skipped**: that is true of all three
required CI legs, and the budgets run only in the `Performance (Linux, GCC)`
Release leg, which is not a required check, and in the release workflow. The
check that notices is [`profiling.perf_budgets_ran`](#the-trace-tests).

Two things are measured outside that binary, because they need Qt or a window:

- **The scene's work** (`apps/app/tests/unit/scene_script_test.cpp`, tags `[scene][perf]`):
  the length of the longest play (6x6) from the scene's own script, in the same work-budget style.
  It runs in every build (one play, not the tier's corpus).
- **The window's frames** (`apps/app/tests/unit/frame_timing_test.cpp`, ctest names
  `system.frame.*`): the application's `--smoke 240` run in a child process, and
  the report it prints, held to the design's frame **against the period of the
  display the window is on**, not a fixed 16.7 ms: the median at most one period
  and 0.3 ms, the 95th percentile at most one period and 2 ms, the maximum at most
  33 ms and no frame over 33 ms. The period is 1000 / `refresh_hz` from the report
  (60 Hz when the screen does not say), and the report prints `screen=` and
  `refresh_hz=`, so a failure says which screen the window was on. A second case,
  always run, holds the budget's arithmetic (60 Hz is 16.97 ms, 100 Hz is 10.3 ms,
  a window that skips every second refresh of a 100 Hz screen fails). The check is
  opt-in with its own variable, **`SNS_PERF_FRAME_TIMING=1`**, and needs a
  display, so a shared runner never runs it and **it has never run in CI**; it
  also skips a build that is not optimised, one with Tracy or coverage in, and
  the `offscreen` platform plugin (it presents no frames;
  [Profiling](profiling.md#the-smoke-mode)). **On the author's desktop it fails
  when the window lands on the 59.95 Hz screen** (a median of 19.9 ms against
  16.97) and passes on the 100 Hz one (10.0 / 10.5 ms); the cause on the slower
  screen is not found ([the frame on a desktop with two
  screens](profiling.md#the-frame-on-a-desktop-with-two-screens)). The frame
  report itself is printed by `--smoke`, which is a tool as the `.perf-report`
  cases are.

```sh
# on a desktop with a display, in an optimised build
set SNS_PERF_FRAME_TIMING=1
ctest -R "frame budget"        # or: sns_frame_timing_tests
```

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
  `sns::phonics_board_options`. And, for the binder, three paths of nine tiles
  against a test set of 39 graphemes (31 of more than one letter and not split).
- **The game session** (phonics board): the same test phase, 100 sessions of each
  of 3x3 and 4x4, session `i` seeded `7919 * i + n`, each dealt (`shake`) and
  then played until the board is cleared: a find for every word of the board
  (with the help asked for before every other), a word traced again, a touch of
  one tile, a snapshot after the first find, `rest` (the session's `resting`
  state, which the window no longer has a screen for), and the last snapshot;
  then the first snapshot restored.
- **The game session** (standard board): the shipped dice (`sns::DiceSets` over
  `data/dice`) and the full word list, 100 sessions of each of 3x3 to 6x6, seeded
  `7919 * i + n` as the dice rows' boards are, each dealt and played in the same
  way: a find for every word of the dictionary on the board (about 33, 100, 192
  and 314 a board: 3,273, 9,948, 19,165 and 31,396 over the 100 boards of a
  size), with the same help, repeat, touch, snapshots and restore.
  The play and the digest are the same code as the phonics board's, with the
  session's mode the only thing that changes.
- **What the screens wait on besides the solver** (`data_corpus.hpp`): the store
  that twenty played boards leave (the standard mode plays 3x3 to 6x6 in turn,
  the phonics mode 3x3 and 4x4, three boards to a day from 12 October 2026, and
  the store is rewritten whole after every change), the heaviest store the format
  allows (twenty 6x6 boards listing 24 words of twenty letters each, 300 finds,
  1,000 vault words, 35 days), the dashboard built from the phonics board's
  twenty boards, the test phase written as the two files of a data directory, and
  a session in `playing` with one word found, to which a path that spells nothing
  the board holds and the found word are submitted again and again.

### What is counted

| Metric | Budget |
| --- | --- |
| `dictionary.words`, `dictionary.nodes` | Exactly 172,823 and 387,889. They change with the data (`data/README.md`) |
| `solve.visits.max.<n>x<n>` over the 200 boards | A quarter above the measurement: 640, 1,940, 3,580, 6,470 |
| `solve.visits.rich.*` | A quarter above: 3,550, 6,900, 11,800, 15,500 |
| `solve.words.rich.*` | Exactly 1,126, 1,464, 1,788, 327 |
| `solve.visits.empty.*`, `solve.words.empty.*` | Exactly 100 and 256; 0 |
| `solve.digest` | Exactly (`kCorpusDigest`). An FNV-1a digest of every word, score and path over the whole corpus, in the order `solve()` returns them |
| `roll.visits.max.<n>x<n>` over 200 rolls, **standard board** | A quarter above: 589, 2,050, 4,690, 8,080 |
| The dice digest | Exactly (`kDiceDigest`). The same FNV-1a digest over every solution of the rolled boards |
| `seed.tries.total`, `seed.solves.total`, `seed.visits.total`, `seed.visits.max`, `seed.fallbacks` by size 3x3 to 6x6, **phonics board** | A quarter above each: tries 2,000, 830, 243, 234 (a 3x3's are the cap of 100 seeds of 20 tries, so that budget cannot fail); solves 6,880, 27,100, 50,900, 85,100; visits 610,000, 1.30 million, 3.16 million, 6.98 million; the most for one seeding 13,100, 31,700, 121,000, 168,000; fallbacks 97, 7, 0, 0 (`kSeedBudgets` has the measurements; docs/design/performance.md has them too, [the game rows](design/performance.md#the-game-rows)) |
| `seed.tries.max.<n>x<n>`, `seed.out_of_range.<n>x<n>`, `seed.phase.words` | At most 20 (the cap), exactly 0, exactly 249 |
| The seeding digest | Exactly (`kSeedDigest`). A digest of every board seeded: its tiles, targets, dead jewels (listed in row order when a board falls back), the words found, tries and solves. It moved when fallback boards began to list their dead jewels in row order, and moves with the seeding algorithm or the phase |
| `bind.comparisons.<path>.9`, **phonics board** | A quarter above: 349, 63, 234 for a path of no block, all blocks and a mix |
| `session.seedings`, `.tries`, `.solves`, `.visits` (total and most) and `.out_of_range` by size, **phonics board** | A quarter above each: 403 and 133 seedings, 7,240 and 897 tries, 22,000 and 28,600 solves, 1.94 million and 1.38 million visits (32,900 and 31,700 for the worst deal), and at most 44 boards out of range on the 3x3 and none on the 4x4; a session seeds again up to its cap of 5 when a board misses three to five words, so these are the price of that guarantee |
| `session.seedings.max.<n>x<n>`, `session.empty.<n>x<n>`, `session.words.total.<n>x<n>` | At most 5 (the cap), exactly 0 (no board is dealt with no word), exactly 522 and 476 for the 3x3 and the 4x4 (the corpus, the phase and the seeding pinned together) |
| `session.play.solves.<n>x<n>`, `session.play.visits.<n>x<n>`, `session.restore.solves.<n>x<n>` | Exactly 0, 0 and 100: tracing, asking for the help, recording and saving make no solve, and a restore makes one |
| The session digest | Exactly (`kSessionDigest`). A digest of every deal (seedings, tries, solves, words, range), the kind of every trace, and the snapshots after the first find and at rest |
| `session.mismatched.<n>x<n>`, **phonics board** | Exactly 0: every record and snapshot of a session says the mode the session played, the phase, and the count of words the deal made |
| `session.standard.rolls.total.<n>x<n>`, `.rolls.max.<n>x<n>`, **standard board** | A quarter above the measurement: 135, 125, 125, 125 and, at most, 4, 2, 2, 2. Only the 3x3 rolls a second time (3.8% of its rolls hold no word): 108 rolls for 100 boards, the worst taking 3 |
| `session.standard.visits.total`, `.visits.max`, `.restore.visits` by size, **standard board** | A quarter above each: 23,500 / 558 / 23,300 for the 3x3, 71,400 / 1,390 / 71,400 for the 4x4, 202,000 / 4,690 / 202,000 for the 5x5 and 373,000 / 8,080 / 373,000 for the 6x6. The worst deal of a 5x5 and a 6x6 is the dice rows' worst board, so these two are the same 4,690 and 8,080 |
| `session.standard.empty.<n>x<n>`, `.words.total.<n>x<n>`, **standard board** | Exactly 0 (a board that holds no word is never dealt) and exactly 3,273, 9,948, 19,165 and 31,396 words over the 100 boards of 3x3 to 6x6 (the dice, the roll, the dictionary and the deal pinned together; they were 3,432 to 48,121 on the dictionary-count dice before 2026-10-19) |
| `session.standard.play.solves`, `.play.visits`, `.restore.solves`, `.mismatched` by size, **standard board** | Exactly 0, 0, 100 and 0: the play and the save make no solve however many words there are, a restore makes one, and every record and snapshot says its mode and the count the deal made |
| The standard session digest | Exactly (`kStandardSessionDigest`). The same digest over every standard board a session dealt and played (with the rolls and the count of the words) |
| `time.load_dictionary`, `time.solve.<4x4 rich, 6x6 rich>`, `time.deal.<3x3, 4x4>`, **phonics board** for the deals | Opt-in, as the other wall-time rows: loading ENABLE against the heavy calibration (budget 5.0; 0.65 to 0.78 measured), one solve of each rich board against the light one (0.3 and 0.8; 0.042 and 0.115 measured), and one phonics deal (0.6 each; 0.087 and 0.086 measured) |
| `time.deal.standard.<n>x<n>`, **standard board** | Opt-in, as the other wall-time rows: one deal against the light calibration, budgets of 0.03, 0.08, 0.15 and 0.30 (measured 0.003, 0.010, 0.023 and 0.046) |
| `store.boards`, `.finds`, `.vault`, `.save.bytes`, `.write.bytes`, `.write.rewrites` by board mode, `store.heaviest.bytes`, `.of_cap` | The store that twenty played boards leave. Exactly 20 boards; 300 finds and 1,000 vault words (**standard board**: the store is at both its limits, since a board holds hundreds of words) or 95 finds and 48 words (**phonics board**); the file at most 38,800 bytes (31,013 measured) or 5,950 (4,756); what the twenty changes write, a file after each, at most 647,000 or 67,000 bytes (517,233 or 53,550 measured, on the GUI thread), and exactly twenty rewrites; the heaviest store the format allows at most 111,000 bytes, under the format's 128 KiB cap (88,521 measured). A further case reads the store back and holds it equal to what was written |
| `dashboard.sounds`, `.boards_met`, `.words_found`, `.vault`, `.inputs`, **phonics board** | The dashboard built from the phonics board's twenty boards against the test phase: exactly 6 sounds, 81 boards met, 133 words found and 48 stickers, reading at most 204 inputs (163 measured) |
| `phase.load.phases`, `.words`, `.graphemes`, `.bytes`, **phonics board** | The test phase loaded from a directory of its two files: exactly 1 phase, 249 words, 6 graphemes, and at most 5,440 bytes (4,345 measured) |
| `submit.found_by_a_miss`, `.again_not_again`, `.finds_added`, both modes | Exactly 0 each: over 100 submits of each kind, a path that spells nothing the board holds is never a find, a word again is always found again, and the session gains no find |
| `scan.*` | Reading four fixed synthetic frames (a 4x4, a 5x5, a 6x6, a 4x4 under glare). Exact, because they pin the corpus: 4 reads, 4 boards answered, 2,764,800 frame pixels and 691,200 working pixels, and, for the optimised reader, 0 pixel buffers allocated, 225 cells read and 291,600 template comparisons. A quarter above the measurement: 2.6 million warped pixels, 3,240 blobs, 619 die candidates, 282 cells read, 364,500 comparisons and 105 buffer hand-outs reused from the reader's scratch pool; a warm read allocates no pixel buffers, so buffers and their most are held at 0 (`sns/scan/work.hpp`). Reading seeds and solves nothing, so these rows carry the default, standard, board mode |
| `time.read.4x4`, `.5x5`, `.6x6`, `.glare` | Opt-in, as the other wall-time rows: one read of a frame against the heavy calibration, budgets 3.0, 4.0, 4.0 and 4.0, seven to eight times the measurement. A read is on the GUI thread, so what it costs is the stall the player sees |
| `time.store.serialize`, `.parse`, `.save`, `time.dashboard.build`, `time.phase.load`, `time.submit.<mode>`, `time.path.validate` | Opt-in: the store's text of the heaviest store, its parse and its save to a file, the dashboard, the phase directory, 200 submits of each kind and 200 validations of 12 tiles, against the light calibration (the save and the phase directory against the heavy one, since they touch a disk); budgets 0.4, 0.8, 1.0, 0.15, 0.5, 0.08 and 0.02, about ten times the highest measurement and, for the two that touch a disk, about fifty |
| `scene.play.6x6` (tags `[scene][perf]`, `apps/app/tests/unit/scene_script_test.cpp`) | The longest play (6x6): the shake's 600 ms, the lid's 250, the drop (35 gaps of 700/36 ms and a die's 380) and the settle's 150, counted from the script: at most 2.2 s for 2.06 |
| The window's frame (`apps/app/tests/unit/frame_timing_test.cpp`, not a metric of the tier's binary) | Opt-in with `SNS_PERF_FRAME_TIMING=1` and a display: the application's smoke report (`p50_ms`, `p95_ms`, `max_ms`, `over33`, `refresh_hz`) against the display's period (1000 / `refresh_hz`) plus 0.3 ms for the median and plus 2 ms for the 95th percentile, and 33 ms and zero frames over it for the worst: 16.97 and 18.67 ms on a 60 Hz screen, 10.3 and 12 ms on a 100 Hz one |

Every metric is for one **board mode**, and the report and a failure say which.
The standard board is the default mode; the phonics board is a parent's switch,
so its rows are not the standard board's work and vice versa. A **verification**
is one solve of a candidate board; a **solve** is any call to `sns::solve`,
including the guided fill's trial of a letter on a tile; the seeding reports
both (`SeededBoard::tries`, `SeededBoard::solves`) and the performance case
holds `solves` to the solver's own tally. A **comparison** is a grapheme held
against the letters at the start of a block; the binder tallies them in
`sns/binder_work.hpp` once per bind, and a case recounts them from the blocks
that came back.

A **visit** is a tile step that stayed inside the dictionary's trie: the
letters of the path so far, with the tile's own, still begin some word. The
solver counts them in a member of its search and adds the total to a
thread-local tally (`sns/solver_work.hpp`) once as the solve ends, so the cost
is one add a solve and nothing per visit; nothing in the application reads it.
`sns::solve`'s signature is as it was. A separate case holds the solver's tally
to a second walk written out in the test with nothing but the dictionary's
trie, so the budgets are held to a definition and not to an implementation.

Every threshold is in `libs/core/bench/budgets.hpp`, with the figure it is above
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
`Performance` leg ([CI](ci.md)), which is **not** one of the required checks; the
Debug legs, which are, prove only that the tier compiles warning-free.

### The trace tests

`profiling.zone_names` reads source text and cannot tell a zone in code that never
runs, or a name chosen at run time (the scene's stage zones), from one a trace
has. Two tests run the program with the Tracy capture attached, export the trace
and read it (`tests/profiling/check_trace.cmake`, with `run_capture.cmake`):

| Test | Runs | The trace must hold |
| --- | --- | --- |
| `profiling.trace_cli` | `sns-cli --repeat 1000000` on a 4x4 board, ended by its timeout after twelve seconds of capture (a Debug build with the sanitizers takes seconds to load the dictionary, so the run is not sized in repeats). Port 18086 | The zone `solver.solve` and the plot `solver.visits` |
| `profiling.trace_app` | `SnapAndSpell --smoke 120` on Qt's offscreen platform, which waits up to five seconds for a profiler so that the start-up zones are in the capture. Port 18087 | The zones `app.startup`, `app.load_qml`, `data.load_dictionary`, `data.load_store`, `data.save_store`, `board.deal`, `board.roll`, `solver.solve`, `scene.step`, `scene.prepare`, `scene.play`, `scene.show`, `scene.shake`, `scene.drop` and `scene.settle` (the last three are chosen by `BoardScene.stageZone()`, so only a run shows they are emitted), and the plots `solver.visits`, `ui.frame_ms`, `ui.first_frame_ms`, `ui.sync_ms`, `ui.animate_ms` and `render.frame_ms`. `render.record_ms` is not asked for: the software renderer has no render pass to record |

They need a build with Tracy (every Debug build has it, and the `*-profiling`
presets) **and** the capture tools, which are built by the same vcpkg feature;
anywhere else they report themselves skipped (exit 4) with the reason, as they do
when the window presents no frame (exit 3 of the program). Each uses a port of
its own, because every Tracy client listens on 8086 and a capture started with
only an address attaches to whichever program started first
([Profiling](profiling.md#which-program-the-capture-attaches-to)); they run alone
(`RUN_SERIAL`, a 240 s timeout, labels `system;profiling`). The Linux Debug legs
run `trace_cli`, the Windows Debug leg both. A tool that dies with an illegal
instruction fails the test and says why (`profiling.tracy_portable` is the check
that keeps the overlay port from building for the runner's own CPU). **Not
verified:** `trace_app` was run in an optimised build when it was written; it was
not run in a Debug build with the sanitizers, whose smoke run is slow.

**`profiling.perf_budgets_ran`** (a `quality` test, `tests/profiling/check_perf_ran.cmake`)
answers a different question: the performance work budgets skip in a build that is
not optimised, and a skipped case is not a failed one, so a leg of Debug builds
passes with the whole tier skipped. This test runs `sns_perf_tests "[perf]~[timing]"`
and counts the cases that ran:

- In an optimised build (Release, RelWithDebInfo, MinSizeRel), or in any build
  where the environment variable **`SNS_REQUIRE_PERF_BUDGETS`** is set to
  something other than empty or `0` (a workflow saying "this leg holds the
  budgets"), it **fails** when no case ran, or when any case skipped, since the
  only reason the tier stands aside is that a build is not optimised.
- Otherwise it reports itself skipped (exit 4) with that reason, so it is never a
  pass that checked nothing.

**No workflow sets `SNS_REQUIRE_PERF_BUDGETS`**, and the leg that is optimised is
not a required check, so today no required check holds a work budget
([Performance requirements](requirements/performance.md)). Making the Release
leg required, or setting the variable on a leg that must hold them, is the fix.

### Capturing it

A short run of `sns-cli` is over before a profiler attaches; the tier's binary
runs for seconds, which makes it a good thing to capture:
[Profiling](profiling.md#capturing-a-trace).

## The scanner

`sns_scan_tests` is a binary of its own, linked against `sns::scan`, `sns::lib`,
`sns::platform` and the synthetic corpus (`sns_scan_synth`), so that the corpus and its
renderer stay out of `sns_unit_tests` and the scanner can be built and run alone
([Scanner design](design/scanner.md) is the page about what it holds and why).

| Test file | What it holds down |
| --- | --- |
| `scan_geometry_test.cpp` | Hulls, the four-sided outline of a convex shape (a rounded rectangle reduces to the rectangle's corners; a triangle to nothing), which corner is first, the projective map and its inverse and the cases where none exists (corners on a line, to within rounding or exactly, or not numbers; the same shapes at a millionth of the size and at a thousand times it; an inverse judged against the size of the matrix); points that are not places are no part of a hull |
| `scan_image_test.cpp` | When a frame is valid, and that a hostile size (2 to the 62nd) cannot wrap the arithmetic that checks it and the luma of an invalid frame is an empty picture and not an overrun; luma of every pixel format, in either byte order and with a stride; shrinking (and a huge factor not overflowing a block's sum), sampling (a point off the picture still gives a grey), blurring (any radius keeps a constant picture constant), a negative size being an empty picture; the local threshold (a smooth gradient is bright nowhere inside the picture); blobs (4-connected, area, hull, a hole not counting, none found in a mask shorter than its picture); a threshold window under a pixel being a pixel; the buffer count |
| `scan_glyph_test.cpp` | The stroke font (26 letters and Qu, each in its box) and the glyph reader on a drawn die, no board and no perturbation: every glyph at every quarter turn and at awkward angles, the turn found, a lighter or heavier hand, no ink where there is none, a letter washed out to a few grey levels not read, a gradient of light, a flat patch having no glyph even when no contrast is asked for, a cell that is not a place on the board read as nothing, and reading a cell twice giving the same answer |
| `scan_finder_test.cpp` | The board found at its true corners in a frame of each condition; a bright rectangle a die's size and a pitch from the board not taken for part of it; the cells counted from the comb for 4, 5 and 6; what the finder knows narrowing the count; the centres of the dice found from their gaps; which corner is first; nothing found in a blank frame, a gradient or a room with a sheet of paper |
| `scan_reader_test.cpp` | `read_board` end to end: the board that was drawn, the same picture as RGBA and with a padded stride, determinism, a given size, the doubt thresholds, a given glyph height (every cell read once and no samples), each error (`board_not_found`, `size_not_found`, `no_letters`, and `invalid_options`, produced by options out of range and checked before the frame is looked at), a frame whose sizes would wrap the arithmetic refused and not read, a lattice of blank dice and a tiled floor not taken for a board, and the counts a read keeps (a warm read allocates no pixel buffers) |
| `scan_confirmation_test.cpp` | `BoardDraft`: from a reading, the doubted and unfilled tiles in order, a correction kept upper-case, each refused letter, accepting, clearing, a quarter turn each way and four of them, a corrected size, a typed-in grid, `confirm()`; `try_from_reading` returning an error for a reading that is not a grid (with the unchecked `from_reading` giving a safe blank for it); a correction upper-cased by ASCII and nothing else; the whole flow from a reading to the words on the board; and `static_assert`s that a `BoardReading` cannot become a `Board` |
| `scan_platform_test.cpp` with `platform_contract.hpp` | The `camera` and `board_reader` interfaces held to their contract with no platform named: not capturing at start, an unlisted camera cannot be started, each error has words, the portable reader is the library's reader unchanged, a frame the camera hands out is one the reader can be given; and the **platform contract suite** ([below](#the-platform-contract)) |
| `scan_corpus_test.cpp` | The accuracy target on the held-out split (below), the judge's own cases (a board numbered from the wrong corner counts as wrong, the turn is the one the corners give), a smoke subset, and the **recorded-frames harness and the PGM reader**, both self-tested on frames the corpus draws |

```sh
ctest --preset windows-msvc-release -R scan              # the scanner's tests, the corpus included
build/windows-msvc-release/bin/sns_scan_tests "[smoke]"  # the subset that runs in Debug too
build/windows-msvc-release/bin/sns_scan_tests "[.scan-report]"   # what the corpus measures now
```

**The target** is five numbers a condition (the board found, correct, silent errors,
flagged, offered) and a count of boards turned wrong, over the synthetic corpus: four
conditions (upright, angled, loose dice, glare) of 24 boards each, and frames with no
board. It runs in an optimised build only (the Release `Performance` leg of CI runs it on
every push; that leg is not a required check); a Debug build with the sanitizers **skips**
those five cases (the four conditions and the frames with no board), as the performance
tier skips, and runs a smoke subset (a board of each condition, a frame with no board of
each kind) so the pipeline runs clean under ASan.

**The bounds are derived, not chosen.** They were once chosen by looking at the split they
were then held against, and three were relaxed after the first run on it missed; a bound
set that way says the reader passes, not that it generalises. Now:

- `kTuning` in `scan_corpus_test.cpp` records, as counts, what the reader gives on the
  **tuning** split, per condition (frames, found, tiles, correct, flagged, silent,
  offered). Nothing else is looked at to set a bound.
- A bound is such a figure moved to its unfavourable side by three standard errors of an
  adjusted proportion and rounded outward to the next half percent (`derive()` in the same
  file). With 24 boards a condition (558 tiles) one tile in a thousand is about the finest
  the test can tell, so the derived bounds on silent errors are of the order of 1.0% to
  2.0%: the design's targets of 0.3% to 0.5% silent **cannot be confirmed or refuted by
  this corpus, and are not claimed**.
- The bounds are held against a third split, **`heldout`** (base seed 6,000,000), that
  nothing is chosen from. If a bound is missed there the test is red and the reader is not
  changed until it passes by looking at that split. A reader change is made against the
  tuning split, the figures in `kTuning` are re-recorded from `[.scan-thresholds]` with the
  reason in the commit message, and a held-out split that has been looked at to steer is
  retired and replaced with a new base seed (`seed_of` in `libs/scan/tests/synth/scene.cpp`).
  The first reading of the split found the board in all 24 of every condition with no
  rotation error; the angled condition's correct rate was 0.0017 over its bound.
- **Rotation is counted.** The turn of the board is judged by its corners, not by
  whichever turn of the truth fits best (that scored a board numbered from the wrong
  corner as right), and no rotation error is allowed in any condition: one is a fault in
  how the corners are numbered, not noise.
- The frames with no board (three kinds, seven of each) must be answered with an error at
  least 95% of the time and none may be believed (most of its tiles trusted).

The corpus is the same frames on every machine: `std::mt19937` taken modulo, a seed from
the condition, the split, the size and the index. Three splits: **tuning**, which the
reader is developed against and the bounds and doubt thresholds are derived from;
**test**, which was read and reported on while the reader changed, so is no longer held
out and is kept for the tools and for the frames other tests draw; and **heldout**, which
the target tests read. The hidden cases are the tools (they assert nothing, and read the
tuning split unless `SNS_SCAN_SPLIT=test` or `heldout` is set; reading `heldout` with a
tool is looking at it):

| Case | Prints |
| --- | --- |
| `[.scan-report]` | The five numbers of each condition, and what drawing and reading the frames cost |
| `[.scan-thresholds]` | The figures to record in `kTuning` when the reader changes, the bounds they derive, and whether `kTuning` is as recorded. Always the tuning split: the one place bounds are set from |
| `[.scan-tune]` | For each bound on silent errors, the least doubt that stays under it: where to choose the thresholds |
| `[.scan-confusions]` | Each wrong tile as truth -> read, how often each true letter is right, and why a frame was not read |
| `[.scan-oracle]` | The glyph step alone, given the true corners and centres: what it gets right when the geometry is perfect |
| `[.scan-negatives]` | How many frames with no board were answered with an error |
| `[.recorded]` | The same numbers over real frames kept outside the repository, from `SNS_SCAN_RECORDED_DIR` (grey PGMs with a `.txt` truth beside each). Skips when unset, and **has not been run on anything real**: its harness is tested (below), the first session on a real board is not to be spent finding out that it cannot read a header |
| `[.glyph-print]` | What each clean upright glyph is read as, with its scores (`scan_glyph_test.cpp`) |

**The recorded-frames harness is self-tested.** `the recorded-frames harness reads a
picture with its truth and judges it` writes frames the corpus draws the way a camera tool
writes them (one plain, one with comments in the PGM header and the fields on lines of
their own), with files that must be passed over, each for its own reason (no truth beside
it, a truth that is not square, a picture cut short, a 16-bit picture, a file that is not a
picture), and holds the harness to reading two, judging them, listing the four it
skipped and counting no rotation error. `a picture file is read only if it is a whole grey
PGM` holds the reader of the format to its refusals. Neither says anything about real
frames: none has been read by anything in this repository.

`sns_scan_render --out <directory>` writes the corpus as PNG files with the truth beside
each, and `--sheet <file.png>` a contact sheet, to look at. It is a one-off tool,
not part of the default build: `cmake --build <build> --target sns_scan_render`
makes it. **Recorded camera frames are
never committed** (`.gitignore`, [Licensing](licensing.md#camera-input-and-letter-recognition)).

**`scan.privacy`** runs `libs/scan/tests/check_privacy.cmake`: the scanner's sources, the
camera and `board_reader` files of every platform, and Mode A's camera source and
its backends may not include a file or stream header, use a file class or API,
capture audio or use a network class (a frame is never written, nothing is
recorded, nothing is sent). It holds four rules: file output, frame capture (no camera
class is asked to keep a picture), audio capture and network. A comment line is skipped.
**`scan.privacy.catches_a_leak`** is the check's own test (`check_privacy_selftest.cmake`, no
longer a `WILL_FAIL` on the check, which would pass on a syntax error in the script): there
is a fixture tree for each rule (`privacy_fixture/libs/scan/leak.cpp` opens an `ofstream`;
`frame_leak`, `audio_leak` and `network_leak` each do the one thing their rule forbids), and the
test requires the check to fail on each, to name that rule and that file and no other rule;
it also runs it on a tree that does nothing wrong (`clean`, which has to pass and be counted)
and on an empty one (`empty`, which has to be refused and not pass for lack of anything to
read). Both only read source, so they run on every host.

### The platform contract

`libs/scan/tests/platform_contract.hpp` holds what any implementation of the `camera` and
`board_reader` interfaces has to keep true, as functions that **return the violations**
(a sentence naming the rule) and do not assert, so that a test can also show the contract
is not vacuous. `scan_platform_test.cpp` runs it: a camera that keeps the rules passes;
a platform with no camera keeps the contract by saying so; each rule of the camera
contract is found out when a fake breaks it; the mailbox holds the latest frame, once, and
only while capturing; and the platform's own camera is run through the same contract; the
board reader likewise (one that keeps it, each rule found out when broken, the platform's
own). The iPad's camera and reader, when they are written, are one more call each. **They
are not written:** `libs/platforms/ios` has no camera or reader, so the contract has been
run on the desktops' stubs, the portable reader and fakes only.

### The camera

The desktop camera is tested **without a camera**, which is the point: a device
must not be needed to prove the state machine, and no live picture may be faked.

| Where | What it holds down |
| --- | --- |
| `apps/app/tests/unit/camera_test.cpp` (`sns_app_tests`, `[camera]`) | An image is copied into the scanner's RGB24 frame, packed row by row and with the QImage padding gone; a grey picture becomes equal channels; a null picture is no frame. A `CameraSource` over a fake backend says `no_camera` with no device, `permission` when refused, hands a frame over once, and reports a failure that happens later. A `ScanModel` with a supplied source opens the check screen from a frame, returns to `ready` with `no_frame` when the frame never arrives, and says `no_camera` with no frame when the source cannot start |
| `apps/app/tests/unit/camera_backend_test.cpp` (only where the kit has Qt Multimedia, `[app][camera][backend]`) | The Qt Multimedia backend built headless: the camera's frames are forwarded into the preview item's own sink; an item with no sink, or a sink destroyed, is not written to; the backend attaches a preview, replaces it and lets it go; a refused camera is a permission problem, said once, and does not start; a camera not yet asked about asks and waits for the answer, and an answer that comes after a stop does not start it; with nothing to capture it is safe to stop and to read |
| `apps/app/tests/unit/scan_model_test.cpp` (`sns_app_tests`, `[app][scan][camera]`) | `ScanModel` and a camera source that fails, never delivers a frame or is replaced: see the row in [What there is](#what-there-is) |
| `apps/app/tests/unit/camera_video_frame_test.cpp` (only where the kit has Qt Multimedia) | A `QVideoFrame` made from a picture goes through `QVideoFrame::toImage()` and the shared conversion to the same RGB24 frame — the backend's step, without a device |
| `camera.screens` (`apps/app/tests/unit/scan_ui`, only where the kit has Qt Multimedia) | The real `ScanScreen` and `ScanModel` in the offscreen stub shell with a fake source and one synthetic board: the live `VideoOutput` is created and shown, Capture is offered, the frame reaches the check screen, and the camera stops. The no-camera check (`scan.screens`) is the other path. Skipped (exit 77) where Qt Multimedia has no backend on the machine ([Camera tests without a media backend](#camera-tests-without-a-media-backend)) |
| `quick.scan` (`apps/app/ui-tests/tst_scan.qml`) | The same preview on and no-preview path through the QML suite, with `fixture.useCamera()` supplying a fake source; the window is offscreen and no device is opened |

The tests never use a device and never write a frame: the fake backend returns
an owned frame made in the test, and the synthetic frame comes from
`libs/scan/tests/synth` (drawn from a seed, not recorded). On this machine there is
no camera at all, so the no-camera path is what the program was actually run
on; the live device path is the one thing that waits for a webcam.

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
throw-away data and holds the checker to a table of 54 cases (the ones in
`phase_data_test.cpp`, so the two implementations agree). A gate that passes
everything is worse than none, so the gate is itself tested; it takes half a
minute because each case starts a PowerShell.

**The review gate.** A file with no `reviewed-by`, `reviewed-on` (a date,
`YYYY-MM-DD`) and `source` is test data. Test data need not say where it came
from; a file that claims a review has to, as `data/README.md` asks of every
list. `SNS_REQUIRE_REVIEWED_PHONICS` makes the
build refuse it: with the switch on, the target `sns_phonics_review_gate` runs
`tools/data/check_phonics_review.cmake` before the library is built, every
time (it is always out of date), so a file edited after configuring is still
caught and building any target that links the library is gated. It is a CMake
script, so a release build needs nothing but CMake. It applies the header rules
and the file-name rule of the PowerShell checker, and only those; the lines of
the data are `data.phonics`'s, which passes `-RequireReviewed` too when the
switch is on. The switch is on by default for `Release`, `RelWithDebInfo` and
`MinSizeRel` and for any multi-config generator (which has no build type at
configure time), and the release presets set it. Turning it off is an explicit
`-DSNS_REQUIRE_REVIEWED_PHONICS=OFF`, which is on the command line of whoever
did it. The gate is held to a table of cases on throw-away data
(`data.phonics.review_gate.selftest`, always) and, with the switch on, to the
data there is (`data.phonics.review_gate`). With no `data/phonics` there is nothing
to review and the gate passes, which is the case today. A half-given review (a reviewer and no date, or a review and no source)
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

The project's dice (`data/dice`) are checked by `dice_data_test.cpp`, which rolls
300 boards a size (seeds 1 to 300) and solves each against the dictionary, and by
the quality cases about the sets themselves. One Windows 11 desktop, Release.

The table is from the dictionary-count dice, before 2026-10-19, and is kept as
the comparison:

| Board | Words a board holds, mean | 10th / 50th / 90th percentile | Boards with no word | Vowels (A E I O U) among tiles |
| --- | --- | --- | --- | --- |
| 3x3 | 30 | 7 / 26 / 58 | 3.3% | 30% |
| 4x4 | 105 | 41 / 98 / 182 | 0.3% | 29% |
| 5x5 | 254 | 137 / 243 / 389 | 0% | 40% |
| 6x6 | 471 | 291 / 458 / 669 | 0% | 38% |

Those sets are no longer shipped. The Scrabble-style sets measure about 32, 102,
194 and 322 words a board on average, and about 35%, 34%, 36% and 36% of their
faces are vowels. The larger boards hold fewer dictionary words because S is
capped at 8% of the faces rather than being a tenth of them. Every shipped size
has `QU` tiles; Q is the tile `QU`.

## Sanitizers, warnings and Debug

Debug builds run the tests under AddressSanitizer (and UBSan on GCC and Clang),
and every first-party target is built with warnings as errors. A sanitizer
finding aborts the test. Details, and how to turn them off, are in
[Building](building.md#sanitizers-and-warnings). The nightly Clang leg runs the
same suite with `clang++` ([CI](ci.md#nightly-analysis)).

### Why the Windows tests set `ASAN_OPTIONS`

MSVC's AddressSanitizer records a 30-frame stack for every allocation and every
free, so that a report can say where a block was allocated and freed, and it
unwinds each with `RtlCaptureStackBackTrace`. A test that allocates much (the
solver, the deal, strings) spent four fifths of its time there, measured by
sampling: a 4x4 chunk of the deal test took 18.1 s with the default and 3.7 s
with two frames (3 frames: 7.5 s; 5: 8.8 s; 10: 9.6 s). The `windows-msvc-debug`
test presets therefore set `ASAN_OPTIONS=malloc_context_size=2`
(`CMakePresets.json`, the hidden preset `windows-asan`).

What this does not change: every bad access, use after free, double free and
overflow is found as before, and reported with the full stack of the access. What
it cuts is the "allocated by" and "freed by" stacks in the report, to the
allocating function (`operator new` and its caller, with file and line). To get
the whole stacks for a failure, run the test again with the variable set after
the preset's value, which wins:

```sh
ASAN_OPTIONS=malloc_context_size=30 ctest --preset windows-msvc-debug -R "the name of the test"
```

(Run the program directly, outside `ctest --preset`, and the default of 30
applies.) `detect_stack_use_after_return`, the quarantine size, the red zone and
`fast_unwind_on_malloc` made no difference to the time.

The other cost under ASan is by value copies: with the sanitizer a copy of a
24-byte vector is a call into the runtime (`__asan_memcpy`), so the dice
simulation (`apps/app/src/scene/sim`) passes its vectors and quaternions by reference
and writes the arithmetic of its inner loop out by component. The digests of its
plays, which the tests compare, are the same as before.

### Test cost: where the Windows Debug run spends its time

The Windows leg runs every case under ASan, a case is a program started afresh
(`catch_discover_tests`), and the leg has four cores. Measured on a four-core
run (`ctest -j 4` with the process held to four cores of a larger machine, no
timing from an earlier run), each change on top of the one before:

| | wall | sum of the cases |
|---|---|---|
| the tree before | 613 s | 1,994 s |
| allocation stacks of two frames, the dice world by reference, the long cases split | about 357 s | 1,275 s |
| the two trace tests beside the others, not alone | about 321 s | 1,170 s |
| first-party code built with `/O1` (`SNS_SANITIZER_OPTIMIZE`) | about 274 s | 970 s |
| Tracy's Debug client does not calibrate its timer | about 204 s | 710 s |
| the longest cases are started first (`COST`) | about 182 s | 715 s |
| the same, after merging `develop` (two runs, 1,144 cases) | 194 s | 727 s |

What each one is:

- **Allocation stacks** and **by-value copies**: the section above.
- **Long cases.** A case that sweeps seeds is several cases, a range of seeds
  each, when each seed's checks stand alone (the 600 deals of "a board is never
  dealt with no word on it", the output of `sns-cli` 60 times over, the groups of
  checks about the scene's script). A case that keeps a statistic *over* its seeds
  stays one: the median of how far sixteen dice were shaken, the median of how
  long the guide took, the share of the 18 face-up runs that were late, the count
  of seedings that kept a later board. Splitting those would hold each part to
  the bound the whole is held to, which is a different test.
- **The trace tests** ran alone (`RUN_SERIAL`), 48 s with three cores idle. They
  take a lock between themselves (`RESOURCE_LOCK`) and run beside the others;
  they passed in each of the eight full runs made beside the others.
- **`/O1`.** ASan makes every access to memory a check and every copy of a struct
  a call, and unoptimised code has many of both. `-DSNS_SANITIZER_OPTIMIZE=OFF`
  brings back `/Od` to step through the code in a debugger. Two tests that skip
  themselves in a build that is not optimised read their flag through a
  `volatile`, since an optimising compiler that sees the flag is constant warns
  of unreachable code (C4702) after the skip.
- **The Tracy client** (a Debug build has it, `cmake/Tracy.cmake`) calibrates its
  cycle counter with a sleep of 200 ms before `main`, in every program: 0.27 s to
  start a test program, 0.09 s without. The Debug client uses the clock instead
  (`TRACY_TIMER_FALLBACK`, `TRACY_DISALLOW_HW_TIMER`, in the port and in
  `cmake/Tracy.cmake`, which have to agree: the timer is read inline in Tracy's
  headers). A Debug build's timings are not numbers about the code; the
  `*-profiling` presets, which numbers are taken from, keep the cycle counter.
- **`COST`.** With no timing from an earlier run, as on a fresh checkout, ctest
  starts the cases in the order they are registered, and the last to start was the
  longest. The cases of 15 s or more have a `COST`, so that they start first.

What is left is mostly the programs themselves: `release.tooling` and
`data.phonics.checker` (a PowerShell process for each of their cases, 35 to 60 s
each), the eight languages of `parent.languages` and `scan.languages`, and the
face-up sweep of the dice (50 s).

## Static analysis

clang-tidy and MSVC's `/analyze` are not ctest entries: they are the nightly
`analysis.yml` jobs, with the checks, the disabled list and the commands to run
them locally in [Building](building.md#static-analysis). A finding there is not
a failing unit test, but it is still a defect report against the tree.

## Coverage

The gate in CI is per component, from `tools/coverage-floors.json`: each of `libs/core`,
`libs/scan`, `libs/platforms` and `apps/cli` has a minimum for lines and for branches (the default is
90% and 80%). The figures are the unit, integration and system tiers together, measured by gcovr on
`Coverage (Linux, GCC)`. A change that drops a component under its floor fails CI. A file in no
component also fails, so new code cannot sit outside the table. A figure more than a point above
its floor is printed as a hint to raise the floor: a floor only goes up, in a pull request of its own.

The window (`apps/app/src`) is not gated: its C++ is built only where Qt is installed, and the UI
tier's coverage is reported separately (`coverage-ui`), because it shows what the QML reaches rather
than what the library's tests do. The local `coverage-unit` target keeps the older single gate of
90% and 80% on `libs/core` for the unit tier alone.

Two instrumentations, chosen by the compiler:

| Preset | Compiler | Instrumentation | Report tool |
| --- | --- | --- | --- |
| `linux-gcc-coverage` | GCC (or Clang, GNU command line) | `--coverage -fprofile-update=atomic -fno-inline` | `gcovr` (pinned in `.github/requirements/coverage.txt`) |
| `windows-clang-coverage` | clang-cl, with the Visual Studio generator | `-fprofile-instr-generate -fcoverage-mapping /Ob0` | `llvm-profdata` + `llvm-cov` |

```sh
# Linux (GCC): the CI job's own commands
cmake --preset linux-gcc-coverage
cmake --build --preset linux-gcc-coverage --target coverage-unit

# Windows (clang-cl): the same gate, with llvm-cov
cmake --preset windows-clang-coverage
cmake --build --preset windows-clang-coverage --target coverage-unit
```

`coverage-unit` runs the `unit` label, empties this tier's counters first (so a
previous UI run cannot raise the number), writes the report and then fails if
the floor is missed. `coverage-ui` does the same for the `ui` label and reports
`apps/app/src` - the C++ the Qt Quick Test suites drive - with no floor; `coverage`
runs both, one after the other. In a build with the window, the UI report is the
demonstration that the suite exercises the application's code through the UI,
not only through its classes.

The report is written to `build/<preset>/coverage/<tier>/`:

- `summary.json` - the same keys for both tools: `line_percent`,
  `branch_percent`, and the covered/total counts;
- `coverage.md` - the table for a CI step summary;
- `report.txt` - the per-file text report;
- `html/` (llvm-cov) or `index.html` (gcovr) - the browsable report.

The gate has been measured on both toolchains. The first run (2026-10-19) is
before the release-readiness work on the library; the second is the last
measurement, **at the time of writing**, on the clang-cl toolchain only (Debug,
the application not built, tests green):

| Toolchain | Lines | Branches |
| --- | ---: | ---: |
| GCC 15.3 with gcovr 8.6 (`gcc:15`, the CI job's compiler family), 2026-10-19 | 94.8% (3521/3713) | 86.1% (3369/3913) |
| clang-cl 22.1 with llvm-cov (`windows-clang-coverage`), 2026-10-19 | 94.9% (4471/4707) | 86.6% (2339/2698) |
| clang-cl 22.1 with llvm-cov (`windows-clang-coverage`), at the time of writing | 95.1% (4776/5017) | 87.8% (2470/2813) |

The counts differ because the two compilers map regions, template
instantiations and exception edges differently; the floor is the same on both
(90% lines, 80% branches). The GCC figure has not been re-measured since the
library grew, and the CI job is what holds it.

The **UI report** from the same kind of run (clang-cl, 2026-10-19, before the
screens were redesigned and before the later suites existed, and **not
re-measured since**) covers `apps/app/src`: 1,646 of 2,680 lines (61.4%) and 538 of
1,115 branches (48.2%) were reached by the eight Qt Quick Test suites then and the
two stub shells. The per-file
table is the point: `ParentController` 78.9%, `ScanModel` 62.7%,
`SoloModel` 56.7% and the layout rules 100% of their lines are executed through
the QML bindings and the tests' presses, not by a direct C++ call. The report
has no floor; a run that reaches nothing is a build problem, not a number to
lower.

The thresholds are CMake cache variables (`SNS_COVERAGE_MIN_LINE`,
`SNS_COVERAGE_MIN_BRANCH`), and the scopes are a tier's; raising a floor above
what the tier reaches is a test-writing job, not a configuration one. The
coverage build is Debug with the sanitizers off (`cmake/Sanitizers.cmake`,
`cmake/Coverage.cmake`): a sanitizer runtime and a coverage counter in the same
process make both numbers untrustworthy.

## The requirements matrix

[Requirements and traceability](requirements.md) gives every requirement of the
product requirements and the design pages a stable ID (`R-GAM-07`) and one row:
what it requires, its status (Implemented, Partial, Decided, Deferred, Not built
or Doc only), the files that build it and the tests that show it, with the notes a
reader needs to trust the row. There are seven pages under `docs/requirements/`;
at the time of writing 453 rows, 31 of them Implemented or Partial with no test at
all. Two `quality` tests keep it tied to the tree. Both read text only, so they
run in every CI leg, need no PowerShell and no display, and are registered in
`tests/checks/CMakeLists.txt`.

**`requirements.matrix`** runs `tests/requirements/check_requirements.cmake`. It
fails when:

- a page has no `Verified at <date>, commit <sha>` line;
- a row is not six cells, has an ID that is not `R-<AREA>-<n>` (with an optional
  lower-case letter), repeats an ID used anywhere on the pages, or has a status that
  is not one of the six words;
- a `` `path::name` `` reference in "Demonstrated by" names a file that does not
  exist or a name that is not in that file **as written** (a Catch2 case name, a
  QML `test_` function, or the ctest name in the CMake file that registers it);
- a path in "Implemented in" does not exist;
- the figure "Rows with no test: N" in `docs/requirements.md` is not the number of
  Implemented or Partial rows that cite no test, so the number cannot be edited
  quietly (it falls by a test being cited and is meant never to rise).

**`requirements.matrix.selftest`** holds that check to fourteen cases on
throw-away trees (a row whose test exists, two references in one cell, a deferred
row needing no test, and, one fault at a time, a test that is gone, a test file
that is gone, a status that is not one, an ID twice, an ID that is not one, an
implemented-in path that is gone, a page without its verification line, an untested
count that grew or was not lowered, and a row that is not six cells), so a
check that stopped matching would be a failure and not a pass.

**What they do not show.** A cited test **exists**; the check does not run it, does
not know whether it passes or is skipped in a leg (the performance budgets are
cited and skip in every required leg), and does not read whether it proves the row:
that is what the status and the notes are for, and what a reviewer of a change to a
row is for. The name only has to appear in the file, so a comment containing it
would satisfy the check. It does not check the commit in the "Verified at" line
against git, does not read "Implemented in" files for what the row says they do, and
does not find a test no row cites. The status of a row is a person's reading of the
code at the commit named, not something the check re-derives. When you rename a
test, `ctest -R "^requirements[.]"` names the rows to correct; when you change a
default or a design number, change the row in the same commit.

## Adding a test

- Put the file in `libs/core/tests/unit/` (library and platform interface), `libs/scan/tests/`
  (the scanner and its interfaces), `apps/app/tests/unit/` (the window's model),
  `libs/core/bench/` (a performance case) or `apps/app/tests/unit/` (the scene's script),
  and add it to the executable's source list in **that directory's**
  `CMakeLists.txt` (`libs/core/tests/unit/CMakeLists.txt`, `apps/app/tests/unit/CMakeLists.txt`, ...).
  `tests/CMakeLists.txt` holds only what the directories share (Catch2, the
  variables, the order they are added in). The lists are explicit, not globbed,
  except that a Qt Quick Test suite is any `apps/app/ui-tests/tst_<suite>.qml`, which
  becomes `quick.<suite>` with no edit.
- Put it in the tier it belongs to: a new library case is `unit`; one that needs
  Qt, a store file or the scanner's corpus is `integration`; one that starts a
  program or the installer is `system`; one that drives QML is `ui`. The tier's
  registration is in the directory's own CMake file (`TEST_PREFIX` and `LABELS`
  for Catch2, `LABELS` for an `add_test`); a new tier entry is one line there, not
  a new build. A check about the tree, the build or the data (a script that reads
  files) is registered in `tests/checks/CMakeLists.txt` and added to
  `SNS_QUALITY_TESTS`, which gives it the `quality` label; its script lives in the
  topic directory beside it (`tests/privacy/`, `tests/toolchain/`, ...), and a
  checker that can pass everything is worse than none, so it comes with a
  `.selftest` that holds it to cases on throw-away trees.
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
- A test the [requirements matrix](#the-requirements-matrix) cites by name is
  found by `requirements.matrix`: renaming or deleting it fails that check, which
  names the row to correct.

## What is not tested

The full list, row by row and with the test that would close each gap, is the
"Remaining gaps" of each requirements page: [board, dice and
shake](requirements/board.md#remaining-gaps), [game, tracing and
progress](requirements/game.md#remaining-gaps), [phonics and the word
library](requirements/phonics.md#remaining-gaps), [scanner and Mode
A](requirements/scanner.md#remaining-gaps), [parent area, dashboard and
privacy](requirements/parent.md#remaining-gaps), [application, audio, languages
and platforms](requirements/application.md#remaining-gaps) and
[performance, constraints and
licensing](requirements/performance.md#remaining-gaps); the index
[summarises the ones that matter most](requirements.md#gaps-that-matter-most).
This page does not repeat them. In short, and so that no green run is read as more
than it is:

- **Nothing here has met anything real.** No frame from a camera or a real board
  has been read, no accelerometer, haptic engine, speaker or screen reader has
  been tried, no camera device has been opened (the desktop camera's Qt
  Multimedia path is built and its screens and conversion are tested, but **this
  machine has no camera**), and the shake detector's thresholds are starting values
  tuned on synthetic traces. Every scanner accuracy number is on frames the corpus
  draws ([Scanner design](design/scanner.md#what-this-does-not-prove)). The iOS build is written and
  has never run on a device; Android is not built; the iOS camera and reader
  (AVFoundation, Vision) are not written.
- **No required check holds a performance budget**, and the window's frame check
  fails on the author's 59.95 Hz screen and has never run in CI
  ([Performance tests](#performance-tests)). Nothing was measured on a tablet or a
  phone, a launch is not held, and boards larger than 6x6 are not in the corpus.
- **The translations are not tested as translations**: 90 to 96 messages in each
  non-English catalogue are the English source and no test counts them
  ([above](#the-interfaces-languages)).
- **Some of the window is tested through stubs or not at all**: `Main.qml`, the
  route into Mode A, the My jewels screen, Mode A's play screens and the screen
  reader are listed [above](#the-interactive-qml-qt-quick-test); the Qt
  application is built and tested in CI only on Windows, and nothing was run on a
  touch screen, a tablet, a phone, Linux or macOS.
- **There is no phonics content** and no literacy reviewer. The reader, the
  checker, the seeding and the sound-it-out sequence are tested on a dozen words of
  test data written in the tests, with placeholder sounds and no clips; that the
  data is right and suits a child is for a reviewer, not a test. The standard board
  has no block list, an accepted risk that a test pins
  ([Game rules](#game-rules-what-is-pinned)).
- **The gate is a brake, not security**: one press-and-hold, which a determined child
  passes. The tests hold the hold, not a key sent to the gate or a hold cancelled by
  moving off it ([the gate's suite](#the-interactive-qml-qt-quick-test)).
- **The release pipeline has not run on GitHub**: `release.tooling` runs its scripts
  with a throw-away key, and the packages the release workflow makes for Linux and
  macOS are unrun.
- **`sns-cli` and the console.** `system.cli.*` runs the program (its options, the
  four exit codes - 0 solved, 1 usage, 2 dictionary, 3 board - and what it prints
  from a twelve-word list). Not tested: the console's UTF-8 mode on Windows and paths
  longer than `MAX_PATH` ([known problems](building.md#platform-notes)), and nobody
  has read the output on a real terminal on Linux or macOS. The console code does
  nothing there (a POSIX terminal already carries UTF-8); the path code is covered by
  `paths_test` in CI on both.
- **Apple clang's boards.** `seeding_test` and `dice_test` pin boards, which is how
  a difference between standard libraries would show. They were made on MSVC and come
  out the same on GCC 15.2 with libstdc++ (run in a container, in Debug and Release);
  Apple clang with libc++ has not been run.
- **Threads.** The library keeps no shared state between calls, but nothing tests
  solving from several threads at once.
