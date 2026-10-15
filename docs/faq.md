# FAQ

**Does it solve a Boggle board today?**
Yes, from a board you type in: give `wjs-cli` the rows of the board and a word
list, and it prints every word that can be traced on the board with its score
and the tiles that spell it. There is also a small Qt Quick window that does
the same from a grid, built only on request. See the
[home page](index.md#using-the-command-line-solver).

**Does it use a camera?**
No. Holding a board up to a webcam, or a phone or tablet camera, is the goal, and it is not
built: there is no camera input and no reading of letters from a picture. The
[architecture](architecture.md#not-built-yet) says what is missing, and the
[roadmap](roadmap.md) says what is planned.

**Is it ready for a five-year-old?**
No. It is meant, in the end, as a learning aid for children of five and six
playing with an adult, and nothing here has been designed or tried with a child
yet. Today it is a solver for an adult who can type a board in.

**Where is the word list? `wjs-cli` says the dictionary file could not be opened.**
Without `-d`, the program looks for `dictionaries/en.txt` beside the
executable, and a build copies ENABLE (`data/dictionaries/en.txt`; see
[Licensing](licensing.md#the-word-list)) there. If you moved the executable, move
its `dictionaries` folder with it, or give any text file of words, one per line,
with `-d <file>`.

**A word I expected is missing.**
In order of likelihood: it is not in your word list; it is shorter than the
minimum of three letters (lower it with `-m`); its letters do not form a path
of touching tiles, with no tile used twice (tiles touch sideways, up, down and
diagonally); or the word has a letter outside A to Z or an apostrophe or
hyphen, and the word list loader skips such a line.

**Why does `-m 2` list words with a score of 0?**
Under the Boggle scoring a word of fewer than three letters scores nothing. Use
`-s rarity` for a scheme that scores every length.

**How do I write the "Qu" tile?**
As a tile of two letters: `"Qu I T"`, with the tiles separated by spaces or
commas and the row quoted. A tile of `Q` alone is the letter Q. A `Qu` tile
counts as two letters of the word, for the minimum length and for the score.

**Does it work with other languages, or accented letters?**
Not yet. Words and tiles are the letters A to Z only; a word with any other
letter is skipped when the list is loaded, and a tile with one is a board
error.

**Which platforms does it run on?**
The code is written for Windows, Linux and macOS, with the differences between
them kept in `src/platforms`. Only Windows with MSVC has been built and run. The
Linux and macOS code has not yet been built, so expect to fix something there.
See [Building](building.md).

**Is this the 2013 program?**
It is the same project, rebuilt. The original was a Windows-only program using
Win32 and DirectShow, built with a Visual Studio solution. That code is gone
from the tree and survives in the git history before the rebuild. The solver
was rewritten in C++23 as a library with tests; the letter-rarity scoring of
the original is kept as `-s rarity`.

**Why is operating system code kept in its own directory, with no `#ifdef`?**
So that the program stays portable by construction, and so that finding what
differs between systems is a matter of reading one directory. See
[Platform isolation](platform-isolation.md).

**Opening a file fails on Windows with a long path.**
A known problem: on Windows, opening a file by a path longer than `MAX_PATH`
fails unless the executable is long-path aware, and these programs are not yet.
Found during testing and not fixed. Put the word list at a shorter path.

**Is anything uploaded?**
The project's own code does not open a network connection: the command-line
program reads the files you name and writes to the console, and the window does
the same. There is no camera input yet, so there are no pictures to handle.

**Can I use the library in my own program?**
There is no install step or exported package yet, so it can only be built as
part of this tree. Its interface is the four headers in
`src/lib/include/wjs/`, and the [architecture](architecture.md#the-library)
describes them. A program that includes it is subject to the terms of the
GPL-3.0-or-later.

**Why GPL-3.0-or-later?**
It is the licence of the sibling project Cubeulator, and it keeps the code
free for whoever receives it. It is
[described in Licensing](licensing.md#the-projects-own-licence). The "or
later" is the reason the project avoids Qt modules that are offered only under
GPL-3.0-only.

**How do I build without the tests?**
Configure with `-DWJS_BUILD_TESTS=OFF`: the manifest then asks vcpkg for no
packages (Catch2 is the only one it ever asks for), and the tests and platform
checks are not built. See [Building](building.md#options).
