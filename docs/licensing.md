# Licensing

An engineering inventory of what the project is licensed under, what it uses,
and what is still open. It is not legal advice, and it does not change the
licence.

How to read the evidence marks:

- **[V]** verified against a file in this repository, an installed package or
  the git history.
- **[I]** inferred, or taken from the sibling project Cubeulator's licensing
  paper, which checked Qt 6.12's licences against Qt's own SBOMs. Treat as a
  lead, and re-check before relying on it.
- **[L]** a lawyer should confirm it before anyone relies on it.

## Summary

1. **The project is GPL-3.0-or-later** [V: `LICENSE`, `vcpkg.json`]. The files
   carry no per-file headers; `LICENSE` and `vcpkg.json` are the statement.
2. **The history has one author**, so as far as it shows there is one
   copyright holder, and the choice of licence is theirs [V: `git shortlog`].
3. **Nothing third-party is shipped, because nothing is packaged.** The
   third-party code in use today is a build tool (vcpkg) and a test framework
   (Catch2).
4. **Qt 6.12 is used by the optional window** and is planned for the GUI. The
   intent is to link only modules that are also offered under the LGPL, so that
   "or later" stays true of the whole program (see [Qt](#qt)).
5. **The English word list is ENABLE**, a separate public-domain work kept
   unmodified in `data/dictionaries/` (see [The word list](#the-word-list)).
6. **The history begins with the author's own 2013 program**, written before
   this licence was chosen. Third-party files committed then (a vendored OCR
   build tree and prebuilt libraries, build output, and a word list whose origin
   was not recorded) were removed from every commit before the repository was
   published (see [History](#history)).

## The project's own licence

WordJewelsSolver is licensed under the GNU General Public License, version 3,
or (at your option) any later version. The text is in `LICENSE` and is the
unmodified GPLv3. `vcpkg.json` says `GPL-3.0-or-later`, and so does this page.

- **Authorship.** Every commit is by Iain Chesworth, under several email
  addresses, from 2013 and from 2026 [V: `git shortlog -sne`]. A copyright
  holder may add permissions, dual-license or re-license their own code [I][L].
- **Contributions** are accepted under the same terms, GPL-3.0-or-later. There
  is no contributor agreement.
- **Not covered by that statement:** the third-party components below, and the
  code from before the licence was chosen ([History](#history)).
- **App stores.** There are no packages and no store listings yet, so GPLv3's
  friction with some stores' terms is not a live question for the first
  release. It will be when the tablet and phone targets in the
  [product requirements](product-requirements.md) and the
  [roadmap](roadmap.md) are built: the Apple App Store in particular. The
  analysis to repeat is the one in Cubeulator's
  [LICENSE-NOTE.md](https://github.com/iainchesworthlabs/cubeulator/blob/main/LICENSE-NOTE.md)
  and [licensing paper](https://github.com/iainchesworthlabs/cubeulator/blob/main/docs/licensing.md).

## Inventory

| Component | Licence | Role | In a build you distribute |
| --- | --- | --- | --- |
| WordJewelsSolver (`src/`, `tests/`, `tools/`, `cmake/`, `docs/`) | GPL-3.0-or-later [V] | This project | Yes |
| vcpkg (`deps/vcpkg`, a git submodule of the labs' fork) | MIT, Copyright Microsoft Corporation [V: `deps/vcpkg/LICENSE.txt`] | Fetches and builds dependencies | No: a build tool |
| vcpkg helper ports `vcpkg-cmake`, `vcpkg-cmake-config` | MIT [V: installed `share/*/copyright`] | Build helpers | No |
| Catch2 3.15.3 | BSL-1.0 [V: installed `share/catch2/copyright`] | The test framework (`tests` vcpkg feature) | No: tests only |
| Qt 6.12 (Core, Gui, Qml, Quick, Quick Controls 2) | Commercial, LGPL-3.0, GPL-2.0 or GPL-3.0 [I] | The optional Qt Quick window; not built by vcpkg | Not yet: there are no packages |
| Tracy | BSD-3-Clause [I] | Planned: opt-in profiling | Not present yet |
| A camera and letter-recognition stack | Not chosen | Planned | Not present yet |

Ports are listed with the licence in their installed `copyright` file. When
packages exist, the third-party notices they must carry will be listed here
from what actually ships, not from this table.

## Qt

Qt is a prebuilt dependency found through `CMAKE_PREFIX_PATH` or `QT_ROOT_DIR`;
it is not part of the repository and is not built by vcpkg. Today only the
optional window (`WJS_BUILD_APP`) uses it. The modules it links are Core, Gui,
Qml, Quick and Quick Controls 2, which Qt offers under the LGPL-3.0 as well as
the GPL [I].

The rule for adding another module is to check how Qt offers it first, in "Qt
licensing" in the Qt 6.12 documentation, and to avoid modules that are offered
only as **GPL-3.0-only** (or commercially). The reason is the "or later": the
project's source files are GPL-3.0-or-later, but a program that links a module
available only under GPL-3.0-only can be conveyed only under GPL-3.0, so "or
later" would stop being true of the whole [I][L]. Cubeulator's paper found this
for Qt Quick 3D, which is offered as commercial or GPL-3.0-only with no LGPL
option; it is the example to remember. A module offered under the LGPL does not
raise it. `<qt>/sbom/*.spdx.json` in a Qt install holds the licence expression
of every module in that install.

Qt's build tools (`moc`, `rcc`, `windeployqt` and others) are GPL-3.0 with
Qt's GPL exception, and are not shipped; the exception covers their output [I].

When packages exist, shipping Qt's LGPL modules brings their own obligations
(the licence text, notices, and keeping the user able to replace the Qt
libraries). They are not yet a question, because nothing is distributed.

## The word list

The solver needs a word list. The one in the repository is **ENABLE**
(`enable1.txt`, 172,823 words), kept unmodified as `data/dictionaries/en.txt`.
Its originators released it into the public domain; where it was fetched from,
when, and its SHA-256 are in [data/README.md](https://github.com/iainchesworthlabs/wordjewelssolver/blob/main/data/README.md).
That the notice is genuine is taken from the list's published statement and has
not been independently verified [I][L]. `wjs-cli` and the window look for
`dictionaries/en.txt` beside the executable, and a build copies it there.

The sentences that follow are why the choice of list was a decision at all, and
what any replacement list has to satisfy.

Why it matters: a word list is data with its own licence, separate from the
program's. Some lists are in the public domain or under permissive licences;
others forbid redistribution or carry terms that do not fit a GPL-3.0-or-later
program shipped with its source. The decision has to settle, at least:

- that the licence allows redistributing the list alongside this program, and
  its source, and that the licence text and any attribution travel with it;
- where the list came from, written down;
- whether a list made for a word game for adults is the right list for a child
  of five or six playing with an adult (a long list is full of words neither
  of them has seen, and a short one will say that a real word is not a word);
- which English: spellings differ between countries.

The word list the original 2013 program used is not part of this repository:
nothing recorded where it came from.

Rules for word lists:

- **do not add a word list without recording its source, date and licence** in
  `data/README.md`, and keep a list that is someone else's work unmodified so its
  hash can be compared;
- the child-facing category and tier lists under `data/lexicon/` are written for
  this project and are subsets of the dictionary, checked by
  `tools/data/check_wordlists.ps1`;
- the tests use lists of a dozen words written in the test itself;
- a list you supply with `-d` is yours, under its own terms; the program reads
  it and does not copy it anywhere.

## Camera input and letter recognition

Neither is built, and the stack is not chosen. When it is:

- record the licence of every library, and also of any trained model or data
  file: they often carry terms of their own, different from the code's, and
  some are not redistributable at all;
- fetch such files at build or first run rather than vendoring them. The
  `.gitignore` already excludes `*.traineddata` and `*.onnx` on that footing;
- a picture of a person must never be committed, and this application is meant
  to be pointed at a game played with children. `/tests/data/captures/` and
  `/docs/evidence/` are git-ignored for that reason, and a test that wants a
  recording must find it outside the tree and skip when it is not there.

## History

The history begins on 2013-06-05 with the original Windows program, which is
the author's own code, written before this licence was chosen. Third-party
files committed in that period (a vendored OCR build tree with prebuilt
libraries, build output, and a word list whose origin was not recorded) were
removed from every commit before this repository was published, so none of them
is in the history or in any release. The tag `legacy-win32-2026-10-15` marks
the last commit of the original Win32 and DirectShow program.

## What a lawyer should confirm

- That distributing a build of this program together with Qt's LGPL modules,
  under GPL-3.0-or-later, meets both sets of terms, in the packaging that is
  eventually chosen.
- The origin and terms of the 2013 word lists, and, if the history were ever
  published with them in, whether it may be.
- Anything to do with an app store, before a submission.
