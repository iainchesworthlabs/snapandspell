# SnapAndSpell

SnapAndSpell finds every word on a Boggle board.

The goal is to hold a real Boggle board up to a webcam, or a phone or tablet
camera, and have it solve the board, as a learning aid for young children (five
and six year olds) playing with an adult. It is also a game: Solo Play, where a
child shakes a box of dice and finds words. You can type a board into `sns-cli`
and get the words and their scores; the application plays Solo Play and can read
a board from the camera, though the reader has only been tested on drawn frames.

!!! note "Status: pre-release, 0.1.0 not yet released"
    The solver, the Solo Play game and the parent area are real and tested.
    The scanner's reader and the desktop camera exist, and **no real board has
    been read**: the reader is tested on drawn frames only
    ([Scanner design](design/scanner.md)). CI builds and tests the library and
    `sns-cli` on Windows, Linux and macOS and the application on Windows; the
    release workflow builds packages for all three desktops and an iOS build for TestFlight
    for the author's own devices, and has never run on GitHub. Android is in
    progress and does not exist yet. The English word list is ENABLE. It has not been
    tried with children. [Architecture](architecture.md) says what is built,
    [Requirements and traceability](requirements.md) lists each requirement with the
    test that shows it, and the [roadmap](roadmap.md) says what comes next.

## What works today

- **A library** (`libs/core`) that parses a board, holds a dictionary as a trie,
  scores words and finds every word on the board with a depth-first search. It
  has no GUI and no operating system code.
- **`sns-cli`**, a command-line front end to the library: give it a board and a
  word list, and it prints the words, their scores and the tiles that spell
  them.
- **The Qt Quick application**, built when Qt 6.12 is found
  (`SNS_BUILD_APP=AUTO`; `ON` requires it, `OFF` skips it):
  the Solo Play game of [Game design](design/game.md) (shake the box, trace
  words on the board and collect jewels) played over the library's session, a
  parent area behind a gate (the dashboard, the settings, About and licences),
  the interface in eight languages, and Mode A: a camera or typed board that the
  grown-up confirms, and a helper that lights a start tile. It is a shell over
  the same library.
- **A board reader** (`libs/scan`) that finds a board in a camera frame and reads
  its letters, with a confirm-and-correct step so the solver only sees a
  confirmed board. Tested on drawn frames.
- **A platform layer** (`libs/platforms`) holding the little that differs
  between Windows, Linux, macOS and iOS, and a check that nothing else does. See
  [Platform isolation](platform-isolation.md).

## Using the command-line solver

```text
sns-cli [-d <file>] [-m <n>] [-s boggle|rarity] <row>...
```

