#pragma once

// Every threshold of the performance tier, in one file.
//
// How to read a number here, and how to move it, is in docs/testing.md
// ("Performance tests"). The short version: each work budget sits a quarter
// above what the corpus measures today, so that a change that really does cost
// more is a failure with a name on it, and a change that costs a little more
// is a conscious edit of a number in this file, not a surprise.
// `wjs_perf_tests "[.perf-report]"` prints what each metric measures now beside
// its budget, and how much room is left.
//
// A work budget is exact on every machine and compiler, so it may be tight. A
// time budget is a ratio to a calibration workload and is deliberately loose:
// it is there for the order-of-magnitude slowdown a visit count cannot see (a
// copy where there was a reference, an allocation per visit), and it must
// never fail because another process was busy.
//
// Measured with MSVC 19.51 (/O2, RelWithDebInfo) on one Windows 11 desktop;
// docs/design/performance.md records the machine and the figures.

#include <array>
#include <cstddef>
#include <cstdint>

namespace wjs::perf::budget {

// ---------------------------------------------------------------------------
// The dictionary: ENABLE, data/dictionaries/en.txt
// ---------------------------------------------------------------------------

// Exact, not a quarter above: they change when the data does, and then a
// person decides whether it was meant to (data/README.md).
inline constexpr std::uint64_t kDictionaryWords = 172'823;
inline constexpr std::uint64_t kDictionaryNodes = 387'889;

// ---------------------------------------------------------------------------
// The solver, counted in visits (wjs/solver_work.hpp)
// ---------------------------------------------------------------------------

// The most visits any one of the kBoardsPerSize random boards of each size
// (corpus.hpp) makes; kSizes order, 3x3 to 6x6. Beside each is the figure the
// budget is a quarter above, and the corpus seed of the board that made it.
inline constexpr std::array<std::uint64_t, 4> kMaxVisitsPerSize = {
    640,    // measured 514 (seed 1235367)
    1'940,  // measured 1,548 (seed 1298720)
    3'580,  // measured 2,859 (seed 617687)
    6'470,  // measured 5,174 (seed 1417507)
};

// The boards built to hold many words (corpus.hpp, kRichBoards order): the
// visits of the search, a quarter above what each measured (2,832 / 5,496 /
// 9,443 / 12,350), and the words it finds, which is exact because it pins the
// corpus and the dictionary together.
inline constexpr std::array<std::uint64_t, 4> kRichVisits = {3'550, 6'900, 11'800, 15'500};
inline constexpr std::array<std::uint64_t, 4> kRichWords = {1'126, 1'464, 1'788, 327};

// The boards with no word on them (kEmptyBoards order): exact. The search
// still walks every tile and its neighbours until the trie refuses.
inline constexpr std::array<std::uint64_t, 2> kEmptyVisits = {100, 256};

// A digest of every solution over the corpus (random boards, rich boards,
// empty boards): each word, its score and its path, in the order solve()
// returns them. One number for "the results did not change". It moves when the
// solver's output does, deliberately and with a reason in the commit; it does
// not move for a change that only makes the same answer faster.
inline constexpr std::uint64_t kCorpusDigest = 228066831253711239ULL;

// ---------------------------------------------------------------------------
// The two board modes
//
// Everything above is the STANDARD board's work (dice, the full word list), as
// docs/design/performance.md says. What follows is that mode's making of a
// board, and then the PHONICS board's seeding and binding. The standard board
// is the default and the phonics board a parent's switch (decided 2026-10-16),
// so each section below says which it is for and every metric carries its
// mode (metrics.hpp, BoardMode). The figures below are a quarter above the
// measurement, rounded up to three significant figures.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// STANDARD BOARD (the default): a roll of the project's dice, then a solve
// ---------------------------------------------------------------------------

// The work of making a standard board: roll data/dice/<n>x<n>.txt with the seed
// of each of the kBoardsPerSize corpus boards of a size (corpus.hpp,
// dice_board) and solve the result against the full word list. The most visits
// any one of them makes, kSizes order, a quarter above what each measured, and
// the corpus seed of the board that made it.
inline constexpr std::array<std::uint64_t, 4> kDiceMaxVisitsPerSize = {
    589,    // measured 471 (seed 886931)
    2'050,  // measured 1,636 (seed 823580)
    4'690,  // measured 3,748 (seed 300927)
    8'080,  // measured 6,457 (seed 277171)
};

// A digest of every solution over those boards, in the order solve() returns
// them. Exact, as kCorpusDigest is, and it moves with the dice files (which
// move with the dictionary: data/README.md) or the roll, by a person's decision.
inline constexpr std::uint64_t kDiceDigest = 9710656603150208005ULL;

// ---------------------------------------------------------------------------
// PHONICS BOARD (a parent's switch): seeding and binding
// ---------------------------------------------------------------------------

// The words of the test phase (corpus.hpp, phonics_phase): exact, because it
// pins the phase and the dictionary together.
inline constexpr std::uint64_t kPhonicsWords = 249;

// What seeding spent over kSeedsPerSize seeds of each size on that phase, with
// the options of wjs::phonics_board_options (guided fill, targets of three
// letters at most, every letter but the dead one as filler), the range 3 to 5
// and 20 tries: each a quarter above what it measured, by size, kSizes order.
//
//   tries_total   boards verified, summed (one solve each)
//   solves_total  every solve: the verifications and the guided fill's trials
//   visits_total  the solver's visits over all of them, the machine-independent
//                 cost of the whole corpus
//   visits_max    the visits of the one seeding that made the most
//   fallbacks     boards that were the nearest and not in range
struct SeedBudget {
    std::uint64_t tries_total;
    std::uint64_t solves_total;
    std::uint64_t visits_total;
    std::uint64_t visits_max;
    std::uint64_t fallbacks;
};

//
// 3x3 is the board this phase seeds worst: 77 of its 100 seeds are fallbacks
// (the 23% that land is what docs/testing.md measured for targets of three
// letters), so nearly every seeding spends all twenty verifications. A quarter
// above its 1,739 is 2,174, which is past the 2,000 that 100 seeds of twenty
// tries make; the budget is that cap, and cannot fail. What holds the 3x3 is
// the solves and the visits, which the guided fill's trials make unbounded.
inline constexpr std::array<SeedBudget, 4> kSeedBudgets = {{
    {2'000, 6'880, 610'000, 13'100, 97},   // 3x3: 1,739 / 5,503 / 487,209 / 10,460 / 77
    {830, 27'100, 1'300'000, 31'700, 7},   // 4x4: 664 / 21,653 / 1,036,442 / 25,292 / 5
    {243, 50'900, 3'160'000, 121'000, 0},  // 5x5: 194 / 40,663 / 2,525,621 / 96,479 / 0
    {234, 85'100, 6'980'000, 168'000, 0},  // 6x6: 187 / 68,049 / 5,578,096 / 133,802 / 0
}};

// The seeding's own cap on verifications (SeedOptions::max_tries). Not a
// quarter above anything: it is the algorithm's promise, and no seed may spend
// more.
inline constexpr std::uint64_t kSeedMaxTries = 20;

// A digest of every board seeded over the corpus: tiles, targets, dead jewels,
// the words found, tries and solves. Exact; it moves with the seeding
// algorithm or the phase, deliberately, and with the pinned boards of
// seeding_test.cpp.
inline constexpr std::uint64_t kSeedDigest = 15489434241397487766ULL;

// The binder's comparisons for a path of nine tiles (corpus.hpp, kBindPaths
// order: none, all, mixed) against the 31 multi-letter graphemes of its test
// set, a quarter above what each measured. A tile that starts no grapheme costs
// 31; one that does costs its place in the binder's order.
inline constexpr std::array<std::uint64_t, 3> kBindComparisons = {
    349,  // none: measured 279 (9 tiles x 31 graphemes)
    63,   // all: measured 50 (4 blocks)
    234,  // mixed: measured 187 (7 blocks)
};

// ---------------------------------------------------------------------------
// Time, as a multiple of the calibration workload (perf/measure.hpp)
// ---------------------------------------------------------------------------

// Each budget is a ratio of the task's fastest run to the calibration
// workload's fastest run, measured in the same process and run alternately
// with it, so that the machine's speed and a stretch of load cancel out. The
// comment above each is what that ratio measured; the budget is several times
// that. What is left over for the noise that does not cancel (frequency steps,
// a neighbour's cache, a compiler that is not the one the number was taken
// with) is the room between the two, and a copy or an allocation per visit is
// a factor of several, not a few percent. These are not 10% gates and must not
// be tightened into being one.
struct TimeBudget {
    double ratio;
};

// Loading ENABLE, against the heavy calibration (a working set of tens of
// megabytes, which loading a trie of 388,000 nodes has too). Measured 0.65 to
// 0.78: 66 to 68 ms against 84 to 125 ms, the calibration drifting with what
// else the machine was doing, which is what the ratio is for.
inline constexpr TimeBudget kLoadTime = {5.0};
// Solving the 4x4 and the 6x6 built to hold many words, one solve against the
// light calibration (a search is a few thousand visits, which a cache holds).
// Measured 0.042 to 0.043 (0.57 to 0.59 ms) and 0.115 to 0.120 (1.6 ms),
// against 13 to 17 ms.
inline constexpr TimeBudget kSolveRich4Time = {0.3};
inline constexpr TimeBudget kSolveRich6Time = {0.8};

// How many timed runs each takes, after one that is thrown away; and how many
// solves make one timed run of a solve (a solve is under a millisecond).
inline constexpr int kLoadRuns = 5;
inline constexpr int kSolveRuns = 15;
inline constexpr int kSolvesPerRun = 50;

}  // namespace wjs::perf::budget
