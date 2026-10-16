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

// ---------------------------------------------------------------------------
// The scanner, counted (wjs/scan/work.hpp)
// ---------------------------------------------------------------------------

// Reading four fixed synthetic frames (a 4x4, a 5x5, a 6x6, a 4x4 under glare,
// metrics.cpp). The reads, and every one answering with a board, and the pixels of
// the frames and of the working pictures, are exact: they pin the corpus. The rest
// are what a read does, a quarter above what each measured (rounded up to three
// significant figures), and are not exact because a decision at the edge of a
// threshold (which look found the board, which height fitted best) can differ by a
// compiler's last digit; a change that reads each cell at four times the angles, or
// allocates a buffer a row, is a factor and not a few percent.
inline constexpr std::uint64_t kScanReads = 4;
inline constexpr std::uint64_t kScanFramePixels = 4ULL * 960 * 720;
inline constexpr std::uint64_t kScanWorkingPixels = 4ULL * 480 * 360;
// Beside each, what it measured (MSVC 19.51, /O2): the warped pixels are the board at
// 480 x 480 and each of five looks for it at 240 x 240; the blobs are every bright
// connected set the five looks found, and the dice candidates the die-shaped ones.
inline constexpr std::uint64_t kScanWarpPixels = 2'600'000;  // measured 2,073,600
inline constexpr std::uint64_t kScanBlobs = 3'240;           // measured 2,589
inline constexpr std::uint64_t kScanDiceCandidates = 619;    // measured 495
inline constexpr std::uint64_t kScanCellsRead = 323;         // measured 258
inline constexpr std::uint64_t kScanComparisons = 418'000;   // measured 334,368
inline constexpr std::uint64_t kScanBuffers = 125;           // measured 100
inline constexpr std::uint64_t kScanBuffersMax = 32;         // measured 25

// One read of a 4x4 and of a 6x6 frame, against the heavy calibration (a read walks
// several pictures, none of which stays in cache). Measured 0.36 to 0.44 (45 to 50 ms)
// and 0.46 to 0.55 (61 to 69 ms) against 102 to 150 ms, three runs, on the desktop
// above; each budget is seven to eight times what it measures.
inline constexpr TimeBudget kScanRead4Time = {3.0};
inline constexpr TimeBudget kScanRead6Time = {4.0};
inline constexpr int kScanRuns = 7;

}  // namespace wjs::perf::budget
