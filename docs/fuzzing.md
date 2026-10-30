# Fuzzing

*For developers. What CI runs is [CI](ci.md); the tests that hold the readers down by hand are in
[Testing](testing.md); what the program protects is [Security](security.md).*

Everything the program reads that a person or another program may have written has a
[libFuzzer](https://llvm.org/docs/LibFuzzer.html) harness: the board text, the word list, the dice file, the
two phonics files, the store file, and the camera frame the scanner is handed. A reader returns an error for
a bad input and never crashes; the fuzzers are there to find the input for which that is not true.

| Library | Targets (`sns_fuzz_<name>`) |
| --- | --- |
| `libs/core/fuzz` | `board` (`Board::parse`: every position a board says it has holds a tile of one to three letters), `cli_arguments` (`parse_cli_arguments`, the command line of `sns-cli`, one argument to a line: a wrong one is an error with a sentence, a right one has a repeat count of one or more), `dictionary` (`Dictionary::load`), `dice_set` (`DiceSet::parse`, and the roll of a set with a square number of dice), `phase_graphemes` and `phase_words` (the phonics files; every problem is reported and can be written out), `store` (`parse_store`; a store that reads writes out again and reads back as the same data) |
| `libs/scan/fuzz` | `frame_reader` (`read_board` over a frame: the first five bytes of the input are the pixel format, the width and the height, each up to 256, and a stride; the rest are the pixels, so the buffer is as short or as long as the input makes it) |

The `store` target also tries each input with its checksum rewritten. The file ends in the FNV-1a hash of
every line before it, and a mutated file almost never keeps a right one, so without this the fuzzer would
not get past the first check to the records. The rule is copied into the harness
(`libs/core/fuzz/store.cpp`); if the format changes, the seeds stop reaching the record parser and the
coverage of that target drops.

Not fuzzed: the command line of `sns-cli`, whose argument parser is in `apps/cli/src/main.cpp` and is not
a function a harness can call; and the application's QML. Moving the parser into the library would add it.

Each target has a directory of **seed inputs** (`corpus/<name>/`), written by
`python3 tools/fuzz/make_seeds.py` from the real files of `data/`, the golden store text of the tests and a
few hand-made cases at the edge of each format, and a directory of **regressions**
(`regressions/<name>/`): inputs that once crashed it.

## Running them

The harnesses are built with Clang, libFuzzer, AddressSanitizer and UBSan by the `linux-fuzz` preset. It
builds the libraries and the harnesses only (no Qt, no tests, no Tracy, and none of the hardening flags: a
fortified libc call bypasses AddressSanitizer), as a Debug build, so the standard library's assertions are
on. It needs a Clang with its sanitizer runtime (`libclang-rt-<n>-dev` on Ubuntu):

```sh
CXX=clang++ cmake --preset linux-fuzz
cmake --build --preset linux-fuzz
ctest --preset linux-fuzz        # replays the seeds and the regressions once: seconds
```

To fuzz one target, give libFuzzer a directory for what it finds and the committed directories to start
from:

```sh
mkdir -p /tmp/work
build/linux-fuzz/bin/sns_fuzz_store -max_total_time=300 /tmp/work \
    libs/core/fuzz/corpus/store libs/core/fuzz/regressions/store
```

A crash leaves the input in a file named `crash-<hash>`. Run the target on that file alone to see the
report again. The sanitizers abort on the first undefined behaviour (`-fno-sanitize-recover=all`), and a
harness aborts on a property that does not hold, so both count. The frame reader costs about 14 ms a frame,
so it gets far fewer executions in a run than the text readers; that is the reader's cost, not the
harness's.

## When a fuzzer finds something

Fix the reader, and add the input that crashed it to `libs/<lib>/fuzz/regressions/<name>/` in the same
pull request, so that the pull request gate replays it from then on. Give the file a name that says what it
was (`truncated-record.txt`). Do not add the files libFuzzer found that did not crash; the seed corpus is
meant to stay small and readable.

## In CI

- **Pull requests and the merge queue:** the `Fuzz regression (Linux, Clang)` job of `ci.yml` builds the
  harnesses and replays the seeds and the regressions (`ctest --preset linux-fuzz`). It is planned like the
  other jobs ([What runs when](ci.md#what-runs-when)) and `CI Status` holds the run to it.
- **Push to `develop`:** `fuzz.yml` fuzzes every target for 60 seconds.
- **Nightly (03:47 UTC):** `fuzz.yml` fuzzes every target for 10 minutes, with the inputs found so far kept
  between runs in the Actions cache. A crash fails the run, uploads the input as the `fuzz-crashes` artifact
  and opens or updates the `nightly-analysis` issue.
- **By hand:** `gh workflow run fuzz.yml --ref <branch> -f seconds=300`.

`fuzz.yml` is not a required check: it is not on a pull request, so a check would never report.
