# Data

Word lists. Plain text, one lowercase word per line, letters a to z, sorted
(byte order), no duplicates. `tools/data/check_wordlists.ps1` holds every file
to that and runs as the `data.wordlists` test.

## `dictionaries/en.txt`: the English dictionary

| | |
| --- | --- |
| What | ENABLE, the "Enhanced North American Benchmark Lexicon": `enable1.txt` |
| Words | 172,823 |
| Fetched from | `https://raw.githubusercontent.com/dolph/dictionary/master/enable1.txt` (the `dolph/dictionary` repository on GitHub) |
| Fetched on | 2026-10-15 |
| SHA-256 | `3F16130220645692ED49C7134E24A18504C2CA55B3C012F7290E3E77C63B1A89` |
| Changes | None. The file is kept byte for byte as fetched, so the hash above can be compared with the source. |
| Terms | Released into the public domain by its originators, who state that anyone is free to use or distribute it with no fee, registration or credit required. |

The public-domain statement is the one published with the list. It has not
been independently verified, and the chain of title is not recorded in this
repository: see [docs/licensing.md](../docs/licensing.md#the-word-list). It is
a separate work from this program and is not covered by the program's licence.

What it is not: ENABLE is a list for word games. It includes words a
five-year-old should not be shown. That is why the child-facing layer below is
an allow-list, not a filter over this file.

## `lexicon/`: the child-facing layer

Written for this project, not taken from another list. Every word is a word of
the dictionary above (the check fails otherwise), so a category or tier is a
subset of what the solver accepts and never a source of new words.

| File | Contents |
| --- | --- |
| `core-words.txt` | An early-reader tier: common short words and a few longer everyday ones |
| `categories/animals.txt` | Animals |
| `categories/colors.txt` | Colours (American spelling in the file name; both `gray` and `grey` are listed) |
| `categories/shapes.txt` | Shapes |
| `categories/food.txt` | Food and drink |
| `categories/body.txt` | Parts of the body |
| `categories/family.txt` | People in a family |
| `categories/nature.txt` | The outdoors |
| `categories/home.txt` | Things in a home and a classroom |
| `categories/clothes.txt` | Clothes |
| `categories/transport.txt` | Ways to travel |

These are a starting set. They have **not been reviewed** by a teacher or a
parent, they hold base forms only (no plurals), and a word can be in more than
one category (`star` is in `nature` and `shapes`). Nothing in the program reads
them yet; the [roadmap](../docs/roadmap.md) (M2) says how they are to be used.

## Adding a list

Record where it came from, when, and under what terms, in this file, in the
same form as the table above. Keep a list that is someone else's work
unmodified. Do not add a list whose origin you cannot state.
