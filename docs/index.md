# WordJewelsSolver

WordJewelsSolver finds every word on a Boggle board.

The goal is to hold a real Boggle board up to a webcam, or a phone or tablet
camera, and have it solve the board, as a learning aid for young children (five
and six year olds) playing with an adult. That goal is not met yet. Today you
type the board in, and the program finds and scores the words.

!!! note "Status: pre-release, nothing released yet"
    The solver is real and tested. Camera input, reading letters from a
    picture, installers and an English word list do not exist yet. It has been
    built and run on Windows with MSVC only; the Linux and macOS code is
    written but has not been built. It has not been tried with children.
    [Architecture](architecture.md) says what is built, and the
    [roadmap](roadmap.md) says what comes next.

## What works today

- **A library** (`src/lib`) that parses a board, holds a dictionary as a trie,
  scores words and finds every word on the board with a depth-first search. It
  has no GUI and no operating system code.
- **`wjs-cli`**, a command-line front end to the library: give it a board and a
  word list, and it prints the words, their scores and the tiles that spell
  them.
- **A small Qt Quick window**, opt-in (`-DWJS_BUILD_APP=ON`, needs Qt 6.12):
  type a board into a grid, press Solve, see the scored words. It is a shell
  over the same library, not the camera application.
- **A platform layer** (`src/platforms`) holding the little that differs
  between Windows, Linux and macOS, and a check that nothing else does. See
  [Platform isolation](platform-isolation.md).

## Using the command-line solver

```text
wjs-cli [-d <file>] [-m <n>] [-s boggle|rarity] <row>...
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
| `-h`, `--help` | Show the usage text and exit |
| `--version` | Show the version and the platform it was built for |

### Output

One line per word: the word, its score, and the path as `r<row>c<column>`,
counted from 1. Then a total line. Words are ordered by score (highest first),
then length (longest first), then alphabetically.

```text
$ wjs-cli -d words.txt -s rarity CAT ORE DGS
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
  (from a table in `src/lib/src/scoring.cpp`), truncated to a whole number, so a
  word of rare letters beats a word of common ones.

### Exit codes

| Code | Meaning |
| --- | --- |
| 0 | Solved (even if no words were found), or `--help` or `--version` |
| 1 | A usage error: an unknown option, an option without its value, a bad number or scheme, or no board |
| 2 | The word list could not be opened or read |
| 3 | The board could not be read |

Errors go to standard error as `wjs-cli: <what went wrong>`. The board is read
before the word list is opened, so a bad board is reported as 3 even if the
word list is also missing.

## Using the window

Build with `-DWJS_BUILD_APP=ON` (see [Building](building.md#the-qt-quick-window)),
then start `WordJewelsSolver` from `build/<preset>/bin/`. It loads the word list
named by `--dictionary <file>` on its command line, or `dictionaries/en.txt`
beside the executable, and says in the window if it cannot. Type one to three
letters into each tile (a lone `Q` means the Qu face), press Solve, and the
words appear with their scores. The board is square, from 2 by 2 to 8 by 8.
The window always uses a minimum length of three letters and standard Boggle
scoring; the options of `wjs-cli` have no equivalent in it yet.

## Where to go next

| | |
| --- | --- |
| [Building](building.md) | Requirements, presets, options, vcpkg and Qt, platform notes |
| [Testing](testing.md) | The suites and checks, sanitizers, coverage |
| [Architecture](architecture.md) | The library and the platform layer |
| [Platform isolation](platform-isolation.md) | The rule about operating system code, and its check |
| [Licensing](licensing.md) | The licence, the dependencies, the word-list question |
| [CI](ci.md) and [Branch protection](branch-protection.md) | What runs on a pull request, and how the branches are protected |
| [Roadmap](roadmap.md) | What is planned |
| [Product requirements](product-requirements.md) | The product brief the roadmap is built from |
| [FAQ](faq.md) | Short answers |