Each `<row>` is one row of the board, as one argument. A worked example is in
the [README](https://github.com/iainchesworthlabs/wordjewelssolver/blob/develop/README.md#a-worked-example).

### The board

- A row with no spaces is read one letter per tile: `TAPE` is four tiles.
- Otherwise tiles are separated by spaces or commas, and a tile may hold one to
  three letters: `"Qu I T"` is three tiles. Quote such a row so that the shell
  passes it as one argument.
- Letters are A to Z, in either case. A tile of `Q` is the letter Q; write
  `Qu` for the Boggle face. A `Qu` tile spends two letters of the word.
- The rows must all be the same length, and the board at most 16 by 16.
- Rows may also be separated by `/` or by line breaks inside one argument, so
  `CAT/ORE/DGS` is the same board as `CAT ORE DGS`.

Tiles touch horizontally, vertically and diagonally, and a tile is used at most
once in a word. A word that can be traced several ways is reported once, with
the first path found.

### The word list

`-d <file>` names a plain text file with one word per line. If `-d` is not
given, the program looks for `dictionaries/en.txt` beside the executable. A
build copies the English dictionary there from `data/dictionaries/en.txt`
(ENABLE; see [Licensing](licensing.md#the-word-list)).

- Words are the letters A to Z, in either case. Blank lines are ignored, and so
  are spaces and tabs around a word.
- A line that is not a word of letters is skipped, not reported: a word with
  an apostrophe or a hyphen, a digit, or a letter outside A to Z (`café` is
  skipped).
- A UTF-8 byte order mark at the start of the file is ignored.
- A word listed twice is counted once. Windows and Unix line endings both work.

### Options

| Option | Meaning |
| --- | --- |
| `-d`, `--dictionary <file>` | The word list. Default: `dictionaries/en.txt` beside the executable |
| `-m`, `--min-length <n>` | The shortest word to report, in letters (a `Qu` tile is two). Default 3 |
| `-s`, `--scoring <scheme>` | `boggle` (default) or `rarity` |
| `-r`, `--repeat <n>` | Solve the board `n` times and print one result. For timing and for [profiling](profiling.md), where a profiler needs about a second to attach. Default 1 |
| `-h`, `--help` | Show the usage text and exit |
| `--version` | Show the version and the platform it was built for |

### Output

One line per word: the word, its score, and the path as `r<row>c<column>`,
counted from 1. Then a total line. Words are ordered by score (highest first),
then length (longest first), then alphabetically.

```text
$ sns-cli -d words.txt -s rarity CAT ORE DGS
DOG                28   r3c1 r2c1 r3c2
COD                24   r1c1 r2c1 r3c1
CAR                21   r1c1 r1c2 r2c2
CAT                20   r1c1 r1c2 r1c3
CARE               18   r1c1 r1c2 r2c2 r2c3
ROD                17   r2c2 r2c1 r3c1
TAR                13   r1c3 r1c2 r2c2
ARE                12   r1c2 r2c2 r2c3
TEA                10   r1c3 r2c3 r1c2
9 words, 163 points
```

That is the board and word list of the README example, scored by letter
rarity.

### Scoring

- **`boggle`**: the scoring printed in the Boggle rules. Words of three or four
  letters score 1, five letters 2, six 3, seven 5, and eight or more 11. A word
  shorter than three letters scores 0, so lowering `-m` to 2 lists two-letter
  words with no points.
- **`rarity`**: the scheme of the original 2013 program. A word scores the mean
  of 100 divided by the percentage frequency of each of its letters in English
  (from a table in `libs/core/src/scoring.cpp`), truncated to a whole number, so a
  word of rare letters beats a word of common ones.

### Exit codes

| Code | Meaning |
| --- | --- |
| 0 | Solved (even if no words were found), or `--help` or `--version` |
| 1 | A usage error: an unknown option, an option without its value, a bad number or scheme, or no board |
| 2 | The word list could not be opened or read |
| 3 | The board could not be read |
| 4 | An exception the program did not foresee |
| 5 | The result could not be written to standard output |

Errors go to standard error as `sns-cli: <what went wrong>`. The board is read
before the word list is opened, so a bad board is reported as 3 even if the
word list is also missing.

## Using the window

The application is built when a Qt 6.12 kit is found (`SNS_BUILD_APP=AUTO`; see
[Building](building.md#the-qt-quick-window)), then start `SnapAndSpell`
from `build/<preset>/bin/`. It plays the Solo Play
game: shake the box and trace words on the board. A grown-up reaches the
settings, the dashboard and the scanner (Mode A) through the parent button, which
asks for a two-second press-and-hold. The default board is the
**standard board**, a roll of the project's dice (`dice/` beside the executable)
at 3x3 to 6x6 against the English dictionary (`dictionaries/en.txt` beside the
executable, or the one `--dictionary <file>` names); the phonics board is played
when the program is started with `--phase-directory <dir>` and the Phonics board
is chosen in the parent area's settings. A game in progress is kept in the store
(`--store <file>`, or the per-user file) and comes back after a restart. The
application always uses a minimum length of three letters and its own star and
point scoring; the options of `sns-cli` have no equivalent in it.

## Where to go next

| | |
| --- | --- |
| [Building](building.md) | Requirements, presets, options, vcpkg and Qt, platform notes |
| [Testing](testing.md) | The suites and checks, sanitizers, coverage, the performance tests |
| [Profiling](profiling.md) | Profiling with Tracy: building, capturing, reading a trace |
| [Fuzzing](fuzzing.md) | The libFuzzer harnesses of the readers: running them, the corpus, what CI does |
| [Architecture](architecture.md) | The library and the platform layer |
| [Platform isolation](platform-isolation.md) | The rule about operating system code, and its check |
| [Licensing](licensing.md) | The licence, the dependencies, the word-list question |
| [CI](ci.md) and [Branch protection](branch-protection.md) | What runs on a pull request, and how the branches are protected |
| [Roadmap](roadmap.md) | What is planned |
| [Product requirements](product-requirements.md) | The product brief the roadmap is built from |
| [Requirements and traceability](requirements.md) | Each requirement, where it is built, the test that shows it, and what is left |
| [Design](design/index.md) | The game, board, phonics, application, parent-area and scanner design |
| [FAQ](faq.md) | Short answers |
