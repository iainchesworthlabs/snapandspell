# FAQ

**Does it solve a Boggle board today?**
Yes, from a board you type in: give `wjs-cli` the rows of the board and a word
list, and it prints every word that can be traced on the board with its score
and the tiles that spell it. The Qt Quick application does the same for a board
you type or scan, and also plays Solo Play with a child. See the
[home page](index.md#using-the-command-line-solver).

**Does it use a camera?**
Yes, in the application on the desktops, and it has never read a real board. Mode A
shows a live picture from the camera (Qt Multimedia) and **Capture** reads one frame
with a reader that finds the board and its letters; the person then confirms or
corrects the letters, or types the board in instead. The reader is tested on drawn
frames only, and the camera backend has not been used in a test or on a real board
([scanner design](design/scanner.md)). A picture is read in memory and never saved
or sent anywhere. `wjs-cli` has no camera. The
[architecture](architecture.md#not-built-yet) says what is missing, and the
[roadmap](roadmap.md) says what is planned.

**Is it ready for a five-year-old?**
Not tested. It is meant as a learning aid for children of five and six playing
with an adult. Solo Play is built for that age (large targets, no reading needed to
play, nothing that can be lost), but it has not been tried with a child, and the
literacy content (the phonics phases and sounds) does not exist because it needs a
reviewer. The standard board also has no block list, so it can show any word in the
ENABLE list. The plan for trying it is in
[Application design](design/application.md#user-study-plan-open).

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
them kept in `src/platforms`. CI builds and tests the library and `wjs-cli` on
Windows (MSVC), Linux (GCC) and macOS (AppleClang); the Qt application is built and
tested in CI on Windows, and built for Linux and macOS by the release workflow,
which has never run. An ad-hoc iOS build for the author's own devices is written
and has never run on a device. Android is being worked on separately and is not
built; there is no web build. See [Building](building.md).

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
program reads the files you name and writes to the console, and the application
keeps its progress in one small file on the device (the store) and sends nothing
anywhere. The camera's picture is read in memory and never saved; no audio is
captured. Build checks (`privacy.sources`, `scan.privacy`) fail if the first-party
code opens a network connection, captures audio, exports data or writes a frame to
disk. The one exception is a *profiling* build, which listens on a local TCP port
and is never released ([Security](https://github.com/iainchesworthlabs/wordjewelssolver/blob/develop/SECURITY.md)).

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
